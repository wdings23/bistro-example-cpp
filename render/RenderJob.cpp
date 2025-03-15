#include <render-driver/RayTrace.h>
#include <render/RenderJob.h>
#include <rapidjson/document.h>

#include <render-driver/Image.h>
#include <render-driver/ImageView.h>

#include <render-driver/Utils.h>

#include <utils/wtfassert.h>
#include <utils/LogPrint.h>

#include <stb_image.h>

#include <sstream>

#if defined(_MSC_VER)
#undef max
#endif // _MSC_VER

namespace Render
{
    namespace Common
    {
        CRenderJob::CRenderJob() {}
        CRenderJob::~CRenderJob() {}

        /*
        **
        */
        void CRenderJob::create(CreateInfo& createInfo)
        {
            miWaitSemaphoreValue = 0u;
            miSignalSemaphoreValue = 0u;

            mbDrawEnabled = true;
            mViewPort[0] = 0; 
            mViewPort[1] = 0;
            mViewPort[2] = createInfo.miScreenWidth;
            mViewPort[3] = createInfo.miScreenHeight;

            mpDevice = createInfo.mpDevice;
            mpDefaultUniformBuffer = createInfo.mpDefaultUniformBuffer;

            maiDispatchSize[0] = createInfo.mDispatchSize.x;
            maiDispatchSize[1] = createInfo.mDispatchSize.y;
            maiDispatchSize[2] = createInfo.mDispatchSize.z;

            rapidjson::Document doc;
            {
                FILE* fp = fopen(createInfo.mFilePath.c_str(), "rb");
                fseek(fp, 0, SEEK_END);
                size_t iFileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                std::vector<char> acBuffer(iFileSize + 1);
                acBuffer[iFileSize] = 0;
                fread(acBuffer.data(), sizeof(char), iFileSize, fp);
                fclose(fp);
                doc.Parse(acBuffer.data());
            }

            if(createInfo.mpapAccelerationStructures)
            {
                for(auto const& keyValue : *createInfo.mpapAccelerationStructures)
                {
                    mapAccelerationStructures[keyValue.first] = keyValue.second;
                }
            }
            
            mName = createInfo.mName;
            
            std::string typeStr = doc["Type"].GetString();
            std::string passStr = doc["PassType"].GetString();

            if(typeStr == "Compute")
            {
                mType = JobType::Compute;
            }
            else if(typeStr == "Graphics")
            {
                mType = JobType::Graphics;
            }
            else if(typeStr == "Copy")
            {
                mType = JobType::Copy;
            }
            else if(typeStr == "Ray Trace")
            {
                mType = JobType::RayTrace;
            }
            
            if(passStr == "Compute")
            {
                mPassType = Render::Common::PassType::Compute;
            }
            else if(passStr == "Draw Meshes")
            {
                mPassType = Render::Common::PassType::DrawMeshes;
            }
            else if(passStr == "Full Triangle")
            {
                mPassType = Render::Common::PassType::FullTriangle;
            }
            else if(passStr == "Draw MeshClusters")
            {
                mPassType = Render::Common::PassType::DrawMeshClusters;
            }
            else if(passStr == "Copy")
            {
                mPassType = Render::Common::PassType::Copy;
            }
            else if(passStr == "Swap Chain")
            {
                mPassType = Render::Common::PassType::SwapChain;
            }
            else if(passStr == "Imgui")
            {
                mPassType = Render::Common::PassType::Imgui;
            }
            else if(passStr == "Ray Trace")
            {
                mPassType = Render::Common::PassType::RayTrace;
            }
            else if(passStr == "Depth Prepass")
            {
                mPassType = Render::Common::PassType::DepthPrepass;
            }
       
            if(mType != JobType::Copy)
            {
                createAttachments(
                    doc,
                    createInfo.miScreenWidth,
                    createInfo.miScreenHeight,
                    createInfo.mpaRenderJobs,
                    createInfo.mpExtraAttachmentJSONInfo
                );

                createPipelineData(
                    doc,
                    createInfo.mpaRenderJobs,
                    createInfo.mpGraphicsCommandQueue,
                    createInfo.mpacConstantBufferData
                );

                initPipelineLayout(
                    createInfo
                );

                std::string shaderDirectory = createInfo.mTopDirectory;
                initPipelineState(
                    doc,
                    createInfo.mpaRenderJobs,
                    createInfo.miScreenWidth,
                    createInfo.miScreenHeight,
                    shaderDirectory,
                    createInfo.mFilePath,
                    createInfo.mpPlatformInstance
                );

                if(mType == JobType::Graphics)
                {
                    createFrameBuffer(
                        createInfo.mpSwapChain
                    );
                }

                computeSemaphoreValues(
                    createInfo
                );
            }
            else
            {
                // set input attachments for copy
                for(auto& copyAttachment : maCopyAttachmentMapping)
                {
                    std::string const& parentJobName = copyAttachment.second.second;
                    std::string const& parentName = copyAttachment.second.first;
                    for(auto& pJob : *createInfo.mpaRenderJobs)
                    {
                        if(pJob->mName == parentJobName)
                        {
                            WTFASSERT(pJob->mapOutputImageAttachments.count(parentName) > 0,
                                "In \"%s\", Can't find \"%s\" from parent job \"%s\"",
                                mName.c_str(),
                                parentName.c_str(),
                                parentJobName.c_str()
                            );
                            std::string newAttachmentName = parentJobName + "-" + parentName;
                            mapInputImageAttachments[newAttachmentName] = pJob->mapOutputImageAttachments[parentName];
                            maAttachmentInfo[newAttachmentName]["ParentJobName"] = parentJobName;

                            break;
                        }
                    }
                }

                platformCreateSemaphore();

                computeSemaphoreValues(
                    createInfo
                );
            }

            initAttachmentBarriers(createInfo);
        }

        /*
        **
        */
        void CRenderJob::createFrameBuffer(
            RenderDriver::Common::CSwapChain* pSwapChain
        )
        {
            platformCreateFrameBuffer(
                pSwapChain
            );
        }

