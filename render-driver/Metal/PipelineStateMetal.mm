#include <render-driver/Metal/PipelineStateMetal.h>
#include <render-driver/Metal/DeviceMetal.h>

#include <render-driver/Metal/UtilsMetal.h>

#include <utils/wtfassert.h>
#include <utils/LogPrint.h>

extern char const* getSaveDir();

namespace RenderDriver
{
    namespace Metal
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPipelineState::create(
            RenderDriver::Common::GraphicsPipelineStateDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Metal::CPipelineState::GraphicsPipelineStateDescriptor const& metalPipelineDesc =
                static_cast<RenderDriver::Metal::CPipelineState::GraphicsPipelineStateDescriptor const&>(desc);

            RenderDriver::Common::CPipelineState::create(desc, device);

            RenderDriver::Metal::CDevice& deviceMetal = static_cast<RenderDriver::Metal::CDevice&>(device);
            mNativeDevice = (__bridge id<MTLDevice>)deviceMetal.getNativeDevice();
            
            NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationScriptsDirectory, NSUserDomainMask, YES);
            NSString* applicationSupportDirectory = [paths firstObject];
            char const* szDir = [applicationSupportDirectory UTF8String];
            printf("save directory: \"%s\"\n", szDir);
            
            NSString* resourcePath = [[NSBundle mainBundle] bundlePath];
            char const* szBundleDir = [resourcePath UTF8String];
            printf("bundle directory: \"%s\"\n", szBundleDir);
            
            NSError* error = nil;
            
            // load metallib binary data
            std::string filePath = std::string(szDir) + "/" + metalPipelineDesc.mLibraryFilePath;
            FILE* fp = fopen(filePath.c_str(), "rb");
            WTFASSERT(fp, "can\'t open file \"%s\"", metalPipelineDesc.mLibraryFilePath.c_str());
            fseek(fp, 0, SEEK_END);
            uint64_t iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<char> acFileContent(iFileSize);
            fread(acFileContent.data(), sizeof(char), iFileSize, fp);
            fclose(fp);
            
            dispatch_data_t data = dispatch_data_create(
                acFileContent.data(),
                iFileSize,
                dispatch_get_main_queue(),
                DISPATCH_DATA_DESTRUCTOR_DEFAULT);
            id<MTLLibrary> library = [mNativeDevice newLibraryWithData: data error: &error];
            //id<MTLLibrary> library = [mNativeDevice newLibraryWithURL: url error: &error];
            //id<MTLLibrary> library = [mNativeDevice newLibraryWithFile: libraryFilePathStr error: &error];
            
            if(error != nil)
            {
                NSLog(@"error => %@", error);
                assert(0);
            }
            
            WTFASSERT(library != nil, "Can'\t load library \"%s\"", metalPipelineDesc.mLibraryFilePath.c_str());
            
            printf("library: %s\n", metalPipelineDesc.mLibraryFilePath.c_str());
            
            NSString* vertexEntryStr = [NSString stringWithUTF8String: metalPipelineDesc.mVertexEntryName.c_str()];
            NSString* fragementEntryStr = [NSString stringWithUTF8String: metalPipelineDesc.mFragementEntryName.c_str()];
            
            // pipeline descriptor
            MTLRenderPipelineDescriptor* renderPipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
            renderPipelineDescriptor.vertexFunction = [library newFunctionWithName: vertexEntryStr];
            renderPipelineDescriptor.fragmentFunction = [library newFunctionWithName: fragementEntryStr];
            WTFASSERT(renderPipelineDescriptor.vertexFunction != nil, "Invalid vertex function name \"%s\"", metalPipelineDesc.mVertexEntryName.c_str());
            WTFASSERT(renderPipelineDescriptor.fragmentFunction != nil, "Invalid fragment function name \"%s\"", metalPipelineDesc.mFragementEntryName.c_str());
            