        /*
        **
        */
        void CRenderJob::createOutputAttachments(CreateInfo const& createInfo)
        {
            mpDevice = createInfo.mpDevice;

            rapidjson::Document doc;
            {
                FILE* fp = fopen(createInfo.mFilePath.c_str(), "rb");
                fseek(fp, 0, SEEK_END);
                size_t iFileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                std::vector<char> acBuffer(iFileSize + 1);
                acBuffer[iFileSize] = 0;
                fread(acBuffer.data(), sizeof(char), iFileSize, fp);
                fclose(fp);
                doc.Parse(acBuffer.data());
            }

            uint32_t iScreenWidth = createInfo.miScreenWidth;
            uint32_t iScreenHeight = createInfo.miScreenHeight;

            uint32_t iOutputWidth = iScreenWidth;
            uint32_t iOutputHeight = iScreenHeight;

            bool bHasDepthAttachment = false;

            std::string jobType = doc["Type"].GetString();

            auto jsonAttachments = doc["Attachments"].GetArray();
            for(uint32_t i = 0; i < (uint32_t)jsonAttachments.Size(); i++)
            {
                auto const& attachmentJSON = jsonAttachments[i];

                std::string name = attachmentJSON["Name"].GetString();
                std::string type = attachmentJSON["Type"].GetString();

                maAttachmentInfo[name]["Type"] = type;

                // image width, height
                uint32_t iImageWidth = iScreenWidth;
                float fAttachmentScaleWidth = 1.0f;
                if(attachmentJSON.HasMember("ScaleWidth"))
                {
                    fAttachmentScaleWidth = attachmentJSON["ScaleWidth"].GetFloat();
                    iImageWidth = (uint32_t)((float)iImageWidth * fAttachmentScaleWidth);
                }

                uint32_t iImageHeight = iScreenHeight;
                float fAttachmentScaleHeight = 1.0f;
                if(attachmentJSON.HasMember("ScaleHeight"))
                {
                    fAttachmentScaleHeight = attachmentJSON["ScaleHeight"].GetFloat();
                    iImageHeight = (uint32_t)((float)iImageHeight * fAttachmentScaleHeight);
                }

                if(attachmentJSON.HasMember("ImageWidth"))
                {
                    iImageWidth = attachmentJSON["ImageWidth"].GetInt();
                }

                if(attachmentJSON.HasMember("ImageHeight"))
                {
                    iImageHeight = attachmentJSON["ImageHeight"].GetInt();
                }

                RenderDriver::Common::Format attachmentFormat = RenderDriver::Common::Format::R8G8B8A8_UNORM;
                if(type == "TextureOutput")
                {
                    handleTextureOutput(
                        attachmentJSON,
                        iImageWidth,
                        iImageHeight
                    );

                    mapOutputImageAttachments[name] = mapImageAttachments[name];
                    mapOutputImageAttachmentViews[name] = mapImageAttachmentViews[name];

                    mapOutputBufferAttachments[name] = nullptr;
                    mapOutputBufferAttachmentViews[name] = nullptr;

                    iOutputWidth = iImageWidth;
                    iOutputHeight = iImageHeight;

                    if(jobType == "Copy")
                    {
                        std::string parentJobName = attachmentJSON["ParentJobName"].GetString();
                        std::string parentName = attachmentJSON["ParentName"].GetString();
                        maCopyAttachmentMapping.push_back(
                            std::make_pair(name, std::make_pair(parentName, parentJobName))
                        );
                    }

                }   // if TextureOutput
                else if(type == "BufferOutput")
                {
                    handleBufferOutput(
                        attachmentJSON
                    );

                    mapOutputImageAttachments[name] = nullptr;
                    mapOutputImageAttachmentViews[name] = nullptr;

                    mapOutputBufferAttachments[name] = mapBufferAttachments[name];
                    mapOutputBufferAttachmentViews[name] = mapBufferAttachmentViews[name];

                }   // if BufferOutput
                else if(type == "TextureInputOutput")
                {
                    // check for depth attachment, don't create if has one
                    std::string attachmentName = attachmentJSON["Name"].GetString();
                    std::string attachmentType = attachmentJSON["Type"].GetString();
                    if(attachmentName == "Depth Output" && attachmentType == "TextureInputOutput")
                    {
                        bHasDepthAttachment = true;
                    }
                }
            }
            
            // depth attachment if needed
            // create depth texture if depth stencil%
            if(doc.HasMember("DepthStencilState"))
            {
                RenderDriver::Common::DepthStencilState depthStencilState = SerializeUtils::Common::getDepthStencilState(doc);
                if(depthStencilState.mbDepthEnabled && !bHasDepthAttachment)
                {
                    if(mpDepthImage == nullptr)
                    {
                        platformCreateDepthImage(
                            iOutputWidth,
                            iOutputHeight
                        );
                    }
                }
            }

            // view port scale
            mViewportScale.x = (float)iOutputWidth / (float)iScreenWidth;
            mViewportScale.y = (float)iOutputHeight / (float)iScreenHeight;
        }

        /*
        **
        */
        void CRenderJob::createPipelineData(
            rapidjson::Document const& doc,
            std::vector<CRenderJob*>* apRenderJobs,
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            std::vector<char>* pacConstantBufferData
        )
        {
            auto aShaderResources = doc["ShaderResources"].GetArray();
            uint32_t iIndex = 0;
            for(auto const& shaderResource : aShaderResources)
            {
                std::string type = shaderResource["type"].GetString();
                std::string name = shaderResource["name"].GetString();

                // save info
                maShaderResourceInfo[name]["type"] = shaderResource["type"].GetString();
                maShaderResourceInfo[name]["usage"] = shaderResource["usage"].GetString();
                maShaderResourceInfo[name]["shader_stage"] = shaderResource["shader_stage"].GetString();
                
                if(shaderResource.HasMember("size"))
                {
                    std::ostringstream oss;
                    oss << shaderResource["size"].GetInt();
                    maShaderResourceInfo[name]["size"] = oss.str().c_str();
                }
                if(shaderResource.HasMember("parent_job"))
                {
                    maShaderResourceInfo[name]["parent_job"] = shaderResource["parent_job"].GetString();
                }

                if(type == "buffer")
                {
                    handleUniformBuffer(
                        shaderResource, 
                        apRenderJobs
                    );

                    if(maShaderResourceInfo[name]["usage"] == "uniform")
                    {
                        maUniformMappings.push_back(std::make_pair(name, "buffer-input"));
                    }
                    else if(maShaderResourceInfo[name]["usage"] == "read_write_storage")
                    {
                        maUniformMappings.push_back(std::make_pair(name, "buffer-output"));
                    }
                    else
                    {
                        WTFASSERT(0, "this case is not handled: %s", maShaderResourceInfo[name]["usage"].c_str());
                    }

                }  
                else if(type == "texture2d")
                {
                    handleUniformTexture(
                        shaderResource,
                        apRenderJobs,
                        pCommandQueue
                    );

                    maUniformMappings.push_back(std::make_pair(name, "texture-input"));
                }
                else
                {
                    WTFASSERT(0, "type not handled");
                }

                ++iIndex;

            }   // for shader resource in shader resources

            if(pacConstantBufferData != nullptr)
            {
                uint32_t iBufferSize = std::max((uint32_t)pacConstantBufferData->size(), 64u);
                RenderDriver::Common::BufferUsage usage = RenderDriver::Common::BufferUsage(
                    uint32_t(RenderDriver::Common::BufferUsage::UniformBuffer) |
                    uint32_t(RenderDriver::Common::BufferUsage::UniformTexel) |
                    uint32_t(RenderDriver::Common::BufferUsage::TransferDest)
                );

                RenderDriver::Common::BufferDescriptor bufferDesc =
                {
                    iBufferSize,
                    RenderDriver::Common::Format::R32_FLOAT,
                    RenderDriver::Common::ResourceFlagBits::None,
                    RenderDriver::Common::HeapType::Default,
                    RenderDriver::Common::ResourceStateFlagBits::None,
                    nullptr,
                    0,
                    0,
                    RenderDriver::Common::Format::R32_FLOAT,
                    usage
                };
                
                std::string name = mName + " Constant Buffer";
                mapUniformBuffers[name] = platformCreateBuffer(name, bufferDesc);
                platformUploadDataToBuffer(
                    *mapUniformBuffers[name], 
                    pacConstantBufferData->data(), 
                    (uint32_t)pacConstantBufferData->size(),
                    *pCommandQueue);

                maShaderResourceInfo[name]["usage"] = "uniform";
                maUniformMappings.push_back(std::make_pair(name, "buffer-input"));
            }
            
#if !defined(_MSC_VER)
            // default uniform buffer at the very end of the descriptor
            maUniformMappings.push_back(std::make_pair("Default Uniform Buffer", "buffer-input"));
#endif // METAL
        }

        /*
        **
        */
        void CRenderJob::initPipelineLayout(
            CreateInfo const& createInfo
        )
        {
            platformInitDescriptorSet();
            mpDescriptorSet->setID(mName + " Descriptor Set");

            PrintOptions opt = {false};
            DEBUG_PRINTF_SET_OPTIONS(opt);

            DEBUG_PRINTF("render job: \"%s\"\n", mName.c_str());

            // initialize any external buffers
            if(createInfo.mpfnInitExternalDataFunc)
            {
                (*createInfo.mpfnInitExternalDataFunc)(this);
            }

            // attachments at set 0
            uint32_t iBinding = 0;
            for(uint32_t i = 0; i < (uint32_t)maAttachmentMappings.size(); i++)
            {
                std::string name = maAttachmentMappings[i].first;
                if(maAttachmentMappings[i].second == "texture-input" || 
                   maAttachmentMappings[i].second == "texture-input-output")
                {
                    RenderDriver::Common::CImage* pImage = mapImageAttachments[name];
                    RenderDriver::Common::CImageView* pImageView = mapImageAttachmentViews[name];
                    bool bReadOnly = (maAttachmentMappings[i].second == "texture-input");
                    if(bReadOnly)
                    {
                        mpDescriptorSet->addImage(
                            pImage,
                            pImageView,
                            iBinding,
                            0,
                            bReadOnly);

                        DEBUG_PRINTF("\ttexture \"%s\" set %d binding %d",
                            name.c_str(),
                            0,
                            iBinding);
                        if(bReadOnly)
                            DEBUG_PRINTF(" read-only\n");
                        else
                            DEBUG_PRINTF("\n");
                    
                        ++iBinding;
                    }
                }
                else if(maAttachmentMappings[i].second == "buffer-input" || 
                    maAttachmentMappings[i].second == "buffer-output" || 
                    maAttachmentMappings[i].second == "buffer-input-output")
                {
                    RenderDriver::Common::CBuffer* pBuffer = mapBufferAttachments[name];
                    bool bReadOnly = (maAttachmentMappings[i].second == "buffer-input");
                    mpDescriptorSet->addBuffer(
                        pBuffer, 
                        iBinding,
                        0,
                        bReadOnly);

                    DEBUG_PRINTF("\tbuffer \"%s\" set %d binding %d",
                        name.c_str(),
                        0,
                        iBinding);
                    if(bReadOnly)
                        DEBUG_PRINTF(" read-only\n");
                    else
                        DEBUG_PRINTF("\n");

                    ++iBinding;
                }
                else if(maAttachmentMappings[i].second == "acceleration-structure")
                {
                    RenderDriver::Common::CAccelerationStructure* pAccelerationStructure = (*createInfo.mpapAccelerationStructures)["bistro"];
                    mpDescriptorSet->addAccelerationStructure(
                        pAccelerationStructure,
                        iBinding,
                        0);

                    DEBUG_PRINTF("\tacceleration structure \"%s\" set %d binding %d\n",
                        name.c_str(),
                        0,
                        iBinding);

                    ++iBinding;
                }
                else if(maAttachmentMappings[i].second == "texture-output")
                {
                    if(mType == JobType::RayTrace || mType == JobType::Compute)
                    {
                        RenderDriver::Common::CImage* pImage = mapImageAttachments[name];
                        RenderDriver::Common::CImageView* pImageView = mapImageAttachmentViews[name];
                        mpDescriptorSet->addImage(
                            pImage,
                            pImageView,
                            iBinding,
                            0,
                            false
                        );

                        DEBUG_PRINTF("\ttexture \"%s\" set %d binding %d\n",
                            name.c_str(),
                            0,
                            iBinding);

                        ++iBinding;
                    }
                }
            }

            // shader resource at set 1
            iBinding = 0;
            for(uint32_t i = 0; i < (uint32_t)maUniformMappings.size(); i++)
            {
                std::string name = maUniformMappings[i].first;
                if(maUniformMappings[i].second == "texture-input" || maUniformMappings[i].second == "texture")
                {
                    RenderDriver::Common::CImage* pImage = mapUniformImages[name];
                    bool bReadOnly = (maShaderResourceInfo[name]["usage"] == "uniform" || maShaderResourceInfo[name]["usage"] == "read_only_storage");
                    if(bReadOnly)
                    {
                        mpDescriptorSet->addImage(
                            pImage,
                            mapUniformImageViews[name],
                            iBinding,
                            1,
                            bReadOnly);

                        DEBUG_PRINTF("\ttexture \"%s\" set %d binding %d",
                            name.c_str(),
                            1,
                            i);
                        if(bReadOnly)
                            DEBUG_PRINTF(" read-only\n");
                        else
                            DEBUG_PRINTF("\n");

                        ++iBinding;
                    }
                }
                else if(maUniformMappings[i].second == "buffer-input" || maUniformMappings[i].second == "buffer-output" || maUniformMappings[i].second == "buffer")
                {
                    RenderDriver::Common::CBuffer* pBuffer = mapUniformBuffers[name];
                    bool bReadOnly = (maShaderResourceInfo[name]["usage"] == "uniform" || maShaderResourceInfo[name]["usage"] == "read_only_storage");
                    mpDescriptorSet->addBuffer(
                        pBuffer, 
                        iBinding,
                        1,
                        bReadOnly);

                    DEBUG_PRINTF("\tbuffer \"%s\" set %d binding %d",
                        name.c_str(),
                        1,
                        i);
                    if(bReadOnly)
                        DEBUG_PRINTF(" read-only\n");
                    else
                        DEBUG_PRINTF("\n");

                    ++iBinding;
                }
                else
                {
                    WTFASSERT(0, "not handled");
                }
            }

#if defined(_MSC_VER)
            mpDescriptorSet->addBuffer(
                mpDefaultUniformBuffer,
                iBinding,
                1,
                true
            );
            DEBUG_PRINTF("\tbuffer \"%s\" set %d binding %d read-only\n",
                mpDefaultUniformBuffer->getID().c_str(),
                1,
                iBinding);
#endif // _MSC_VER

            // create the descriptor set and update the buffers
            mpDescriptorSet->finishLayout(
                createInfo.maSamplers
            );

            mpDescriptorSet->setID(mName + " Descriptor Set");
            
            DEBUG_PRINTF("\n");

            opt.mbDisplayTime = true;
            DEBUG_PRINTF_SET_OPTIONS(opt);
        }