            // color attachment format
            for(uint32_t iColorAttachment = 0; iColorAttachment < metalPipelineDesc.miNumRenderTarget; iColorAttachment++)
            {
                if(metalPipelineDesc.mbOutputPresent)
                {
// temp temp temp
renderPipelineDescriptor.colorAttachments[iColorAttachment].pixelFormat = MTLPixelFormatBGRA8Unorm;
                }
                else
                {
                    RenderDriver::Common::Format const& colorAttachmentFormat = metalPipelineDesc.maRenderTargetFormats[iColorAttachment];
                    renderPipelineDescriptor.colorAttachments[iColorAttachment].pixelFormat = RenderDriver::Metal::Utils::convert(colorAttachmentFormat);
                }
            }
            
            // depth attachment format, no depth enabled or not swap chain pass ==> invalid pixel format for depth
            renderPipelineDescriptor.depthAttachmentPixelFormat =
                (desc.mDepthStencilState.mbDepthEnabled || desc.mbOutputPresent) ?
                RenderDriver::Metal::Utils::convert(metalPipelineDesc.mDepthStencilFormat) :
                MTLPixelFormatInvalid;
            
            // further info: https://github.com/KhronosGroup/SPIRV-Cross/issues/792
            // vertex descriptor, constant buffer index => 0, vertex buffer => 30
            
            MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor new];
            if(desc.mbFullTrianglePass || desc.mbOutputPresent)
            {
                vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
                vertexDescriptor.attributes[0].offset = 0;
                vertexDescriptor.attributes[0].bufferIndex = kiVertexBufferIndex;
                
                vertexDescriptor.attributes[1].format = MTLVertexFormatFloat3;
                vertexDescriptor.attributes[1].offset = sizeof(float) * 4 + sizeof(float) * 4;
                vertexDescriptor.attributes[1].bufferIndex = kiVertexBufferIndex;
            }
            else
            {
                vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
                vertexDescriptor.attributes[0].offset = 0;
                vertexDescriptor.attributes[0].bufferIndex = kiVertexBufferIndex;
                
                vertexDescriptor.attributes[1].format = MTLVertexFormatFloat3;
                vertexDescriptor.attributes[1].offset = sizeof(float) * 4;
                vertexDescriptor.attributes[1].bufferIndex = kiVertexBufferIndex;
                
                vertexDescriptor.attributes[2].format = MTLVertexFormatFloat3;
                vertexDescriptor.attributes[2].offset = sizeof(float) * 4 + sizeof(float) * 4;
                vertexDescriptor.attributes[2].bufferIndex = kiVertexBufferIndex;
            }
            
            vertexDescriptor.layouts[kiVertexBufferIndex].stride = sizeof(float) * 4 * 3;
            vertexDescriptor.layouts[kiVertexBufferIndex].stepRate = 1;
            vertexDescriptor.layouts[kiVertexBufferIndex].stepFunction = MTLVertexStepFunctionPerVertex;
            
            renderPipelineDescriptor.vertexDescriptor = vertexDescriptor;
            
            // create pipeline
            error = nil;
            MTLAutoreleasedRenderPipelineReflection reflection;
            mNativeRenderPipelineState = [mNativeDevice
                newRenderPipelineStateWithDescriptor: renderPipelineDescriptor 
                options: MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo
                reflection: &reflection
                error: &error];
            
            if(error != nil)
            {
                NSLog(@"error => %@", error);
                assert(0);
            }
            mNativeComputePipelineState = nil;
            
            // initial alooc render pass desciptor
            mNativeRenderPassDescriptor = [[MTLRenderPassDescriptor alloc] init];
            
            mNativeDepthStencilDescriptor = [[MTLDepthStencilDescriptor alloc] init];
            mNativeDepthStencilDescriptor.depthWriteEnabled = desc.mDepthStencilState.mbDepthEnabled;
            mNativeDepthStencilDescriptor.depthCompareFunction = (desc.mDepthStencilState.mbDepthEnabled) ? RenderDriver::Metal::Utils::convert(desc.mDepthStencilState.mDepthFunc) : RenderDriver::Metal::Utils::convert(RenderDriver::Common::ComparisonFunc::Always);
            
            mNativeDepthStencilState = [mNativeDevice newDepthStencilStateWithDescriptor: mNativeDepthStencilDescriptor];
            
            uint32_t iNumVertexVariables = 0;
            NSArray <MTLArgument *>* vsArgs = [reflection vertexArguments];
            for(MTLArgument* arg in vsArgs)
            {
                
                NSString* name = [arg name];
                MTLArgumentType type = [arg type];
                //NSLog(@"%@", arg);
                
                char const* szVariableName = [name UTF8String];
                char const* szVariableType = "buffer";
                if(type == MTLArgumentTypeTexture)
                {
                    szVariableType = "texture";
                }
                else if(type == MTLArgumentTypeSampler)
                {
                    szVariableType = "sampler";
                }
                
                DEBUG_PRINTF("\t%d argument name: %s type: %s\n",
                       iNumVertexVariables,
                       szVariableName,
                       szVariableType);
                
                // struct member, just list them as individual variable for now
                if(type == MTLArgumentTypeBuffer)
                {
                    MTLStructType* structType = [arg bufferStructType];
                    NSArray<MTLStructMember *>* pStructMembers = [structType members];
                    for(MTLStructMember* pMember in pStructMembers)
                    {
                        char const* szMemberName = [[pMember name] UTF8String];
                        DEBUG_PRINTF("\t\tmember: %s\n", szMemberName);
                    }
                }
                
                maVertexShaderResourceReflectionInfo.emplace_back(
                    szVariableName,
                    type,
                    RenderDriver::Common::ShaderType::Vertex);
                
                ++iNumVertexVariables;
            }
            
            uint32_t iNumFragmentVariables = 0;
            NSArray <MTLArgument *>* fsArgs = [reflection fragmentArguments];
            for(MTLArgument* arg in fsArgs)
            {
                
                NSString* name = [arg name];
                MTLArgumentType type = [arg type];
                //NSLog(@"%@", arg);
                
                char const* szVariableName = [name UTF8String];
                
                char const* szVariableType = "buffer";
                if(type == MTLArgumentTypeTexture)
                {
                    szVariableType = "texture";
                }
                else if(type == MTLArgumentTypeSampler)
                {
                    szVariableType = "sampler";
                }
                
                DEBUG_PRINTF("\t%d argument name: %s type: %s\n",
                       iNumFragmentVariables,
                       szVariableName,
                       szVariableType);
                
                // struct member, just list them as individual variable for now
                if(type == MTLArgumentTypeBuffer)
                {
                    MTLStructType* structType = [arg bufferStructType];
                    NSArray<MTLStructMember *>* pStructMembers = [structType members];
                    for(MTLStructMember* pMember in pStructMembers)
                    {
                        char const* szMemberName = [[pMember name] UTF8String];
                        DEBUG_PRINTF("\t\tmember: %s\n", szMemberName);
                    }
                }
                
                maFragmentShaderResourceReflectionInfo.emplace_back(
                    szVariableName,
                    type,
                    RenderDriver::Common::ShaderType::Fragment);
                
                ++iNumFragmentVariables;
            }
            
            // fillout shader resource indices look for layout indices
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const* paShaderResources = desc.mpDescriptor->getDesc().mpaShaderResources;
            if(paShaderResources)
            {
                // vertex shader layout mapping for shader resources
                std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = *paShaderResources;
                setLayoutMapping(
                    maiVertexShaderResourceLayoutIndices,
                    maVertexShaderResourceReflectionInfo,
                    aShaderResources);
                
                setLayoutMapping(
                    maiFragmentShaderResourceLayoutIndices,
                    maFragmentShaderResourceReflectionInfo,
                    aShaderResources);
                
                desc.mpDescriptor->setVertexLayoutIndices(maiVertexShaderResourceLayoutIndices);
                desc.mpDescriptor->setFragmentLayoutIndices(maiFragmentShaderResourceLayoutIndices);
            }
            
            return mHandle;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPipelineState::create(
            RenderDriver::Common::ComputePipelineStateDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CPipelineState::create(desc, device);

            RenderDriver::Metal::CPipelineState::ComputePipelineStateDescriptor const& metalPipelineDesc =
                static_cast<RenderDriver::Metal::CPipelineState::ComputePipelineStateDescriptor const&>(desc);

            RenderDriver::Metal::CDevice& deviceMetal = static_cast<RenderDriver::Metal::CDevice&>(device);
            mNativeDevice = (__bridge id<MTLDevice>)deviceMetal.getNativeDevice();
            
            // load metallib binary data
            std::string filePath = std::string(getSaveDir()) + "/" + metalPipelineDesc.mLibraryFilePath.c_str();
            FILE* fp = fopen(filePath.c_str(), "rb");
            WTFASSERT(fp, "can\'t open file \"%s\"", metalPipelineDesc.mLibraryFilePath.c_str());
            fseek(fp, 0, SEEK_END);
            uint64_t iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<char> acFileContent(iFileSize);
            fread(acFileContent.data(), sizeof(char), iFileSize, fp);
            fclose(fp);
            
            // shader library
            dispatch_data_t data = dispatch_data_create(
                acFileContent.data(),
                iFileSize,
                dispatch_get_main_queue(),
                DISPATCH_DATA_DESTRUCTOR_DEFAULT);
            NSError* error = nil;
            id<MTLLibrary> library = [mNativeDevice newLibraryWithData: data error: &error];
            if(error != nil)
            {
                NSLog(@"error => %@", error);
            }
            
            NSString* computeEntryStr = [NSString stringWithUTF8String: metalPipelineDesc.mComputeEntryName.c_str()];
            
            MTLAutoreleasedComputePipelineReflection reflection;
            
            // create pipeline with function
            id<MTLFunction> computeFunction = [library newFunctionWithName: computeEntryStr];
            mNativeComputePipelineState = [mNativeDevice
                 newComputePipelineStateWithFunction: computeFunction
                 options: MTLPipelineOptionArgumentInfo | MTLPipelineOptionBufferTypeInfo
                 reflection: &reflection
                 error: &error];
            mNativeRenderPipelineState = nil;
            
            if(error != nil)
            {
                NSLog(@"error => %@", error);
            }
            
            //NSArray<id<MTLBinding>>* bindings = [reflection bindings];
            
            std::vector<SerializeUtils::Common::ShaderResourceInfo> aShaderResources;
            
            // shader resource layout reflection
            uint32_t iNumComputeVariables = 0;
            NSArray <MTLArgument *>* csArgs = [reflection arguments];
            for(MTLArgument* arg in csArgs)
            //for(id<MTLBinding> arg in bindings)
            {
                SerializeUtils::Common::ShaderResourceInfo shaderResourceInfo;
                
                NSString* name = [arg name];
                MTLArgumentType type = [arg type];
                //NSLog(@"%@", arg);
                
                //MTLBindingType type = [arg type];
                
                char const* szVariableName = [name UTF8String];
                char const* szVariableType = "buffer";
                if(type == MTLArgumentTypeTexture)
                {
                    szVariableType = "texture";
                }
                else if(type == MTLArgumentTypeSampler)
                {
                    szVariableType = "sampler";
                }
                
                DEBUG_PRINTF("\t%d argument name: %s type: %s\n",
                       iNumComputeVariables,
                       szVariableName,
                       szVariableType);

                shaderResourceInfo.mName = szVariableName;
                shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN;
                if(std::string(szVariableType) == "buffer")
                {
                    shaderResourceInfo.mType = ShaderResourceType::RESOURCE_TYPE_BUFFER_IN;
                }
                aShaderResources.push_back(shaderResourceInfo);
                

                // struct member, just list them as individual variable for now
                if(type == MTLArgumentTypeBuffer)
                {
                    MTLStructType* structType = [arg bufferStructType];
                    NSArray<MTLStructMember *>* pStructMembers = [structType members];
                    for(MTLStructMember* pMember in pStructMembers)
                    {
                        char const* szMemberName = [[pMember name] UTF8String];
                        DEBUG_PRINTF("\t\tmember: %s\n", szMemberName);
                    }
                }
    
                //ShaderResourceReflectionInfo reflectionInfo(szVariableName, type, RenderDriver::Common::ShaderType::Compute);
                //reflectionInfo.mName = szVariableName;
                //reflectionInfo.mType = type;
                //reflectionInfo.mShaderType = RenderDriver::Common::ShaderType::Compute;
                //maComputeShaderResourceReflectionInfo.push_back(reflectionInfo);
                
                maComputeShaderResourceReflectionInfo.emplace_back(
                                                                   szVariableName,
                                                                   type,
                                                                   RenderDriver::Common::ShaderType::Compute
                );
                
                ++iNumComputeVariables;
            }
            
            // fillout shader resource indices look for layout indices
            if(desc.mpDescriptor->getDesc().mpaShaderResources)
            {
                aShaderResources = *desc.mpDescriptor->getDesc().mpaShaderResources;
            }
            
            setLayoutMapping(
                maiComputeShaderResourceLayoutIndices,
                maComputeShaderResourceReflectionInfo,
                aShaderResources);

            desc.mpDescriptor->setComputeLayoutIndices(maiComputeShaderResourceLayoutIndices);
            
            return mHandle;
        }

        /*
        **
        */
        void CPipelineState::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            
            
        }

        /*
        **
        */
        void* CPipelineState::getNativePipelineState()
        {
            void* pRet = (mNativeRenderPipelineState == nil) ? (__bridge void*)mNativeComputePipelineState : (__bridge void*)mNativeRenderPipelineState;
            return pRet;
        }

        /*
        **
        */
        void CPipelineState::setLayoutMapping(
            std::vector<uint32_t>& aiShaderResourceLayoutIndices,
            std::vector<ShaderResourceReflectionInfo>& aReflectionInfo,
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources)
        {
            uint32_t iValidLayoutIndex = 0;
            for(uint32_t i = 0; i < static_cast<uint32_t>(aReflectionInfo.size()); i++)
            {
                // look for the shader resource with the reflection name
                std::string const& reflectionShaderResourceName = aReflectionInfo[i].mName;
                if(reflectionShaderResourceName.find("vertexBuffer.") != std::string::npos ||
                   reflectionShaderResourceName == "linearSampler" ||
                   reflectionShaderResourceName == "pointSampler")
                {
                    continue;
                }
                
                auto iter = std::find_if(
                     aShaderResources.begin(),
                     aShaderResources.end(),
                     [reflectionShaderResourceName](SerializeUtils::Common::ShaderResourceInfo const& shaderResource)
                     {
                         std::string fillerName = std::string("(") + reflectionShaderResourceName + ")";
                         return (
                             shaderResource.mShaderResourceName == reflectionShaderResourceName ||
                             shaderResource.mName.find(fillerName) != std::string::npos ||
                             (reflectionShaderResourceName == "_Globals" && shaderResource.mName.find("$Globals") != std::string::npos)
                        );
                     });
                
                // set shader resource lookup index
                WTFASSERT(iter != aShaderResources.end(), "Can\'t find shader resource \"%s\"", reflectionShaderResourceName.c_str());
                uint32_t iShaderResourceIndex = static_cast<uint32_t>(std::distance(aShaderResources.begin(), iter));
                aiShaderResourceLayoutIndices.push_back(iShaderResourceIndex);
                ++iValidLayoutIndex;
                
                DEBUG_PRINTF("\tlayout %d %s (%d)\n",
                             i,
                             reflectionShaderResourceName.c_str(),
                             iShaderResourceIndex);
            }
        }
    

    }   // Metal

} // RenderDriver