        /*
        **
        */
        void CRenderJob::initPipelineState(
            rapidjson::Document const& doc,
            std::vector<CRenderJob*>* apRenderJobs,
            uint32_t iWidth,
            uint32_t iHeight,
            std::string const& shaderDirectory,
            std::string const& pipelineFilePath,
            void* pPlatformInstance
        )
        {
            auto baseStart = pipelineFilePath.find_last_of("/");
            if(baseStart == std::string::npos)
            {
                baseStart = pipelineFilePath.find_last_of("\\");
            }
            std::string pipelineFileName = pipelineFilePath.substr(baseStart + 1);
            auto pipelineFileExtensionStart = pipelineFileName.rfind(".");
            std::string pipelineBaseName = pipelineFileName.substr(0, pipelineFileExtensionStart);
            
            if(mType == JobType::Graphics)
            {
                platformInitPipelineState();

                // depth and raster states
                RenderDriver::Common::DepthStencilState depthStencilState = SerializeUtils::Common::getDepthStencilState(doc);
                RenderDriver::Common::RasterState rasterState = SerializeUtils::Common::getRasterState(doc);
                
                // blend stats
                std::vector<RenderDriver::Common::BlendState> aBlendStates;
                SerializeUtils::Common::getBlendState(
                    aBlendStates,
                    doc
                );

                // vertex format
                std::vector<RenderDriver::Common::VertexFormat> aInputFormat;
                SerializeUtils::Common::getVertexFormat(aInputFormat, doc);

                // initialize creation descriptor for current platform
                RenderDriver::Common::GraphicsPipelineStateDescriptor* pDesc = platformInitGraphicsPipelineStateDescriptor();

                // set from above
                pDesc->mpDescriptor = mpDescriptorSet;
                pDesc->mRasterState = rasterState;
                pDesc->mDepthStencilState = depthStencilState;
                pDesc->maInputFormat = aInputFormat;
                pDesc->maVertexFormats = aInputFormat.data();
                
                // formats for output attachments
                std::vector<RenderDriver::Common::Format> aAttachmentFormats;
                uint32_t iIndex = 0;
                bool bHasDepthInput = false;
                for(auto const& keyValue : maAttachmentMappings)
                {
                    if(keyValue.second == "texture-output" || keyValue.second == "texture-input-output")
                    {
                        aAttachmentFormats.push_back(maAttachmentFormats[keyValue.first]);
                        
                        if(keyValue.second == "texture-input-output")
                        {
                            pDesc->maLoadOps[iIndex] = RenderDriver::Common::LoadOp::Load;
                        }
                        else
                        {
                            RenderDriver::Common::LoadOp loadOp = RenderDriver::Common::LoadOp::Clear;
                            if(maAttachmentLoadOps.count(keyValue.first) > 0)
                            {
                                if(maAttachmentLoadOps[keyValue.first] == LoadStoreOp::Load)
                                {
                                    loadOp = RenderDriver::Common::LoadOp::Load;
                                }
                            }

                            pDesc->maLoadOps[iIndex] = RenderDriver::Common::LoadOp::Clear;
                        }

                        ++iIndex;
                    }
                }

                if(mbInputOutputDepth)
                {
                    pDesc->maLoadOps[iIndex] = RenderDriver::Common::LoadOp::Load;
                }

                for(uint32_t i = 0; i < aAttachmentFormats.size(); i++)
                {
                    pDesc->maRenderTargetBlendStates[i] = aBlendStates[0];
                    pDesc->maRenderTargetFormats[i] = aAttachmentFormats[i];
                }

                // get the number of valid output image attachments
                uint32_t iNumRenderTargets = 0;
                for(auto const& keyValue : mapOutputImageAttachments)
                {
                    if(keyValue.second != nullptr)
                    {
                        ++iNumRenderTargets;
                    }
                }

                pDesc->miNumRenderTarget = (bHasDepthInput) ? iNumRenderTargets - 1 : iNumRenderTargets;
                pDesc->miNumVertexMembers = (uint32_t)aInputFormat.size();
                pDesc->mSample.miCount = 1;
                pDesc->mSample.miQuality = 0;
                pDesc->mDepthStencilFormat = RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT;
                pDesc->miFlags = 0;
                pDesc->maVertexFormats = pDesc->maInputFormat.data();
                pDesc->mbOutputPresent = (mPassType == Render::Common::PassType::SwapChain) ? true : false;
                pDesc->mbFullTrianglePass = (mPassType == Render::Common::PassType::FullTriangle || mPassType == Render::Common::PassType::SwapChain) ? true : false;
                
                std::string shaderPath = doc["Shader"].GetString();

                std::string fullPath = shaderDirectory + "\\shaders\\" + shaderPath;
                std::vector<char> acShaderBufferVS;
                std::vector<char> acShaderBufferFS;
                pDesc = platformFillOutGraphicsPipelineDescriptor(
                    pDesc,
                    acShaderBufferVS,
                    acShaderBufferFS,
                    fullPath,
                    pipelineBaseName
                );

                pDesc->miPixelShaderSize = (uint32_t)acShaderBufferFS.size();
                pDesc->mpPixelShader = (uint8_t*)acShaderBufferFS.data();
                pDesc->miVertexShaderSize = (uint32_t)acShaderBufferVS.size();
                pDesc->mpVertexShader = (uint8_t*)acShaderBufferVS.data();

                // no need for pixel shader in depth pre-pass
                if(mPassType == PassType::DepthPrepass)
                {
                    pDesc->mpPixelShader = nullptr;
                }
              
                // TODO: use output attachments number
DEBUG_PRINTF("render job: %s\n", mName.c_str());
                mpPipelineState->create(
                    *pDesc,
                    *mpDevice
                );

            }   // if type == graphics
            else if(mType == JobType::Compute)
            {
                platformInitPipelineState();

                // initialize creation descriptor for current platform
                RenderDriver::Common::ComputePipelineStateDescriptor* pDesc = platformInitComputePipelineStateDescriptor();
                
                std::string shaderPath = doc["Shader"].GetString();

                // fill creation descriptor
                std::string fullPath = shaderDirectory + "\\shaders\\" + shaderPath;
                std::vector<char> acShaderBuffer;
                pDesc = platformFillOutComputePipelineDescriptor(
                    pDesc,
                    acShaderBuffer,
                    fullPath,
                    pipelineBaseName
                );

                // create compute pipeline
                mpPipelineState->create(
                    *pDesc, 
                    *mpDevice
                );

            }   // else if type == compute
            else if(mType == JobType::RayTrace)
            {
                platformInitPipelineState();

                RenderDriver::Common::RayTracePipelineStateDescriptor* pDesc = platformInitRayTracePipelineStateDescriptor();

                std::string shaderPath = doc["Shader"].GetString();
                std::string fullPath = shaderDirectory + "\\shaders\\" + shaderPath;
                std::vector<char> acRayGenShaderBuffer;
                std::vector<char> acCloseHitShaderBuffer;
                std::vector<char> acMissShaderBuffer;
                pDesc = platformFillOutRayTracePipelineDescriptor(
                    pDesc,
                    acRayGenShaderBuffer,
                    acCloseHitShaderBuffer,
                    acMissShaderBuffer,
                    fullPath,
                    pipelineBaseName,
                    pPlatformInstance
                );
                pDesc->mpDescriptor = mpDescriptorSet;
                
                mpPipelineState->create(
                    *pDesc,
                    *mpDevice
                );
            }

            mpPipelineState->setID(mName);

        }

        /*
        **
        */
        void CRenderJob::processDepthAttachment(
            std::vector<CRenderJob*>* apRenderJobs
        )
        {
            uint32_t iIndex = 0;
            for(auto const& keyValue : maAttachmentInfo)
            {
                std::string const& attachmentName = keyValue.first;
                if(maAttachmentInfo[attachmentName]["Name"] == "Depth Output")
                {
                    std::string const& parentJobName = maAttachmentInfo[attachmentName]["ParentJobName"];
                    for(auto& pRenderJob : *apRenderJobs)
                    {
                        if(pRenderJob->mName == parentJobName)
                        {
                            mpDepthImage = pRenderJob->mpDepthImage;
                            break;
                        }
                    }

                    WTFASSERT(mpDepthImage != nullptr, "Did not find \"Depth Output\" from parent \"%s\"",
                        parentJobName.c_str()
                    );
                    if(maAttachmentInfo[attachmentName]["Type"] == "TextureInput")
                    {
                        mapImageAttachments["Depth Output"] = mpDepthImage;
                    }
                    break;
                }

                ++iIndex;
            }
            
        }

        /*
        **
        */
        void CRenderJob::createAttachments(
            rapidjson::Document const& doc,
            uint32_t iScreenWidth,
            uint32_t iScreenHeight,
            std::vector<Render::Common::CRenderJob*>* apRenderJobs,
            std::vector<std::map<std::string, std::string>> const* pExtraAttachmentJSONInfo
        )
        {
            auto jsonAttachments = doc["Attachments"].GetArray();
            for(uint32_t i = 0; i < (uint32_t)jsonAttachments.Size(); i++)
            {
                auto const& attachmentJSON = jsonAttachments[i];

                std::string name = attachmentJSON["Name"].GetString();
                std::string type = attachmentJSON["Type"].GetString();

                maAttachmentInfo[name]["Type"] = type;

                // image width, height
                uint32_t iImageWidth = iScreenWidth;
                float fAttachmentScaleWidth = 1.0f;
                if(attachmentJSON.HasMember("ScaleWidth"))
                {
                    fAttachmentScaleWidth = attachmentJSON["ScaleWidth"].GetFloat();
                    iImageWidth = (uint32_t)((float)iImageWidth * fAttachmentScaleWidth);
                    mViewportScale.x = fAttachmentScaleWidth;
                }

                uint32_t iImageHeight = iScreenHeight;
                float fAttachmentScaleHeight = 1.0f;
                if(attachmentJSON.HasMember("ScaleHeight"))
                {
                    fAttachmentScaleHeight = attachmentJSON["ScaleHeight"].GetFloat();
                    iImageHeight = (uint32_t)((float)iImageHeight * fAttachmentScaleHeight);
                    mViewportScale.y = fAttachmentScaleHeight;
                }

                if(attachmentJSON.HasMember("ImageWidth"))
                {
                    iImageWidth = attachmentJSON["ImageWidth"].GetInt();
                }

                if(attachmentJSON.HasMember("ImageHeight"))
                {
                    iImageHeight = attachmentJSON["ImageHeight"].GetInt();
                }

                RenderDriver::Common::Format attachmentFormat = RenderDriver::Common::Format::R8G8B8A8_UNORM;
                if(type == "TextureOutput")
                {
                    maAttachmentMappings.push_back(std::make_pair(name, "texture-output"));

                }   // if TextureOutput
                else if(type == "TextureInput")
                {
                    handleTextureInput(
                        attachmentJSON,
                        apRenderJobs
                    );

                    std::string parentJobName = attachmentJSON["ParentJobName"].GetString();

                    std::string newAttachmentName = parentJobName + "-" + name;
                    maAttachmentMappings.push_back(std::make_pair(newAttachmentName, "texture-input"));

                }   // if TextureInput
                else if(type == "BufferOutput")
                {
                    //handleBufferOutput(
                    //    attachmentJSON
                    //);

                    maAttachmentMappings.push_back(std::make_pair(name, "buffer-output"));

                }   // if BufferOutput
                else if(type == "BufferInput")
                {
                    handleBufferInput(
                        attachmentJSON,
                        apRenderJobs
                    );

                    std::string parentJobName = attachmentJSON["ParentJobName"].GetString();
                    std::string newAttachnmentName = parentJobName + "-" + name;
                    maAttachmentMappings.push_back(std::make_pair(newAttachnmentName, "buffer-input"));
                }
                else if(type == "TextureInputOutput")
                {
                    handleTextureInputOutput(
                        attachmentJSON,
                        apRenderJobs,
                        iImageWidth,
                        iImageHeight
                    );

                    if(name != "Depth Output")
                    {
                        mapOutputImageAttachments[name] = mapImageAttachments[name];
                        mapOutputImageAttachmentViews[name] = mapImageAttachmentViews[name];

                        maAttachmentMappings.push_back(std::make_pair(name, "texture-input-output"));
                    }

                    mapOutputBufferAttachments[name] = nullptr;
                    mapOutputBufferAttachmentViews[name] = nullptr;

                    
                }
                else if(type == "BufferInputOutput")
                {
                    handleBufferInputOutput(
                        attachmentJSON,
                        apRenderJobs
                    );

                    mapOutputImageAttachments[name] = nullptr;
                    mapOutputImageAttachmentViews[name] = nullptr;

                    mapOutputBufferAttachments[name] = mapBufferAttachments[name];
                    mapOutputBufferAttachmentViews[name] = mapBufferAttachmentViews[name];

                    maAttachmentMappings.push_back(std::make_pair(name, "buffer-input-output"));
                }
                else if(type == "AccelerationStructure")
                {
                    maAttachmentMappings.push_back(std::make_pair(name, "acceleration-structure"));
                }
            }
        }

        /*
        **
        */
        void CRenderJob::handleTextureOutput(
            rapidjson::Value const& attachmentJSON,
            uint32_t iImageWidth,
            uint32_t iImageHeight
        )
        {
            std::string attachmentName = attachmentJSON["Name"].GetString();

            RenderDriver::Common::Format attachmentFormat = RenderDriver::Common::Format::R8G8B8A8_UNORM;

            std::string attachmentFormatStr = attachmentJSON["Format"].GetString();
            if(attachmentFormatStr == "rgba32float")
                attachmentFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
            else if(attachmentFormatStr == "bgra8unorm-srgb")
                attachmentFormat = RenderDriver::Common::Format::B8G8R8A8_UNORM_SRGB;
            else if(attachmentFormatStr == "bgra8unorm")
                attachmentFormat = RenderDriver::Common::Format::B8G8R8A8_UNORM;
            else if(attachmentFormatStr == "rgba16float")
                attachmentFormat = RenderDriver::Common::Format::R16G16B16A16_FLOAT;
            else if(attachmentFormatStr == "r32float")
                attachmentFormat = RenderDriver::Common::Format::R32_FLOAT;
            else if(attachmentFormatStr == "r16float")
                attachmentFormat = RenderDriver::Common::Format::R16_FLOAT;
            else if(attachmentFormatStr == "rg32float")
                attachmentFormat = RenderDriver::Common::Format::R32G32_FLOAT;
            else if(attachmentFormatStr == "rg16float")
                attachmentFormat = RenderDriver::Common::Format::R16G16_FLOAT;
            else if(attachmentFormatStr == "r32float")
                attachmentFormat = RenderDriver::Common::Format::R32_FLOAT;
            else if(attachmentFormatStr == "rgb10a2unorm")
                attachmentFormat = RenderDriver::Common::Format::R10G10B10A2_UNORM;
            else if(attachmentFormatStr == "bgr10a2unorm")
                attachmentFormat = RenderDriver::Common::Format::R10G10B10A2_UNORM;
            
            platformCreateAttachmentImage(
                attachmentName,
                iImageWidth,
                iImageHeight,
                attachmentFormat);

            maAttachmentFormats[attachmentName] = attachmentFormat;

            if(attachmentJSON.HasMember("LoadOp"))
            {
                if(std::string(attachmentJSON["LoadOp"].GetString()) == "Load")
                {
                    maAttachmentLoadOps[attachmentName] = LoadStoreOp::Load;
                }
            }
            
            if(attachmentJSON.HasMember("StoreOp"))
            {
                if(std::string(attachmentJSON["StoreOp"].GetString()) == "Store")
                {
                    maAttachmentStoreOps[attachmentName] = LoadStoreOp::Store;
                }
            }

            mapOutputImageAttachments[attachmentName] = mapImageAttachments[attachmentName];
        }
          
        /*
        **
        */
        void CRenderJob::handleTextureInput(
            rapidjson::Value const& attachmentJSON,
            std::vector<Render::Common::CRenderJob*>* apRenderJobs
        )
        {
            // get parent job
            std::string name = attachmentJSON["Name"].GetString();
            std::string parentJobName = attachmentJSON["ParentJobName"].GetString();
            std::string parentName = name;

            if(attachmentJSON.HasMember("ParentName"))
            {
                parentName = attachmentJSON["ParentName"].GetString();
            }

            CRenderJob* pParentJob = nullptr;
            for(auto& pJob : *apRenderJobs)
            {
                if(pJob->mName == parentJobName)
                {
                    pParentJob = pJob;
                    break;
                }
            }

            std::string newAttachmentName = parentJobName + "-" + parentName;
            if(pParentJob && pParentJob->mapImageAttachments.count(parentName))
            {
                // set parent attachments and views, nullptr => input texture
                mapImageAttachments[newAttachmentName] = pParentJob->mapImageAttachments[parentName];
                mapImageAttachmentViews[newAttachmentName] = pParentJob->mapImageAttachmentViews[parentName];
                maAttachmentFormats[newAttachmentName] = pParentJob->maAttachmentFormats[parentName];

                mapInputImageAttachments[newAttachmentName] = pParentJob->mapImageAttachments[parentName];
                mapInputImageAttachmentViews[newAttachmentName] = pParentJob->mapImageAttachmentViews[parentName];
            }
            else
            {
                if(parentName == "Depth Output")
                {
                    WTFASSERT(pParentJob->mpDepthImage, "Parent Job \"%s\" don't have depth output\n", pParentJob->mName.c_str());
                    mapImageAttachments[newAttachmentName] = pParentJob->mpDepthImage;
                    mapImageAttachmentViews[newAttachmentName] = pParentJob->mpDepthImageView;

                    mapInputImageAttachments[newAttachmentName] = pParentJob->mpDepthImage;
                    mapInputImageAttachmentViews[newAttachmentName] = pParentJob->mpDepthImageView;
                }
                else
                {
                    WTFASSERT(0, "\"%s\" should not be here, no parent name \"%s\" parent job \"%s\"", 
                        mName.c_str(),
                        parentName.c_str(), 
                        parentJobName.c_str());
                }
               
            }
        }

        /*
        **
        */
        void CRenderJob::handleBufferOutput(
            rapidjson::Value const& attachmentJSON
        )
        {
            RenderDriver::Common::BufferUsage usage = RenderDriver::Common::BufferUsage(
                uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) | 
                uint32_t(RenderDriver::Common::BufferUsage::StorageTexel) |
                uint32_t(RenderDriver::Common::BufferUsage::TransferDest) | 
                uint32_t(RenderDriver::Common::BufferUsage::IndirectBuffer)
            );
            if(attachmentJSON.HasMember("Usage") && std::string(attachmentJSON["Usage"].GetString()) == "Vertex")
            {
                usage = (RenderDriver::Common::BufferUsage)((uint32_t)usage | uint32_t(RenderDriver::Common::BufferUsage::VertexBuffer));
            }

            if(attachmentJSON.HasMember("CPU Visible"))
            {
                if(std::string(attachmentJSON["CPU Visible"].GetString()) == "True")
                {
                    usage = (RenderDriver::Common::BufferUsage)((uint32_t)usage | uint32_t(RenderDriver::Common::BufferUsage::TransferSrc));
                }
            }

            uint32_t iBufferSize = 0;
            iBufferSize = attachmentJSON["Size"].GetInt();
            
            std::string name = attachmentJSON["Name"].GetString();
            platformCreateAttachmentBuffer(
                name,
                iBufferSize,
                RenderDriver::Common::Format::R32_FLOAT,
                usage);

            mapOutputBufferAttachments[name] = mapBufferAttachments[name];
        }

        /*
        **
        */
        void CRenderJob::handleBufferInput(
            rapidjson::Value const& attachmentJSON,
            std::vector<Render::Common::CRenderJob*>* apRenderJobs
        )
        {
            // get parent job
            std::string parentJobName = attachmentJSON["ParentJobName"].GetString();
            std::string parentName = attachmentJSON["Name"].GetString(); 
            if(attachmentJSON.HasMember("ParentName"))
            {
                parentName = attachmentJSON["ParentName"].GetString();
            }
            CRenderJob* pParentJob = nullptr;
            for(auto& pJob : *apRenderJobs)
            {
                if(pJob->mName == parentJobName)
                {
                    pParentJob = pJob;
                    break;
                }
            }

            std::string newAttachmentName = parentJobName + "-" + parentName;
            if(pParentJob && pParentJob->mapBufferAttachments.count(parentName))
            {
                // set parent attachments and views, nullptr => input buffer
                mapBufferAttachments[newAttachmentName] = pParentJob->mapBufferAttachments[parentName];
                mapBufferAttachmentViews[newAttachmentName] = pParentJob->mapBufferAttachmentViews[parentName];
                maAttachmentFormats[newAttachmentName] = pParentJob->maAttachmentFormats[parentName];

                mapInputBufferAttachments[newAttachmentName] = pParentJob->mapBufferAttachments[parentName];
                mapInputBufferAttachmentViews[newAttachmentName] = pParentJob->mapBufferAttachmentViews[parentName];
            }
            else
            {
                WTFASSERT(0, "check this");

                // delayed attachment
                mapBufferAttachments[parentName] = nullptr;

                // save info to create after all the render jobs have been loaded
                std::map<std::string, std::string> delayedAttachment;
                delayedAttachment["Name"] = attachmentJSON["Name"].GetString();
                delayedAttachment["ParentJobName"] = parentJobName;
                delayedAttachment["ParentName"] = parentName;
            }
        }

        /*
        **
        */
        void CRenderJob::handleBufferInputOutput(
            rapidjson::Value const& attachmentJSON,
            std::vector<Render::Common::CRenderJob*>* apRenderJobs
        )
        {
            RenderDriver::Common::BufferUsage usage = RenderDriver::Common::BufferUsage::StorageBuffer;

            std::string const name = attachmentJSON["Name"].GetString();

            // get parent job
            std::string parentJobName = attachmentJSON["ParentJobName"].GetString();
            std::string parentName = name;
            if(attachmentJSON.HasMember("ParentName"))
            {
                parentName = attachmentJSON["ParentName"].GetString();
            }

            CRenderJob* pParentJob = nullptr;
            for(auto& pJob : *apRenderJobs)
            {
                if(pJob->mName == parentJobName)
                {
                    pParentJob = pJob;
                    break;
                }
            }

            // create attachment 
            if(pParentJob && pParentJob->mapBufferAttachments.count(parentName))
            {
                // set parent attachments and views, nullptr => input texture
                mapBufferAttachments[parentName] = pParentJob->mapBufferAttachments[parentName];
                mapBufferAttachmentViews[parentName] = pParentJob->mapBufferAttachmentViews[parentName];
                maAttachmentFormats[parentName] = pParentJob->maAttachmentFormats[parentName];

                mapBufferAttachments[parentName] = pParentJob->mapBufferAttachments[parentName];
                mapInputBufferAttachments[parentName] = pParentJob->mapBufferAttachments[parentName];
                mapOutputBufferAttachments[parentName] = pParentJob->mapBufferAttachments[parentName];
            }
        }

        /*
        **
        */
        void CRenderJob::handleTextureInputOutput(
            rapidjson::Value const& attachmentJSON,
            std::vector<Render::Common::CRenderJob*>* apRenderJobs,
            uint32_t iImageWidth,
            uint32_t iImageHeight
        )
        {
            // get parent job
            std::string parentJobName = attachmentJSON["ParentJobName"].GetString();
            std::string parentName = attachmentJSON["Name"].GetString();   
            if(attachmentJSON.HasMember("ParentName"))
            {
                parentName = attachmentJSON["ParentName"].GetString();
            }
            CRenderJob* pParentJob = nullptr;
            for(auto& pJob : *apRenderJobs)
            {
                if(pJob->mName == parentJobName)
                {
                    pParentJob = pJob;
                    break;
                }
            }
            
            WTFASSERT(pParentJob, "Can't find parent job \"%s\"", parentJobName.c_str());

            std::string attachmentName = attachmentJSON["Name"].GetString();
            if(parentName == "Depth Output")
            {
                mapImageAttachments[parentName] = pParentJob->mpDepthImage;
                mpDepthImage = pParentJob->mpDepthImage;
                mpDepthImageView = pParentJob->mpDepthImageView;
                mbInputOutputDepth = true;

                //mapInputImageAttachments[parentName] = pParentJob->mapImageAttachments[parentName];
                //mapOutputImageAttachments[parentName] = pParentJob->mapImageAttachments[parentName];

                //mapInputImageAttachmentViews[parentName] = pParentJob->mapImageAttachmentViews[parentName];
                //mapOutputImageAttachmentViews[parentName] = pParentJob->mapImageAttachmentViews[parentName];
            }
            else
            {
                if(pParentJob && pParentJob->mapImageAttachments.count(parentName))
                {
                    mapImageAttachments[parentName] = pParentJob->mapImageAttachments[parentName];
                    mapImageAttachmentViews[parentName] = pParentJob->mapImageAttachmentViews[parentName];
                    maAttachmentFormats[parentName] = pParentJob->maAttachmentFormats[parentName];

                    //mapInputImageAttachments[parentName] = pParentJob->mapImageAttachments[parentName];
                    //mapOutputImageAttachments[parentName] = pParentJob->mapImageAttachments[parentName];

                    //mapInputImageAttachmentViews[parentName] = pParentJob->mapImageAttachmentViews[parentName];
                    //mapOutputImageAttachmentViews[parentName] = pParentJob->mapImageAttachmentViews[parentName];
                }
            }
        }

        /*
        **
        */
        void CRenderJob::handleUniformBuffer(
            rapidjson::Value const& shaderResource,
            std::vector<CRenderJob*>* apRenderJobs)
        {
            std::string name = shaderResource["name"].GetString();
            std::string type = shaderResource["type"].GetString();
            std::string usageStr = shaderResource["usage"].GetString();
            uint32_t iBufferSize = 0;
            if(shaderResource.HasMember("size"))
            {
                iBufferSize = shaderResource["size"].GetInt();
            }

            // look for parent uniform 
            bool bSkipCreation = (iBufferSize <= 0);
            if(shaderResource.HasMember("parent_job"))
            {
                std::string parentJobName = shaderResource["parent_job"].GetString();
                std::string parentShaderResourceName = name;
                for(auto& pRenderJob : *apRenderJobs)
                {
                    if(pRenderJob && pRenderJob->mName == parentJobName)
                    {
                        // look for the shader resource in parent job
                        bool bFound = false;
                        for(auto& keyValue : pRenderJob->maShaderResourceInfo)
                        {
                            if(keyValue.first == parentShaderResourceName)
                            {
                                mapUniformBuffers[name] = pRenderJob->mapUniformBuffers[name];
                                
                                if(keyValue.second["type"] == "buffer")
                                {
                                    mapInputBufferAttachments[name] = pRenderJob->mapUniformBuffers[name];
                                }
                                else
                                {
                                    mapInputImageAttachments[name] = pRenderJob->mapUniformImages[name];
                                }

                                bSkipCreation = true;
                                bFound = true;
                                break;
                            }
                        }

                        WTFASSERT(mapUniformBuffers[name] != nullptr, "Did not find attachment \"%s\" from parent \"%s\"", name.c_str(), parentJobName.c_str());

                        // see if it's an attachment
                        if(!bSkipCreation)
                        {
                            for(auto const& keyValue : pRenderJob->mapBufferAttachments)
                            {
                                if(keyValue.first == parentShaderResourceName)
                                {
                                    mapUniformBuffers[name] = pRenderJob->mapBufferAttachments[parentShaderResourceName];
                                    bSkipCreation = true;
                                    break;
                                }
                            }
                        }

                        if(bFound)
                        {
                            break;
                        }

                    }   // if render job name == parent job name

                }

            }   // if shader resource has parent

            // usage
            RenderDriver::Common::BufferUsage usage = RenderDriver::Common::BufferUsage(
                uint32_t(RenderDriver::Common::BufferUsage::UniformBuffer) |
                uint32_t(RenderDriver::Common::BufferUsage::UniformTexel) | 
                uint32_t(RenderDriver::Common::BufferUsage::TransferDest)
            );
            if(usageStr == "read_write_storage")
            {
                usage = RenderDriver::Common::BufferUsage(
                    uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) |
                    uint32_t(RenderDriver::Common::BufferUsage::StorageTexel) |
                    uint32_t(RenderDriver::Common::BufferUsage::TransferDest)
                );
            }

            // create buffer
            if(!bSkipCreation)
            {
                RenderDriver::Common::BufferDescriptor bufferDesc =
                {
                    iBufferSize,
                    RenderDriver::Common::Format::R32_FLOAT,
                    RenderDriver::Common::ResourceFlagBits::None,
                    RenderDriver::Common::HeapType::Default,
                    RenderDriver::Common::ResourceStateFlagBits::None,
                    nullptr,
                    0,
                    0,
                    RenderDriver::Common::Format::R32_FLOAT,
                    usage
                };

                mapUniformBuffers[name] = platformCreateBuffer(name, bufferDesc);
            }
        }

        /*
        **
        */
        void CRenderJob::handleUniformTexture(
            rapidjson::Value const& shaderResource,
            std::vector<CRenderJob*>* apRenderJobs,
            RenderDriver::Common::CCommandQueue* pCommandQueue
        )
        {
            std::string name = shaderResource["name"].GetString();

            int32_t iTextureWidth = 0, iTextureHeight = 0;
            RenderDriver::Common::Format textureFormat = RenderDriver::Common::Format::R8G8B8A8_UNORM;
            if(shaderResource.HasMember("file_path"))
            {
                std::string filePath = shaderResource["file_path"].GetString();
                
                std::vector<unsigned char> acImageData;
                platformLoadImage(
                    acImageData,
                    iTextureWidth,
                    iTextureHeight,
                    filePath);
                
                unsigned char* pImageData = acImageData.data();
                RenderDriver::Common::Format format = RenderDriver::Common::Format::R8G8B8A8_UNORM;
                RenderDriver::Common::CImage* pImage = platformCreateImageWithData(
                    name,
                    (unsigned char const*)pImageData,
                    iTextureWidth,
                    iTextureHeight,
                    format,
                    pCommandQueue
                );

                mapUniformImages[name] = pImage;
                
                // image view
                RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
                imageViewDesc.mFormat = format;
                imageViewDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
                imageViewDesc.mpImage = pImage;
                imageViewDesc.mViewType = RenderDriver::Common::ResourceViewType::RenderTargetView;
                platformCreateImageView(
                    name, 
                    imageViewDesc);

                
            }
        }

        /*
        **
        */
        void CRenderJob::computeSemaphoreValues(
            CreateInfo& createInfo
        )
        {
            // look for parent job with the largest signal semaphore value and use as that as wait value 
            // image input attachment
            for(auto const& pInputImageAttachment : mapInputImageAttachments)
            {
                if(pInputImageAttachment.second != nullptr)
                {
                    // get parent job name for input attachment or shader resource
                    std::string parentJobName = "";
                    if(maAttachmentInfo.count(pInputImageAttachment.first) > 0)
                    {
                        auto& attachmentInfo = maAttachmentInfo[pInputImageAttachment.first];
                        parentJobName = attachmentInfo["ParentJobName"];
                    }
                    else if(maShaderResourceInfo.count(pInputImageAttachment.first) > 0)
                    {
                        auto& shaderResourceInfo = maShaderResourceInfo[pInputImageAttachment.first];
                        parentJobName = shaderResourceInfo["ParentJobName"];
                    }

                    for(auto const& job : *createInfo.mpaRenderJobs)
                    {
                        if(job->mName == parentJobName)
                        {
                            if(miWaitSemaphoreValue < job->miSignalSemaphoreValue)
                            {
                                miWaitSemaphoreValue = job->miSignalSemaphoreValue;

                                // set the wait fence to the parent job's signal fence
                                mpWaitFence = job->mpSignalFence;
                            }
                            break;
                        }
                    }
                }
            }

            // buffer input attachment
            for(auto const& pInputBufferAttachment : mapInputBufferAttachments)
            {
                if(pInputBufferAttachment.second != nullptr)
                {
                    // get parent job name for input attachment or shader resource
                    std::string parentJobName = "";
                    if(maAttachmentInfo.count(pInputBufferAttachment.first) > 0)
                    {
                        auto& attachmentInfo = maAttachmentInfo[pInputBufferAttachment.first];
                        parentJobName = attachmentInfo["ParentJobName"];
                    }
                    else if(maShaderResourceInfo.count(pInputBufferAttachment.first) > 0)
                    {
                        auto& shaderResourceInfo = maShaderResourceInfo[pInputBufferAttachment.first];
                        parentJobName = shaderResourceInfo["parent_job"];
                    }

                    // look for the parent job and use its signal semaphore value for wait value
                    for(auto const& job : *createInfo.mpaRenderJobs)
                    {
                        if(job->mName == parentJobName)
                        {
                            if(miWaitSemaphoreValue < job->miSignalSemaphoreValue)
                            {
                                miWaitSemaphoreValue = job->miSignalSemaphoreValue;

                                // set the wait fence to the parent job's signal fence
                                mpWaitFence = job->mpSignalFence;
                            }
                            break;
                        }
                    }
                }
            }

            // update the signal value for any output attachments
            if(mapOutputImageAttachments.size() > 0 || mapOutputBufferAttachments.size() > 0)
            {
                miSignalSemaphoreValue = miWaitSemaphoreValue + 1;
                if(*createInfo.mpiSemaphoreValue < miSignalSemaphoreValue)
                {
                    *createInfo.mpiSemaphoreValue = miSignalSemaphoreValue;
                }
            }
        }

        /*
        **
        */
        void CRenderJob::initAttachmentBarriers(CreateInfo const& createInfo)
        {
            platformInitAttachmentBarriers(createInfo);
        }

    }   // Common

}   // Render
