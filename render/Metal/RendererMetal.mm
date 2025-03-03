#include <render/Metal/RendererMetal.h>
#include <render/Metal/RenderJobMetal.h>

#include <render-driver/Metal/BufferMetal.h>
#include <render-driver/Metal/CommandQueueMetal.h>
#include <render-driver/Metal/PipelineStateMetal.h>
#include <render-driver/Metal/CommandAllocatorMetal.h>
#include <render-driver/Metal/CommandBufferMetal.h>
#include <render-driver/Metal/FenceMetal.h>
#include <render-driver/Metal/UtilsMetal.h>
//#include <render-driver/Metal/FrameBufferMetal.h>
//#include <render-driver/Metal/AccelerationStructureMetal.h>

#include <render-driver/Metal/ImageMetal.h>

#include <utils/LogPrint.h>
#include <utils/wtfassert.h>

#include <sstream>

extern char const* getSaveDir();

namespace Render
{
    namespace Metal
    {
        /*
        **
        */
        void CRenderer::platformSetup(Render::Common::RendererDescriptor const& desc)
        {
            bool bRenderDoc = true;

            Render::Metal::RendererDescriptor const& descMetal = static_cast<Render::Metal::RendererDescriptor const&>(desc);
            
            mRenderDriverType = Render::Common::RenderDriverType::Metal;

            mpDevice = std::make_unique<RenderDriver::Metal::CDevice>();
            RenderDriver::Common::CDevice::CreateDesc createDesc = {};
            createDesc.mpPhyiscalDevice = nullptr;
            mpDevice->create(createDesc);

            mpSwapChain = std::make_unique<RenderDriver::Metal::CSwapChain>();
            RenderDriver::Metal::SwapChainDescriptor swapChainCreateDesc = {};
            swapChainCreateDesc.mFormat = desc.mFormat;
            swapChainCreateDesc.miWidth = desc.miScreenWidth;
            swapChainCreateDesc.miHeight = desc.miScreenHeight;
            mpSwapChain->create(swapChainCreateDesc, *mpDevice);
            
            mpComputeCommandQueue = std::make_unique<RenderDriver::Metal::CCommandQueue>();
            mpGraphicsCommandQueue = std::make_unique<RenderDriver::Metal::CCommandQueue>();
            mpCopyCommandQueue = std::make_unique<RenderDriver::Metal::CCommandQueue>();
            mpGPUCopyCommandQueue = std::make_unique<RenderDriver::Metal::CCommandQueue>();
            
            RenderDriver::Metal::CCommandQueue::CreateDesc queueCreateDesc = {};
            queueCreateDesc.mpDevice = mpDevice.get();
            queueCreateDesc.mType = RenderDriver::Common::CCommandQueue::Type::Graphics;
            mpGraphicsCommandQueue->create(queueCreateDesc);
            queueCreateDesc.mType = RenderDriver::Common::CCommandQueue::Type::Compute;
            mpComputeCommandQueue->create(queueCreateDesc);
            queueCreateDesc.mType = RenderDriver::Common::CCommandQueue::Type::Copy;
            mpCopyCommandQueue->create(queueCreateDesc);
            queueCreateDesc.mType = RenderDriver::Common::CCommandQueue::Type::CopyGPU;
            mpGPUCopyCommandQueue->create(queueCreateDesc);

            mpGraphicsCommandQueue->setID("Graphics Command Queue");
            mpComputeCommandQueue->setID("Compute Command Queue");
            mpCopyCommandQueue->setID("Copy Command Queue");
            mpGPUCopyCommandQueue->setID("GPU Copy Command Queue");
            
            mDefaultUniformBuffer = std::make_unique<RenderDriver::Metal::CBuffer>();

            RenderDriver::Common::BufferDescriptor bufferCreationDesc = {};
            bufferCreationDesc.miSize = 1024;
            bufferCreationDesc.mBufferUsage = RenderDriver::Common::BufferUsage(
                uint32_t(RenderDriver::Common::BufferUsage::UniformBuffer) | 
                uint32_t(RenderDriver::Common::BufferUsage::TransferDest)
            );
            bufferCreationDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            mDefaultUniformBuffer->create(
                bufferCreationDesc, 
                *mpDevice
            );
            mDefaultUniformBuffer->setID("Default Uniform Buffer");
            mpDefaultUniformBuffer = mDefaultUniformBuffer.get();
            
            RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc;
            RenderDriver::Metal::CommandBufferDescriptor commandBufferDesc;
            
            mapQueueCommandAllocators.resize(RenderDriver::Common::CCommandQueue::NumTypes);
            mapQueueCommandBuffers.resize(RenderDriver::Common::CCommandQueue::NumTypes);
            for(uint32_t i = 0; i < static_cast<uint32_t>(mapQueueCommandBuffers.size()); i++)
            {
                std::string name = "Graphics Queue";
                if(i == RenderDriver::Common::CCommandQueue::Type::Graphics)
                {
                    commandBufferDesc.mpCommandQueue = (__bridge id<MTLCommandQueue>)mpGraphicsCommandQueue->getNativeCommandQueue();
                }
                else if(i == RenderDriver::Common::CCommandQueue::Type::Compute)
                {
                    name = "Compute Queue";
                    commandBufferDesc.mpCommandQueue = (__bridge id<MTLCommandQueue>)mpComputeCommandQueue->getNativeCommandQueue();
                }
                else if(i == RenderDriver::Common::CCommandQueue::Type::Copy)
                {
                    name = "Copy Queue";
                    commandBufferDesc.mpCommandQueue = (__bridge id<MTLCommandQueue>)mpCopyCommandQueue->getNativeCommandQueue();
                }
                
                // command buffers
                mapQueueCommandAllocators[i] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                mapQueueCommandBuffers[i] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                commandAllocatorDesc.mType = static_cast<RenderDriver::Common::CommandBufferType>(i);
                mapQueueCommandAllocators[i]->create(commandAllocatorDesc, *mpDevice);
                mapQueueCommandAllocators[i]->setID(name + " Command Allocator");

                commandBufferDesc.mpCommandAllocator = mapQueueCommandAllocators[i].get();
                commandBufferDesc.mType = commandAllocatorDesc.mType;
                commandBufferDesc.mpPipelineState = nullptr;
                
                mapQueueCommandBuffers[i]->create(commandBufferDesc, *mpDevice);
                mapQueueCommandBuffers[i]->setID(name + " Command Buffer");

                mapQueueCommandAllocators[i]->reset();
                mapQueueCommandBuffers[i]->reset();
            }
            
            // upload command buffer
            mpUploadCommandBuffer = std::make_unique<RenderDriver::Metal::CCommandBuffer>();
            RenderDriver::Metal::CommandBufferDescriptor metalCommandBufferDesc = {};
            metalCommandBufferDesc.mpCommandAllocator = mpUploadCommandAllocator.get();
            metalCommandBufferDesc.mpPipelineState = nullptr;
            metalCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
            metalCommandBufferDesc.mpCommandQueue = (__bridge id<MTLCommandQueue>)mpCopyCommandQueue->getNativeCommandQueue();
            mpUploadCommandBuffer->create(metalCommandBufferDesc, *mpDevice);
            mpUploadCommandBuffer->setID("Upload Command Buffer");
            mpUploadCommandBuffer->reset();
            
            mpUploadFence = std::make_unique<RenderDriver::Metal::CFence>();
            RenderDriver::Common::FenceDescriptor fenceDesc = {};
            mpUploadFence->create(fenceDesc, *mpDevice);
            mpUploadFence->setID("Upload Fence");

            // upload command buffer allocator
            mpUploadCommandAllocator = std::make_unique<RenderDriver::Metal::CCommandAllocator>();
            commandAllocatorDesc = {};
            commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
            mpUploadCommandAllocator->create(commandAllocatorDesc, *mpDevice);
            mpUploadCommandAllocator->setID("Upload Command Allocator");
            
            id<MTLDevice> nativeDevice = (__bridge id<MTLDevice>)mpDevice->getNativeDevice();
            
            // texture samplers
            MTLSamplerDescriptor* linearSamplerDescriptor = [MTLSamplerDescriptor new];
            linearSamplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
            linearSamplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
            mDefaultNativeLinearSamplerState = [nativeDevice newSamplerStateWithDescriptor: linearSamplerDescriptor];
            
            MTLSamplerDescriptor* pointSamplerDescriptor = [MTLSamplerDescriptor new];
            pointSamplerDescriptor.magFilter = MTLSamplerMinMagFilterNearest;
            pointSamplerDescriptor.minFilter = MTLSamplerMinMagFilterNearest;
            mDefaultNativePointSamplerState = [nativeDevice newSamplerStateWithDescriptor: pointSamplerDescriptor];
            
            // filler buffer and textures
            mFillerBuffer = std::make_unique<RenderDriver::Metal::CBuffer>();
            mFillerTexture = std::make_unique<RenderDriver::Metal::CImage>();
            
            RenderDriver::Common::BufferDescriptor bufferDesc = {};
            bufferDesc.miSize = 64;
            bufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            
            RenderDriver::Common::ImageDescriptor imageDesc = {};
            imageDesc.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
            imageDesc.miWidth = 16;
            imageDesc.miHeight = 16;
            imageDesc.miNumImages = 1;
            
            mFillerBuffer->create(bufferDesc, *mpDevice);
            mFillerTexture->create(imageDesc, *mpDevice);
            
            mFillerBuffer->setID("Filler Buffer");
            mFillerTexture->setID("Filler Texture");
            
            // set up indirect command buffer
            {
                MTLIndirectCommandBufferDescriptor* icbDescriptor = [MTLIndirectCommandBufferDescriptor new];
                icbDescriptor.commandTypes = MTLIndirectCommandTypeDraw;
                icbDescriptor.inheritBuffers = NO;
                icbDescriptor.maxVertexBufferBindCount = 1;
                icbDescriptor.maxFragmentBufferBindCount = 1;
                icbDescriptor.inheritPipelineState = YES;
                
                // create command buffer to store the draw commands
                mGenerateIndirectDrawCommandBuffer = [nativeDevice
                  newIndirectCommandBufferWithDescriptor: icbDescriptor
                  maxCommandCount: 4096
                  options: MTLResourceStorageModePrivate];
                mGenerateIndirectDrawCommandBuffer.label = @"Generate Indirect Draw Command Buffer";
                
                
                NSError *error;
                
                // Load the shaders from default library
                std::string computeShaderFilePath = std::string(getSaveDir()) + "/shader-output/indirect-draw-command-compute.metallib";
                DEBUG_PRINTF("%s\n", computeShaderFilePath.c_str());
                FILE* fp = fopen(computeShaderFilePath.c_str(), "rb");
                WTFASSERT(fp, "can\'t open file \"%s\"", computeShaderFilePath.c_str());
                fseek(fp, 0, SEEK_END);
                uint64_t iShaderFileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                std::vector<char> acShaderFileContent(iShaderFileSize);
                fread(acShaderFileContent.data(), sizeof(char), iShaderFileSize, fp);
                fclose(fp);
                dispatch_data_t shaderData = dispatch_data_create(
                    acShaderFileContent.data(),
                    iShaderFileSize,
                    dispatch_get_main_queue(),
                    DISPATCH_DATA_DESTRUCTOR_DEFAULT);
                id<MTLLibrary> mDrawCommandComputeLibary = [
                    nativeDevice newLibraryWithData:
                        shaderData error: &error];
                
                id<MTLFunction> GPUCommandEncodingKernel = [mDrawCommandComputeLibary newFunctionWithName:@"createDrawCommands"];
                mIndirectDrawCommandComputePipeline = [nativeDevice
                    newComputePipelineStateWithFunction:GPUCommandEncodingKernel
                    error:&error];
                
                // argument buffer containing the encoded command buffers
                id<MTLArgumentEncoder> argumentEncoder =
                    [GPUCommandEncodingKernel newArgumentEncoderWithBufferIndex:1];
                NSUInteger encodedLength = argumentEncoder.encodedLength;
                mIndirectDrawCommandArgumentBuffer = [nativeDevice newBufferWithLength:encodedLength
                                                       options:MTLResourceStorageModeShared];
                mIndirectDrawCommandArgumentBuffer.label = @"Generating Draw Command Argument Buffer";
                [argumentEncoder setArgumentBuffer:mIndirectDrawCommandArgumentBuffer offset:0];
                [argumentEncoder setIndirectCommandBuffer:mGenerateIndirectDrawCommandBuffer
                                                  atIndex:0];
            }
            
            mDesc = desc;
        }

        /*
        **
        */
        void CRenderer::transitionBarriers(
            std::vector<RenderDriver::Common::Utils::TransitionBarrierInfo> const& aBarriers)
        {
            WTFASSERT(0, "Implement me");
            
            // create graphics command buffer if needed
            if(mapQueueGraphicsCommandBuffers[0] == nullptr)
            {
                mapQueueGraphicsCommandAllocators[0] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                mapQueueGraphicsCommandBuffers[0] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                RenderDriver::Common::CommandAllocatorDescriptor graphicsCommandAllocatorDesc;
                graphicsCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                mapQueueGraphicsCommandAllocators[0]->create(graphicsCommandAllocatorDesc, *mpDevice);

                std::ostringstream graphicCommandAllocatorName;
                graphicCommandAllocatorName << "Graphics Command Allocator 0";
                mapQueueGraphicsCommandAllocators[0]->setID(graphicCommandAllocatorName.str());
                mapQueueGraphicsCommandAllocators[0]->reset();

                RenderDriver::Common::CommandBufferDescriptor graphicsCommandBufferDesc;
                graphicsCommandBufferDesc.mpCommandAllocator = mapQueueGraphicsCommandAllocators[0].get();
                graphicsCommandBufferDesc.mpPipelineState = nullptr;
                graphicsCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                mapQueueGraphicsCommandBuffers[0]->create(graphicsCommandBufferDesc, *mpDevice);

                std::ostringstream graphicCommandBufferName;
                graphicCommandBufferName << "Graphics Command Buffer 0";
                mapQueueGraphicsCommandBuffers[0]->setID(graphicCommandBufferName.str());
                mapQueueGraphicsCommandBuffers[0]->reset();
            }

            // create compute command buffer if needed
            if(mapQueueComputeCommandBuffers[0] == nullptr)
            {
                mapQueueComputeCommandAllocators[0] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                mapQueueComputeCommandBuffers[0] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                RenderDriver::Common::CommandAllocatorDescriptor computeCommandAllocatorDesc;
                computeCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                mapQueueComputeCommandAllocators[0]->create(computeCommandAllocatorDesc, *mpDevice);
                mapQueueComputeCommandAllocators[0]->reset();

                std::ostringstream computeCommandAllocatorName;
                computeCommandAllocatorName << "Compute Command Allocator 0";
                mapQueueComputeCommandAllocators[0]->setID(computeCommandAllocatorName.str());

                RenderDriver::Common::CommandBufferDescriptor computeCommandBufferDesc;
                computeCommandBufferDesc.mpCommandAllocator = mapQueueComputeCommandAllocators[0].get();
                computeCommandBufferDesc.mpPipelineState = nullptr;
                computeCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                mapQueueComputeCommandBuffers[0]->create(computeCommandBufferDesc, *mpDevice);

                std::ostringstream computeCommandBufferName;
                computeCommandBufferName << "Compute Command Buffer 0";
                mapQueueComputeCommandBuffers[0]->setID(computeCommandBufferName.str());
                mapQueueComputeCommandBuffers[0]->reset();
            }

            for(uint32_t i = 0; i < static_cast<uint32_t>(aBarriers.size()); i++)
            {
                DEBUG_PRINTF("transition \"%s\"\n",
                    aBarriers[i].mpImage->getID().c_str());
            }

            auto transitionImageBarriers = [&](
                RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
                uint32_t iNumBarrierInfo,
                RenderDriver::Common::CCommandBuffer* pCommandBuffer,
                uint32_t iQueueType)
            {
                // image transitions
                uint32_t iNumImageTransitions = 0;
                for(uint32_t i = 0; i < iNumBarrierInfo; i++)
                {
                    RenderDriver::Common::Utils::TransitionBarrierInfo const& barrierInfo = aBarrierInfo[i];
                    if(barrierInfo.mpImage &&
                        barrierInfo.mAfter != RenderDriver::Common::ResourceStateFlagBits::CopyDestination &&
                        barrierInfo.mAfter != RenderDriver::Common::ResourceStateFlagBits::CopySource)
                    {
                        
                    }
                }

                if(iNumImageTransitions > 0)
                {
                    // pipeline barrier
                }
            };

            // transition in graphics queue
            {
                transitionImageBarriers(
                    aBarriers.data(),
                    static_cast<uint32_t>(aBarriers.size()),
                    mapQueueGraphicsCommandBuffers[0].get(),
                    static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Graphics));

                mapQueueGraphicsCommandBuffers[0]->close();
                mpGraphicsCommandQueue->execCommandBuffer(
                    *mapQueueGraphicsCommandBuffers[0].get(),
                    *mpDevice);
                
                // native wait
                
                mapQueueGraphicsCommandBuffers[0]->reset();
            }

            // transition in compute queue
            {
                transitionImageBarriers(
                    aBarriers.data(),
                    static_cast<uint32_t>(aBarriers.size()),
                    mapQueueComputeCommandBuffers[0].get(),
                    static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Compute));

                mapQueueComputeCommandBuffers[0]->close();
                mpComputeCommandQueue->execCommandBuffer(
                    *mapQueueComputeCommandBuffers[0].get(),
                    *mpDevice);
            
                // native wait
                
                mapQueueComputeCommandBuffers[0]->reset();
            }
        }

        /*
        **
        */
        void CRenderer::transitionImageLayouts(
            std::vector<RenderDriver::Metal::Utils::ImageLayoutTransition> const& aTransitions)
        {
            WTFASSERT(0, "Implement me");
            
            // create graphics command buffer if needed
            if(mapQueueGraphicsCommandBuffers[0] == nullptr)
            {
                mapQueueGraphicsCommandAllocators[0] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                mapQueueGraphicsCommandBuffers[0] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                RenderDriver::Common::CommandAllocatorDescriptor graphicsCommandAllocatorDesc;
                graphicsCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                mapQueueGraphicsCommandAllocators[0]->create(graphicsCommandAllocatorDesc, *mpDevice);

                std::ostringstream graphicCommandAllocatorName;
                graphicCommandAllocatorName << "Graphics Command Allocator 0";
                mapQueueGraphicsCommandAllocators[0]->setID(graphicCommandAllocatorName.str());
                mapQueueGraphicsCommandAllocators[0]->reset();

                RenderDriver::Common::CommandBufferDescriptor graphicsCommandBufferDesc;
                graphicsCommandBufferDesc.mpCommandAllocator = mapQueueGraphicsCommandAllocators[0].get();
                graphicsCommandBufferDesc.mpPipelineState = nullptr;
                graphicsCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                mapQueueGraphicsCommandBuffers[0]->create(graphicsCommandBufferDesc, *mpDevice);

                std::ostringstream graphicCommandBufferName;
                graphicCommandBufferName << "Graphics Command Buffer 0";
                mapQueueGraphicsCommandBuffers[0]->setID(graphicCommandBufferName.str());
                mapQueueGraphicsCommandBuffers[0]->reset();
            }

            // create compute command buffer if needed
            if(mapQueueComputeCommandBuffers[0] == nullptr)
            {
                mapQueueComputeCommandAllocators[0] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                mapQueueComputeCommandBuffers[0] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                RenderDriver::Common::CommandAllocatorDescriptor computeCommandAllocatorDesc;
                computeCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                mapQueueComputeCommandAllocators[0]->create(computeCommandAllocatorDesc, *mpDevice);
                mapQueueComputeCommandAllocators[0]->reset();

                std::ostringstream computeCommandAllocatorName;
                computeCommandAllocatorName << "Compute Command Allocator 0";
                mapQueueComputeCommandAllocators[0]->setID(computeCommandAllocatorName.str());

                RenderDriver::Common::CommandBufferDescriptor computeCommandBufferDesc;
                computeCommandBufferDesc.mpCommandAllocator = mapQueueComputeCommandAllocators[0].get();
                computeCommandBufferDesc.mpPipelineState = nullptr;
                computeCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                mapQueueComputeCommandBuffers[0]->create(computeCommandBufferDesc, *mpDevice);

                std::ostringstream computeCommandBufferName;
                computeCommandBufferName << "Compute Command Buffer 0";
                mapQueueComputeCommandBuffers[0]->setID(computeCommandBufferName.str());
                mapQueueComputeCommandBuffers[0]->reset();
            }

            // memory barrier for image to transfer layout
            uint32_t iNumImageTransitions = 0;
            for(auto const& transition : aTransitions)
            {
                RenderDriver::Metal::CImage* pImageMetal = static_cast<RenderDriver::Metal::CImage*>(transition.mpImage);
                
            }

            // native barrier

            mapQueueGraphicsCommandBuffers[0]->close();
            mpGraphicsCommandQueue->execCommandBuffer(
                *mapQueueGraphicsCommandBuffers[0].get(),
                *mpDevice);
        
            // native wait
            
            mapQueueGraphicsCommandBuffers[0]->reset();
        }

        /*
        **
        */
        void CRenderer::initImgui()
        {
            WTFASSERT(0, "Implement me");
        }


        /*
        **
        */
        void CRenderer::platformUploadResourceData(
            RenderDriver::Common::CBuffer& buffer,
            void* pRawSrcData,
            uint64_t iDataSize,
            uint64_t iDestDataOffset)
        {
            char szOutput[256];
            sprintf(szOutput, "Upload Resource %s (%lld bytes)\n",
                buffer.getID().c_str(),
                iDataSize);

            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::Metal::CBuffer>());

            RenderDriver::Metal::CBuffer& uploadBuffer = static_cast<RenderDriver::Metal::CBuffer&>(*mapUploadBuffers.back());
            RenderDriver::Common::BufferDescriptor uploadBufferDesc = {};

            uploadBufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
            uploadBufferDesc.miSize = iDataSize;
            uploadBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferSrc;
            uploadBuffer.create(uploadBufferDesc, *mpDevice);
            uploadBuffer.setID("Upload Buffer");

            uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
            platformCopyCPUToGPUBuffer(
                *mpUploadCommandBuffer,
                &buffer,
                &uploadBuffer,
                pRawSrcData,
                0,
                static_cast<uint32_t>(iDestDataOffset),
                static_cast<uint32_t>(iDataSize),
                iFlags);
            
            uploadBuffer.releaseNativeBuffer();
        }

        /*
        **
        */
        void CRenderer::platformSetComputeDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
            RenderDriver::Metal::CCommandBuffer& commandBufferMetal = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer);
                        
            // set buffer and texture shader resources for compute shader
            id<MTLComputeCommandEncoder> nativeComputeEncoder = commandBufferMetal.getNativeComputeCommandEncoder();
            WTFASSERT(nativeComputeEncoder != nil, "Invalid compute command encoder");
            
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResource = *descriptorSet.getDesc().mpaShaderResources;
            
            std::vector<uint32_t> const& aiLayoutIndices = descriptorSet.getComputeLayoutIndices();
            
            uint32_t iBufferIndex = 0, iTextureIndex = 0;
            for(uint32_t iLayoutIndex = 0; iLayoutIndex < static_cast<uint32_t>(aiLayoutIndices.size()); iLayoutIndex++)
            {
                uint32_t iShaderResourceIndex = UINT32_MAX;
                if(iLayoutIndex < aiLayoutIndices.size())
                {
                    iShaderResourceIndex = aiLayoutIndices[iLayoutIndex];
                }
                
                if(iShaderResourceIndex == UINT32_MAX)
                {
                    id<MTLTexture> nativeImage = (__bridge id<MTLTexture>)mFillerTexture->getNativeImage();
                    [nativeComputeEncoder
                     setTexture: nativeImage
                     atIndex: iBufferIndex];
                    ++iBufferIndex;
                    ++iTextureIndex;
                }
                else
                {
                    auto& shaderResource = aShaderResource[iShaderResourceIndex];
                    if(shaderResource.mExternalResource.mpBuffer)
                    {
                        id<MTLBuffer> nativeShaderBuffer = (__bridge id<MTLBuffer>)shaderResource.mExternalResource.mpBuffer->getNativeBuffer();
                        [nativeComputeEncoder
                         setBuffer: nativeShaderBuffer
                         offset: 0
                         atIndex: iBufferIndex];
                        
                        ++iBufferIndex;
                    }
                    else if(shaderResource.mExternalResource.mpImage)
                    {
                        id<MTLTexture> nativeTexture = (__bridge id<MTLTexture>)shaderResource.mExternalResource.mpImage->getNativeImage();
                        [nativeComputeEncoder
                         setTexture: nativeTexture
                         atIndex: iTextureIndex];
                        
                        ++iTextureIndex;
                    }
                    else
                    {
                        // TODO: set texture sampler
                        if(shaderResource.mName == "sampler" || shaderResource.mName == "textureSampler")
                        {
                            [nativeComputeEncoder
                             setSamplerState: mDefaultNativePointSamplerState
                             atIndex: 0];
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN)
                        {
                            id<MTLBuffer> nativeBuffer = (__bridge id<MTLBuffer>)mFillerBuffer->getNativeBuffer();
                            [nativeComputeEncoder
                             setBuffer: nativeBuffer
                             offset: 0
                             atIndex: iBufferIndex];
                            
                            ++iBufferIndex;
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
                        {
                            id<MTLTexture> nativeTexture = (__bridge id<MTLTexture>)mFillerTexture->getNativeImage();
                            [nativeComputeEncoder
                             setTexture: nativeTexture
                             atIndex: iTextureIndex];
                            
                            ++iTextureIndex;
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT)
                        {
                            // root constant
                            WTFASSERT(shaderResource.mName == "rootConstant", "Buffer In/Out type signifies root constant");
                            
                            RenderDriver::Metal::CDescriptorSet& descriptorSetMetal = static_cast<RenderDriver::Metal::CDescriptorSet&>(descriptorSet);
                            id<MTLBuffer> rootConstantBuffer = descriptorSetMetal.getRootConstant();
                            
                            [nativeComputeEncoder
                             setBuffer: rootConstantBuffer
                             offset: 0
                             atIndex: iBufferIndex];
                            
                            ++iBufferIndex;
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT)
                        {
                            //id<MTLBuffer> nativeShaderBuffer = (__bridge id<MTLBuffer>)shaderResource.mShaderResourceName
                            //[nativeComputeEncoder
                            // setBuffer: rootConstantBuffer
                            // offset: 0
                            // atIndex: iBufferIndex];
                            
                            ++iBufferIndex;
                        }
                        else
                        {
                            
                            WTFASSERT(0, "type %d not handled", shaderResource.mType);
                        }
                        
                    }
                }
                
                //++iShaderIndex;
            }
        }

        /*
        **
        */
        void CRenderer::platformSetRayTraceDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
            RenderDriver::Metal::CCommandBuffer& commandBufferMetal = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer);
        
            std::string saveDir = getSaveDir();
            
            // set buffer and texture shader resources for compute shader
            id<MTLComputeCommandEncoder> nativeComputeEncoder = commandBufferMetal.getNativeComputeCommandEncoder();
            WTFASSERT(nativeComputeEncoder != nil, "Invalid compute command encoder");
            
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResource = *descriptorSet.getDesc().mpaShaderResources;
            
            std::vector<uint32_t> const& aiLayoutIndices = descriptorSet.getComputeLayoutIndices();
            
            uint32_t iBufferIndex = 0, iTextureIndex = 0;
            for(uint32_t iLayoutIndex = 0; iLayoutIndex < static_cast<uint32_t>(aiLayoutIndices.size()); iLayoutIndex++)
            {
                uint32_t iShaderResourceIndex = UINT32_MAX;
                if(iLayoutIndex < aiLayoutIndices.size())
                {
                    iShaderResourceIndex = aiLayoutIndices[iLayoutIndex];
                }
                
                if(iShaderResourceIndex == UINT32_MAX)
                {
                    id<MTLTexture> nativeImage = (__bridge id<MTLTexture>)mFillerTexture->getNativeImage();
                    [nativeComputeEncoder
                     setTexture: nativeImage
                     atIndex: iBufferIndex];
                    ++iBufferIndex;
                    ++iTextureIndex;
                }
                else
                {
                    auto& shaderResource = aShaderResource[iShaderResourceIndex];
                    if(shaderResource.mExternalResource.mpBuffer)
                    {
                        id<MTLBuffer> nativeShaderBuffer = (__bridge id<MTLBuffer>)shaderResource.mExternalResource.mpBuffer->getNativeBuffer();
                        [nativeComputeEncoder
                         setBuffer: nativeShaderBuffer
                         offset: 0
                         atIndex: iBufferIndex];
                        
                        ++iBufferIndex;
                    }
                    else if(shaderResource.mExternalResource.mpImage)
                    {
                        id<MTLTexture> nativeTexture = (__bridge id<MTLTexture>)shaderResource.mExternalResource.mpImage->getNativeImage();
                        [nativeComputeEncoder
                         setTexture: nativeTexture
                         atIndex: iTextureIndex];
                        
                        ++iTextureIndex;
                    }
                    else
                    {
                        // TODO: set texture sampler
                        if(shaderResource.mName == "sampler" || shaderResource.mName == "textureSampler")
                        {
                            [nativeComputeEncoder
                             setSamplerState: mDefaultNativePointSamplerState
                             atIndex: 0];
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN)
                        {
                            id<MTLBuffer> nativeBuffer = (__bridge id<MTLBuffer>)mFillerBuffer->getNativeBuffer();
                            [nativeComputeEncoder
                             setBuffer: nativeBuffer
                             offset: 0
                             atIndex: iBufferIndex];
                            
                            ++iBufferIndex;
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
                        {
                            id<MTLTexture> nativeTexture = (__bridge id<MTLTexture>)mFillerTexture->getNativeImage();
                            [nativeComputeEncoder
                             setTexture: nativeTexture
                             atIndex: iTextureIndex];
                            
                            ++iTextureIndex;
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT)
                        {
                            // root constant
                            WTFASSERT(shaderResource.mName == "rootConstant", "Buffer In/Out type signifies root constant");
                            
                            RenderDriver::Metal::CDescriptorSet& descriptorSetMetal = static_cast<RenderDriver::Metal::CDescriptorSet&>(descriptorSet);
                            id<MTLBuffer> rootConstantBuffer = descriptorSetMetal.getRootConstant();
                            
                            [nativeComputeEncoder
                             setBuffer: rootConstantBuffer
                             offset: 0
                             atIndex: iBufferIndex];
                            
                            ++iBufferIndex;
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT)
                        {
                            //id<MTLBuffer> nativeShaderBuffer = (__bridge id<MTLBuffer>)shaderResource.mShaderResourceName
                            //[nativeComputeEncoder
                            // setBuffer: rootConstantBuffer
                            // offset: 0
                            // atIndex: iBufferIndex];
                            
                            ++iBufferIndex;
                        }
                        else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_ACCELERATION_STRUCTURE)
                        {
                            id<MTLAccelerationStructure> nativeAccelerationStructure = (__bridge id<MTLAccelerationStructure>)shaderResource.mpAccelerationStructure->getNativeAccelerationStructure();
                            [nativeComputeEncoder
                             setAccelerationStructure: nativeAccelerationStructure
                             atBufferIndex: iBufferIndex];
                            
                            ++iBufferIndex;
                        }
                        else
                        {
                            
                            WTFASSERT(0, "type %d not handled", shaderResource.mType);
                        }
                        
                    }
                }
                
                //++iShaderIndex;
            }
            
            
        }

        /*
        **
        */
        void CRenderer::platformSetGraphicsDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
            bool bHasTextures = false;
            id<MTLRenderCommandEncoder> renderCommandEncoder = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer).getNativeRenderCommandEncoder();
            
            std::vector<uint32_t> const& aiVertexLayoutIndices = descriptorSet.getVertexLayoutIndices();
            std::vector<uint32_t> const& aiFragmentLayoutIndices = descriptorSet.getFragmentLayoutIndices();
            
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = *descriptorSet.getDesc().mpaShaderResources;
            std::vector<uint32_t> aiSet(aShaderResources.size());
            
            uint32_t iNumBufferVS = 0, iNumTextureVS = 0, iNumBufferFS = 0, iNumTextureFS = 0;
            for(uint32_t i = 0; i < static_cast<uint32_t>(aiVertexLayoutIndices.size()); i++)
            {
                uint32_t iShaderIndex = aiVertexLayoutIndices[i];
                SerializeUtils::Common::ShaderResourceInfo const& shaderResource = aShaderResources[iShaderIndex];
                if(shaderResource.mExternalResource.mpBuffer)
                {
                    id<MTLBuffer> nativeBuffer = (__bridge id<MTLBuffer>)shaderResource.mExternalResource.mpBuffer->getNativeBuffer();
                    [renderCommandEncoder
                        setVertexBuffer: nativeBuffer
                        offset: 0
                        atIndex: iNumBufferVS];
                    
                    ++iNumBufferVS;
                }
                else if(shaderResource.mExternalResource.mpImage)
                {
                    id<MTLTexture> nativeTexture = (__bridge id<MTLTexture>)shaderResource.mExternalResource.mpImage->getNativeImage();
                    [renderCommandEncoder
                     setVertexTexture: nativeTexture
                     atIndex: iNumTextureVS];
                    
                    ++iNumTextureVS;
                }
            }
            
            for(uint32_t i = 0; i < static_cast<uint32_t>(aiFragmentLayoutIndices.size()); i++)
            {
                uint32_t iShaderIndex = aiFragmentLayoutIndices[i];
                SerializeUtils::Common::ShaderResourceInfo const& shaderResource = aShaderResources[iShaderIndex];
                if(shaderResource.mExternalResource.mpBuffer)
                {
                    id<MTLBuffer> nativeBuffer = (__bridge id<MTLBuffer>)shaderResource.mExternalResource.mpBuffer->getNativeBuffer();
                    [renderCommandEncoder
                        setFragmentBuffer: nativeBuffer
                        offset: 0
                        atIndex: iNumBufferFS];
                    
                    ++iNumBufferFS;
                }
                else if(shaderResource.mExternalResource.mpImage)
                {
                    id<MTLTexture> nativeTexture = (__bridge id<MTLTexture>)shaderResource.mExternalResource.mpImage->getNativeImage();
                    [renderCommandEncoder
                     setFragmentTexture: nativeTexture
                     atIndex: iNumTextureFS];
                    
                    ++iNumTextureFS;
                    
                    bHasTextures = true;
                }
            }
            
            // add samplers to the end
            if(bHasTextures)
            {
                [renderCommandEncoder
                 setFragmentSamplerState: mDefaultNativeLinearSamplerState
                 atIndex: 0];
                
                [renderCommandEncoder
                 setFragmentSamplerState: mDefaultNativePointSamplerState
                 atIndex: 1];
            }
        }

        /*
        **
        */
        void CRenderer::platformSetGraphicsRoot32BitConstant(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iValue,
            uint32_t iRootParameterIndex,
            uint32_t iOffset)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformSetGraphicsRoot32BitConstants(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            void const* paValues,
            uint32_t iNumValues,
            uint32_t iRootParameterIndex,
            uint32_t iOffsetIn32Bits)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformSetComputeRoot32BitConstant(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iValue,
            uint32_t iRootParameterIndex,
            uint32_t iOffset)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformSetComputeRoot32BitConstants(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState,
            void const* paValues,
            uint32_t iNumValues,
            uint32_t iRootParameterIndex,
            uint32_t iOffsetIn32Bits)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformSetDescriptorHeap(
            RenderDriver::Common::CDescriptorHeap& descriptorHeap,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            // no descriptor heap
        }

        /*
        **
        */
        void CRenderer::platformSetResourceViews(
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CDescriptorHeap& descriptorHeap,
            uint32_t iTripleBufferIndex,
            Render::Common::JobType jobType)
        {
            // no resource views, has been set in descriptor function
        }

        /*
        **
        */
        void CRenderer::platformSetViewportAndScissorRect(
            uint32_t iWidth,
            uint32_t iHeight,
            float fMaxDepth,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            // viewport
            MTLViewport viewport;
            viewport.originX = 0.0;
            viewport.originY = 0.0;
            viewport.width = iWidth;
            viewport.height = iHeight;
            viewport.zfar = fMaxDepth;
            viewport.znear = -1.0;
            
            id<MTLRenderCommandEncoder> nativeRenderCommandEncoder = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer).getNativeRenderCommandEncoder();
            [nativeRenderCommandEncoder setViewport: viewport];
        }

        /*
        **
        */
        void CRenderer::platformSetRenderTargetAndClear2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            std::vector<RenderDriver::Common::CDescriptorHeap*>& apRenderTargetDescriptorHeaps,
            std::vector<RenderDriver::Common::CDescriptorHeap*>& apDepthStencilDescriptorHeaps,
            uint32_t iNumRenderTargetAttachments,
            std::vector<std::vector<float>> const& aafClearColors,
            std::vector<bool> const& abClear)
        {
            // already set in the render pass descriptor passed in for render command encoder
        }

        /*
        **
        */
        void CRenderer::platformCopyImageToCPUMemory(
            RenderDriver::Common::CImage* pGPUImage,
            std::vector<float>& afImageData)
        {
            WTFASSERT(0, "Implement me");
        }

        std::mutex sMutex;

        /*
        **
        */
        void CRenderer::platformCopyBufferToCPUMemory(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pCPUBuffer,
            uint64_t iSrcOffset,
            uint64_t iDataSize)
        {
            /*if(mpReadBackBuffer == nullptr)
            {
                mpReadBackBuffer = std::make_unique<RenderDriver::Metal::CBuffer>();
                RenderDriver::Common::BufferDescriptor desc;
                desc.miSize = iDataSize;
                desc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
                desc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferDest;
                mpReadBackBuffer->create(
                    desc,
                    *mpDevice
                );
            }*/
            
            id<MTLBuffer> nativeBuffer = (__bridge id<MTLBuffer>)pGPUBuffer->getNativeBuffer();
            
            RenderDriver::Metal::CBuffer readBackBuffer;
            RenderDriver::Common::BufferDescriptor desc;
            desc.miSize = iDataSize;
            desc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            desc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferSrc;
            readBackBuffer.create(desc, *mpDevice);
            readBackBuffer.setID("Read Back Buffer");
            
            mpUploadCommandBuffer->reset();
            readBackBuffer.copy(*pGPUBuffer, *mpUploadCommandBuffer, 0, 0, iDataSize);
            mpUploadCommandBuffer->close();
            
            mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)mpUploadCommandBuffer->getNativeCommandList();
            [nativeCommandBuffer waitUntilCompleted];
            
            id<MTLBuffer> nativeReadBackBuffer = (__bridge id<MTLBuffer>)readBackBuffer.getNativeBuffer();
            void* pContent = [nativeReadBackBuffer contents];
            memcpy(pCPUBuffer, ((uint8_t*)pContent) + iSrcOffset, iDataSize);
            
            readBackBuffer.releaseNativeBuffer();
            mpUploadCommandBuffer->reset();
        
        }

        /*
        **
        */
        void CRenderer::platformCopyBufferToCPUMemory2(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pCPUBuffer,
            uint64_t iSrcOffset,
            uint64_t iDataSize,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformUpdateTextureInArray(
            RenderDriver::Common::CImage& image,
            void const* pRawSrcData,
            uint32_t iImageWidth,
            uint32_t iImageHeight,
            RenderDriver::Common::Format const& format,
            uint32_t iTextureArrayIndex,
            uint32_t iBaseDataSize)
        {
            WTFASSERT(0, "Implement me");
            
            RenderDriver::Metal::CDevice* pDeviceMetal = static_cast<RenderDriver::Metal::CDevice*>(mpDevice.get());

            RenderDriver::Metal::CImage& imageMetal = static_cast<RenderDriver::Metal::CImage&>(image);

            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::Metal::CBuffer>());
            RenderDriver::Metal::CBuffer& uploadBuffer = static_cast<RenderDriver::Metal::CBuffer&>(*mapUploadBuffers.back());

            uint32_t iNumBaseComponents = SerializeUtils::Common::getNumComponents(format);
            uint32_t iImageSize = iImageWidth * iImageHeight * iNumBaseComponents * iBaseDataSize;

            RenderDriver::Common::BufferDescriptor uploadBufferDesc =
            {
                /* .miSize          */      iImageSize,
                /* .mFormat         */      RenderDriver::Common::Format::UNKNOWN,
                /* .mFlags          */      RenderDriver::Common::ResourceFlagBits::None,
                /* .mHeapType       */      RenderDriver::Common::HeapType::Upload,
                /* .mOverrideState  */      RenderDriver::Common::ResourceStateFlagBits::None,
                /* .mpCounterBuffer */      nullptr,
            };
            uploadBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferSrc;
            uploadBuffer.create(uploadBufferDesc, *mpDevice);
            uploadBuffer.setID("Upload Image Buffer");

            uploadBuffer.setData((void*)pRawSrcData, iImageSize);

            RenderDriver::Common::CommandBufferState const& commandBufferState = mpUploadCommandBuffer->getState();
            if(commandBufferState == RenderDriver::Common::CommandBufferState::Closed)
            {
                mpUploadCommandBuffer->reset();
            }
    
            mpUploadCommandBuffer->close();

            RenderDriver::Metal::CCommandQueue* pCopyCommandQueueMetal = static_cast<RenderDriver::Metal::CCommandQueue*>(mpCopyCommandQueue.get());

            
            RenderDriver::Metal::CFence* pUploadFenceMetal = static_cast<RenderDriver::Metal::CFence*>(mpUploadFence.get());
            
            ++miCopyCommandFenceValue;
            uint64_t iCopyCommandFenceValue = UINT64_MAX;
#if 0
            if(*pSemaphore)
            {
                ++iCopyCommandFenceValue;
                miCopyCommandFenceValue = iCopyCommandFenceValue;
            }
            else
            {
                iCopyCommandFenceValue = miCopyCommandFenceValue;
            }
#endif // #if 0
            
            uint32_t iNumSignalSemaphores = 0;
            mpCopyCommandQueue->execCommandBuffer(
                *mpUploadCommandBuffer, 
                *mpDevice);
        
            mpUploadCommandAllocator->reset();
            mpUploadCommandBuffer->reset();

            miCopyCommandFenceValue = iCopyCommandFenceValue;
        }

        /*
        **
        */
        void CRenderer::platformCopyImage2(
            RenderDriver::Common::CImage& destImage,
            RenderDriver::Common::CImage& srcImage,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            bool bSrcWritable)
        {
            id<MTLTexture> srcTexture = (__bridge id<MTLTexture>)srcImage.getNativeImage();
            id<MTLTexture> destTexture = (__bridge id<MTLTexture>)destImage.getNativeImage();
            
            RenderDriver::Metal::CCommandBuffer& commandBufferMetal = (RenderDriver::Metal::CCommandBuffer&)commandBuffer;
            
            id<MTLBlitCommandEncoder> nativeBlitCommandEncoder = commandBufferMetal.getNativeBlitCommandEncoder();
            WTFASSERT(nativeBlitCommandEncoder != nil, "No blit command encoder");
            [nativeBlitCommandEncoder
                 copyFromTexture: srcTexture
                 toTexture: destTexture];
        }

        /*
        **
        */
        void CRenderer::platformCopyBufferToBuffer(
            RenderDriver::Common::CBuffer* pDestBuffer,
            RenderDriver::Common::CBuffer* pSrcBuffer,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint64_t iDataSize)
        {
            WTFASSERT(0, "Implement me");

        }

        /*
        **
        */
        float3 CRenderer::platformGetWorldFromScreenPosition(
            uint32_t iX,
            uint32_t iY,
            uint32_t iScreenWidth,
            uint32_t iScreenHeight)
        {
            WTFASSERT(0, "Implement me");


            float3 ret(0.0f, 0.0f, 0.0f);
            return ret;
        }

        /*
        **
        */
        uint64_t CRenderer::getTotalVertexBufferGPUAddress()
        {
            WTFASSERT(0, "Implement me");
            return 0;
        }

        /*
        **
        */
        uint64_t CRenderer::getTotalIndexBufferGPUAddress()
        {
            WTFASSERT(0, "Implement me");
            return 0;
        }

        /*
        **
        */
        void CRenderer::platformBeginRenderDebuggerCapture(std::string const& captureFilePath)
        {
            WTFASSERT(0, "Implement me");
            
        }

        /*
        **
        */
        void CRenderer::platformEndRenderDebuggerCapture()
        {
            WTFASSERT(0, "Implement me");
        }

        static uint32_t siNumFrames = 0;

        /*
        **
        */
        void CRenderer::platformBeginRenderDocCapture(std::string const& filePath)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformEndRenderDocCapture()
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformTransitionBarriers(
            RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iNumBarrierInfo,
            bool bReverseState,
            void* pUserData)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCreateRenderJobFences()
        {
            std::string aFenceNames[4] =
            {
                "Graphics Queue Fence",
                "Compute Queue Fence",
                "Copy Queue Fence",
                "Copy GPU Queue Fence",
            };

            mapCommandQueueFences.resize(RenderDriver::Common::CCommandQueue::NumTypes);
            for(uint32_t iFence = 0; iFence < static_cast<uint32_t>(mapCommandQueueFences.size()); iFence++)
            {
                mapCommandQueueFences[iFence] = std::make_unique<RenderDriver::Metal::CFence>();
                RenderDriver::Common::FenceDescriptor desc;
                mapCommandQueueFences[iFence]->create(desc, *mpDevice);
                mapCommandQueueFences[iFence]->setID(aFenceNames[iFence]);
            }
        }

        /*
        **
        */
        void CRenderer::platformPostSetup()
        {
            for(uint32_t i = 0; i < static_cast<uint32_t>(mapQueueGraphicsCommandAllocators.size()); i++)
            {
                if(mapQueueGraphicsCommandAllocators[i] == nullptr)
                {
                    // graphics
                    mapQueueGraphicsCommandAllocators[i] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                    mapQueueGraphicsCommandBuffers[i] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                    RenderDriver::Common::CommandAllocatorDescriptor graphicsCommandAllocatorDesc;
                    graphicsCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                    mapQueueGraphicsCommandAllocators[i]->create(graphicsCommandAllocatorDesc, *mpDevice);

                    std::ostringstream graphicCommandAllocatorName;
                    graphicCommandAllocatorName << "Graphics Command Allocator " << i;
                    mapQueueGraphicsCommandAllocators[i]->setID(graphicCommandAllocatorName.str());
                    mapQueueGraphicsCommandAllocators[i]->reset();

                    RenderDriver::Common::CommandBufferDescriptor graphicsCommandBufferDesc;
                    graphicsCommandBufferDesc.mpCommandAllocator = mapQueueGraphicsCommandAllocators[i].get();
                    graphicsCommandBufferDesc.mpPipelineState = nullptr;
                    graphicsCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                    mapQueueGraphicsCommandBuffers[i]->create(graphicsCommandBufferDesc, *mpDevice);

                    std::ostringstream graphicCommandBufferName;
                    graphicCommandBufferName << "Graphics Command Buffer " << i;
                    mapQueueGraphicsCommandBuffers[i]->setID(graphicCommandBufferName.str());
                    mapQueueGraphicsCommandBuffers[i]->reset();
                }

                if(mapQueueComputeCommandAllocators[i] == nullptr)
                {
                    // compute
                    mapQueueComputeCommandAllocators[i] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                    mapQueueComputeCommandBuffers[i] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                    RenderDriver::Common::CommandAllocatorDescriptor computeCommandAllocatorDesc;
                    computeCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    mapQueueComputeCommandAllocators[i]->create(computeCommandAllocatorDesc, *mpDevice);
                    mapQueueComputeCommandAllocators[i]->reset();

                    std::ostringstream computeCommandAllocatorName;
                    computeCommandAllocatorName << "Compute Command Allocator " << i;
                    mapQueueComputeCommandAllocators[i]->setID(computeCommandAllocatorName.str());

                    RenderDriver::Common::CommandBufferDescriptor computeCommandBufferDesc;
                    computeCommandBufferDesc.mpCommandAllocator = mapQueueComputeCommandAllocators[i].get();
                    computeCommandBufferDesc.mpPipelineState = nullptr;
                    computeCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    mapQueueComputeCommandBuffers[i]->create(computeCommandBufferDesc, *mpDevice);

                    std::ostringstream computeCommandBufferName;
                    computeCommandBufferName << "Compute Command Buffer " << i;
                    mapQueueComputeCommandBuffers[i]->setID(computeCommandBufferName.str());
                    mapQueueComputeCommandBuffers[i]->reset();
                }

                if(mapQueueCopyCommandAllocators[i] == nullptr)
                {
                    // copy
                    mapQueueCopyCommandAllocators[i] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                    mapQueueCopyCommandBuffers[i] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                    RenderDriver::Common::CommandAllocatorDescriptor copyCommandAllocatorDesc;
                    copyCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    mapQueueCopyCommandAllocators[i]->create(copyCommandAllocatorDesc, *mpDevice);
                    mapQueueCopyCommandAllocators[i]->reset();

                    std::ostringstream copyCommandAllocatorName;
                    copyCommandAllocatorName << "Copy Command Allocator " << i;
                    mapQueueCopyCommandAllocators[i]->setID(copyCommandAllocatorName.str());

                    RenderDriver::Common::CommandBufferDescriptor copyCommandBufferDesc;
                    copyCommandBufferDesc.mpCommandAllocator = mapQueueCopyCommandAllocators[i].get();
                    copyCommandBufferDesc.mpPipelineState = nullptr;
                    copyCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    mapQueueCopyCommandBuffers[i]->create(copyCommandBufferDesc, *mpDevice);

                    std::ostringstream copyCommandBufferName;
                    copyCommandBufferName << "Copy Command Buffer " << i;
                    mapQueueCopyCommandBuffers[i]->setID(copyCommandBufferName.str());
                    mapQueueCopyCommandBuffers[i]->reset();

                    // copy gpu
                    mapQueueGPUCopyCommandAllocators[i] = std::make_shared<RenderDriver::Metal::CCommandAllocator>();
                    mapQueueGPUCopyCommandBuffers[i] = std::make_shared<RenderDriver::Metal::CCommandBuffer>();

                    RenderDriver::Common::CommandAllocatorDescriptor gpuCopyCommandAllocatorDesc;
                    gpuCopyCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    mapQueueGPUCopyCommandAllocators[i]->create(gpuCopyCommandAllocatorDesc, *mpDevice);

                    std::ostringstream gpuCopyCommandAllocatorName;
                    gpuCopyCommandAllocatorName << "GPU Copy Command Allocator " << i;
                    mapQueueGPUCopyCommandAllocators[i]->setID(gpuCopyCommandAllocatorName.str());
                    mapQueueGPUCopyCommandAllocators[i]->reset();

                    RenderDriver::Common::CommandBufferDescriptor gpuCopyCommandBufferDesc;
                    gpuCopyCommandBufferDesc.mpCommandAllocator = mapQueueGPUCopyCommandAllocators[i].get();
                    gpuCopyCommandBufferDesc.mpPipelineState = nullptr;
                    gpuCopyCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    mapQueueGPUCopyCommandBuffers[i]->create(gpuCopyCommandBufferDesc, *mpDevice);

                    std::ostringstream gpuCopyCommandBufferName;
                    gpuCopyCommandBufferName << "GPU Copy Command Buffer " << i;
                    mapQueueGPUCopyCommandBuffers[i]->setID(gpuCopyCommandBufferName.str());
                    mapQueueGPUCopyCommandBuffers[i]->reset();
                }
            }
        }

        /*
        **
        */
        void CRenderer::platformSwapChainMoveToNextFrame()
        {
            // Done in App thread
        }

        /*
        **
        */
        void CRenderer::platformCopyCPUToGPUBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CBuffer* pDestBuffer,
            RenderDriver::Common::CBuffer* pUploadBuffer,
            void* pCPUData,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint32_t iDataSize,
            uint32_t iFlags)
        {
            pUploadBuffer->setData(pCPUData, iDataSize);
            pDestBuffer->copy(
                *pUploadBuffer,
                commandBuffer,
                iDestOffset,
                iSrcOffset,
                iDataSize);
            
            if((iFlags & static_cast<uint32_t>(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY)) > 0)
            {
                platformExecuteCopyCommandBuffer(commandBuffer, iFlags);
                pUploadBuffer->releaseNativeBuffer();
                commandBuffer.reset();
            }
        }

        /*
        **
        */
        void CRenderer::platformCopyCPUToGPUBuffer2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CBuffer* pDestBuffer,
            RenderDriver::Common::CBuffer* pUploadBuffer,
            void* pCPUData,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint32_t iDestDataSize,
            uint32_t iTotalDataSize,
            uint32_t iFlag)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformCopyCPUToGPUBuffer3(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue,
            RenderDriver::Common::CBuffer* pDestBuffer,
            void* pCPUData,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint32_t iDataSize,
            RenderDriver::Common::CBuffer& uploadBuffer)
        {
            WTFASSERT(0, "Implement me");
            
            WTFASSERT(iDataSize < uploadBuffer.getDescriptor().miSize, "Copy size exceeds %d", uploadBuffer.getDescriptor().miSize);
            
            commandBuffer.reset();
            uploadBuffer.setData(pCPUData, iDataSize);
            
            commandBuffer.close();

            commandQueue.execCommandBufferSynchronized(
                commandBuffer,
                *mpDevice
            );
        }

        /*
        **
        */
        void CRenderer::platformExecuteCopyCommandBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iFlag)
        {
            commandBuffer.close();
            mpCopyCommandQueue->execCommandBuffer(
                commandBuffer,
                *mpDevice
            );
            
            RenderDriver::Metal::CFence* pUploadFenceMetal = static_cast<RenderDriver::Metal::CFence*>(mpUploadFence.get());
            
            pUploadFenceMetal->waitCPU2(
                UINT64_MAX,
                mpCopyCommandQueue.get(),
                &commandBuffer);
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker(
            std::string const& name,
            RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
            // no debug marker
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker2(
            std::string const& name,
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker(
            RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
            // no debug marker
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker2(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker3(
            std::string const& name,
            float4 const& color,
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            // no debug marker
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker3(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            // no debug marker
        }

        /*
        **
        */
        void CRenderer::platformInitializeRenderJobs(
            std::vector<std::string> const& aRenderJobNames)
        {
            for(auto const& name : aRenderJobNames)
            {
                maRenderJobs[name] = std::make_unique<Render::Metal::CRenderJob>();
            }

            for(auto const& name : aRenderJobNames)
            {
                mapRenderJobs[name] = maRenderJobs[name].get();
            }
        }

        /*
        **
        */
        void CRenderer::platformCreateRenderJobCommandBuffers(
            std::vector<std::string> const& aRenderJobNames
        )
        {
            for(auto const& renderJobName : aRenderJobNames)
            {                
                maRenderJobCommandBuffers[renderJobName] = std::make_unique<RenderDriver::Metal::CCommandBuffer>();
                maRenderJobCommandAllocators[renderJobName] = std::make_unique<RenderDriver::Metal::CCommandAllocator>();

                // creation descriptors
                RenderDriver::Metal::CommandBufferDescriptor desc;
                RenderDriver::Common::CommandAllocatorDescriptor allocatorDesc;
                desc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                allocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                desc.mpCommandQueue = (__bridge id<MTLCommandQueue>)mpGraphicsCommandQueue->getNativeCommandQueue();
                if(mapRenderJobs[renderJobName]->mType == Render::Common::JobType::Compute)
                {
                    desc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    allocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    desc.mpCommandQueue = (__bridge id<MTLCommandQueue>)mpComputeCommandQueue->getNativeCommandQueue();
                    
                }
                else if(mapRenderJobs[renderJobName]->mType == Render::Common::JobType::Copy)
                {
                    desc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    allocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    desc.mpCommandQueue = (__bridge id<MTLCommandQueue>)mpCopyCommandQueue->getNativeCommandQueue();
                }
                else if(mapRenderJobs[renderJobName]->mType == Render::Common::JobType::RayTrace)
                {
                    desc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    allocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    desc.mpCommandQueue = (__bridge id<MTLCommandQueue>)mpComputeCommandQueue->getNativeCommandQueue();
                }

                // create command buffer allocator
                maRenderJobCommandAllocators[renderJobName]->create(
                    allocatorDesc,
                    *mpDevice
                );
                mapRenderJobCommandAllocators[renderJobName] = maRenderJobCommandAllocators[renderJobName].get();

                // create command buffer
                desc.mpPipelineState = mapRenderJobs[renderJobName]->mpPipelineState;
                desc.mpCommandAllocator = maRenderJobCommandAllocators[renderJobName].get();
                                
                maRenderJobCommandBuffers[renderJobName]->create(
                    desc,
                    *mpDevice
                );
                maRenderJobCommandBuffers[renderJobName]->reset();
                mapRenderJobCommandBuffers[renderJobName] = maRenderJobCommandBuffers[renderJobName].get();
                mapRenderJobCommandBuffers[renderJobName]->setID(renderJobName + " Command Buffer");
            }
        }
    
        /*
        **
        */
        void CRenderer::platformBeginRenderPass2(
            Render::Common::RenderPassDescriptor2 const& renderPassDesc
        )
        {
            RenderDriver::Metal::CPipelineState* pPipelineStateMetal = static_cast<RenderDriver::Metal::CPipelineState*>(renderPassDesc.mpPipelineState);
            MTLRenderPassDescriptor* pNativeRenderPassDescriptor = pPipelineStateMetal->getNativeRenderPassDecriptor();
            
            pNativeRenderPassDescriptor.depthAttachment.texture = nil;
            
            //Render::Metal::RenderPassDescriptor2 const& renderPassDescMetal = static_cast<Render::Metal::RenderPassDescriptor2 const&>(renderPassDesc);
            
            // set output attachment info
            Render::Common::CRenderJob* pRenderJob = (Render::Common::CRenderJob*)renderPassDesc.mpRenderJob;
            bool bSwapChainPass = (pRenderJob->mPassType == Render::Common::PassType::SwapChain);
            uint32_t iAttachmentIndex = 0;
            uint32_t iNumAttachments = (bSwapChainPass) ? 1 : static_cast<uint32_t>(pRenderJob->maAttachmentMappings.size());
            for(uint32_t iAttachment = 0; iAttachment < iNumAttachments; iAttachment++)
            {
                if(pRenderJob->maAttachmentMappings[iAttachment].second != "texture-output" &&
                   pRenderJob->maAttachmentMappings[iAttachment].second != "texture-input-output")
                {
                    continue;
                }
                
                std::string const& name = pRenderJob->maAttachmentMappings[iAttachment].first;
                RenderDriver::Metal::CImage* pImageMetal = (RenderDriver::Metal::CImage*)pRenderJob->mapOutputImageAttachments[name];
                
DEBUG_PRINTF("\toutput attachment %d: \"%s\"\n", iAttachment, name.c_str());
                
                if(name == "Depth Output")
                {
                    // depth attachment
                    pNativeRenderPassDescriptor.depthAttachment.texture = (__bridge id<MTLTexture>)pImageMetal->getNativeImage();
                    pNativeRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
                    pNativeRenderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
                    pNativeRenderPassDescriptor.depthAttachment.clearDepth = 1.0;
                    pNativeRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
                    
                }
                else
                {
                    // set color image attachment to swap chain's drawable
                    if(bSwapChainPass)
                    {
                        pImageMetal = static_cast<RenderDriver::Metal::CSwapChain*>(mpSwapChain.get())->getColorImage();
                    }
                    
                    // color attachment
                    pNativeRenderPassDescriptor.colorAttachments[iAttachmentIndex].texture = (__bridge id<MTLTexture>)pImageMetal->getNativeImage();
                    pNativeRenderPassDescriptor.colorAttachments[iAttachmentIndex].loadAction = MTLLoadActionClear;
                    pNativeRenderPassDescriptor.colorAttachments[iAttachmentIndex].storeAction = MTLStoreActionStore;
                    pNativeRenderPassDescriptor.colorAttachments[iAttachmentIndex].clearColor = MTLClearColorMake(0.0, 0.0, 0.3, 0.0);
                    
                    if(pRenderJob->maAttachmentMappings[iAttachment].second == "texture-input-output")
                    {
                        pNativeRenderPassDescriptor.colorAttachments[iAttachmentIndex].loadAction = MTLLoadActionLoad;
                    }
                    
                    ++iAttachmentIndex;
                }
            }
            
            if(pRenderJob->mpDepthImage != nullptr)
            {
                // depth attachment
                pNativeRenderPassDescriptor.depthAttachment.texture = (__bridge id<MTLTexture>)pRenderJob->mpDepthImage->getNativeImage();
                pNativeRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
                pNativeRenderPassDescriptor.depthAttachment.storeAction = MTLStoreActionStore;
                pNativeRenderPassDescriptor.depthAttachment.clearDepth = 1.0;
                
                if(pRenderJob->mbInputOutputDepth)
                {
                    pNativeRenderPassDescriptor.depthAttachment.loadAction = MTLLoadActionLoad;
                }
            }
            
            pNativeRenderPassDescriptor.renderTargetWidth = renderPassDesc.miOutputWidth;
            pNativeRenderPassDescriptor.renderTargetHeight = renderPassDesc.miOutputHeight;
            pNativeRenderPassDescriptor.renderTargetArrayLength = 1;
            pNativeRenderPassDescriptor.defaultRasterSampleCount = 1;
            
            RenderDriver::Metal::CCommandBuffer* pCommandBufferMetal = static_cast<RenderDriver::Metal::CCommandBuffer*>(renderPassDesc.mpCommandBuffer);
            
            pCommandBufferMetal->beginRenderPass(pNativeRenderPassDescriptor);
            id<MTLRenderCommandEncoder> nativeRenderCommandEncoder = pCommandBufferMetal->getNativeRenderCommandEncoder();
            nativeRenderCommandEncoder.label = [NSString stringWithUTF8String: std::string(pRenderJob->mName + " Render Command Encoder").c_str()];
            
            [nativeRenderCommandEncoder setDepthStencilState: pPipelineStateMetal->getNativeDepthStencilState()];
            
            [nativeRenderCommandEncoder setFrontFacingWinding: MTLWindingCounterClockwise];
            
            
        }

        /*
        **
        */
        void CRenderer::platformEndRenderPass2(
            Render::Common::RenderPassDescriptor2 const& renderPassDesc
        )
        {
            // Nothing to be done
            
        }

        /*
        **
        */
        void CRenderer::platformCreateVertexAndIndexBuffer(
            std::string const& name,
            uint32_t iVertexBufferSize,
            uint32_t iIndexBufferSize
        )
        {
            maVertexBuffers[name] = std::make_unique<RenderDriver::Metal::CBuffer>();
            maIndexBuffers[name] = std::make_unique<RenderDriver::Metal::CBuffer>();
            maTrianglePositionBuffers[name] = std::make_unique<RenderDriver::Metal::CBuffer>();

            RenderDriver::Common::BufferDescriptor vertexBufferCreateInfo;
            vertexBufferCreateInfo.miSize = iVertexBufferSize;
            vertexBufferCreateInfo.mBufferUsage = static_cast<RenderDriver::Common::BufferUsage>(
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::VertexBuffer) |
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::TransferDest) |
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::StorageBuffer)
            );
            maVertexBuffers[name]->create(
                vertexBufferCreateInfo,
                *mpDevice
            );
            maVertexBuffers[name]->setID(name + " Vertex Buffer");
            mapVertexBuffers[name] = maVertexBuffers[name].get();

            RenderDriver::Common::BufferDescriptor indexBufferCreateInfo;
            indexBufferCreateInfo.miSize = iIndexBufferSize;
            indexBufferCreateInfo.mBufferUsage = static_cast<RenderDriver::Common::BufferUsage>(
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::IndexBuffer) |
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::TransferDest) | 
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::StorageBuffer) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly)
            );
            maIndexBuffers[name]->create(
                indexBufferCreateInfo,
                *mpDevice
            );
            maIndexBuffers[name]->setID(name + " Index Buffer");
            mapIndexBuffers[name] = maIndexBuffers[name].get();

            // just the triangle positions
            uint32_t iNumVertexPositions = iVertexBufferSize / (sizeof(float4) * 3);
            RenderDriver::Common::BufferDescriptor positionBufferCreateInfo;
            positionBufferCreateInfo.miSize = iNumVertexPositions * sizeof(float4);
            positionBufferCreateInfo.mBufferUsage = static_cast<RenderDriver::Common::BufferUsage>(
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::VertexBuffer) |
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::TransferDest) |
                static_cast<uint32_t>(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly)
            );
            maTrianglePositionBuffers[name]->create(
                positionBufferCreateInfo,
                *mpDevice
            );
            maTrianglePositionBuffers[name]->setID(name + " Triangle Position Buffer");
            mapTrianglePositionBuffers[name] = maTrianglePositionBuffers[name].get();
        }

        /*
        **
        */
        void CRenderer::platformSetVertexAndIndexBuffers2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            std::string const& meshName)
    {
            id<MTLRenderCommandEncoder> nativeRenderCommandEncoder = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer).getNativeRenderCommandEncoder();
            
            id<MTLBuffer> nativeVertexBuffer = (__bridge id<MTLBuffer>)mapVertexBuffers[meshName]->getNativeBuffer();
            [nativeRenderCommandEncoder setVertexBuffer: nativeVertexBuffer offset: 0 atIndex: 30];
        }

        /*
        **
        */
        void CRenderer::platformCreateSwapChainCommandBuffer()
        {
            WTFASSERT(0, "Implement me");
            
            mSwapChainCommandBuffer = std::make_unique<RenderDriver::Metal::CCommandBuffer>();
            mSwapChainCommandAllocator = std::make_unique<RenderDriver::Metal::CCommandAllocator>();

            RenderDriver::Common::CommandAllocatorDescriptor allocatorDesc;
            allocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
            mSwapChainCommandAllocator->create(
                allocatorDesc,
                *mpDevice
            );

            RenderDriver::Common::CommandBufferDescriptor desc;
            desc.mpCommandAllocator = mSwapChainCommandAllocator.get();
            mSwapChainCommandBuffer->create(
                desc, 
                *mpDevice
            );

            mpSwapChainCommandBuffer = mSwapChainCommandBuffer.get();
        }

        /*
        **
        */
        void CRenderer::platformTransitionBarrier3(
            RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarriers,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iNumBarriers,
            RenderDriver::Common::CCommandQueue::Type const& queueType
        )
        {
            // no barriers
#if 0
            for(uint32_t i = 0; i < iNumBarriers; i++)
            {
                if(aBarriers[i].mpImage == nullptr)
                {
                    continue;
                }

                RenderDriver::Common::Utils::TransitionBarrierInfo const& barrierInfo = aBarriers[i];
                RenderDriver::Metal::CImage* pImageMetal = (RenderDriver::Metal::CImage*)barrierInfo.mpImage;
                
            }
#endif // #if 0
        }

        /*
        **
        */
        void CRenderer::platformTransitionOutputAttachments()
        {
            WTFASSERT(0, "Implement me");
            
            for(auto& renderJobKeyValue : maRenderJobs)
            {
                RenderDriver::Common::CCommandQueue* pGraphicsCommandQueue = mpGraphicsCommandQueue.get();
                RenderDriver::Common::CCommandQueue* pComputeCommandQueue = mpComputeCommandQueue.get();

                for(auto& outputAttachmentKeyValue : renderJobKeyValue.second->mapOutputImageAttachments)
                {
                    if(outputAttachmentKeyValue.second == nullptr)
                    {
                        continue;
                    }
                }
            }
        }

        /*
        **
        */
        void CRenderer::platformCreateAccelerationStructures(
            std::vector<float4> const& aTrianglePositions,
            std::vector<uint32_t> const& aiTriangleIndices,
            std::vector<std::pair<uint32_t, uint32_t>> const& aMeshRanges,
            uint32_t iNumMeshes
        )
        {
            RenderDriver::Metal::CBuffer& vertexBuffer = *((RenderDriver::Metal::CBuffer*)mapVertexBuffers["bistro"]);
            RenderDriver::Metal::CBuffer& indexBuffer = *((RenderDriver::Metal::CBuffer*)mapIndexBuffers["bistro"]);
            
            id<MTLBuffer> nativeVertexBuffer = (__bridge id<MTLBuffer>)vertexBuffer.getNativeBuffer();
            id<MTLBuffer> nativeIndexBuffer = (__bridge id<MTLBuffer>)indexBuffer.getNativeBuffer();
            
            MTLAccelerationStructureTriangleGeometryDescriptor* pGeometryDesc = [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];
            pGeometryDesc.indexBuffer = nativeIndexBuffer;
            pGeometryDesc.indexType = MTLIndexTypeUInt16;
            pGeometryDesc.vertexBuffer = nativeVertexBuffer;
            pGeometryDesc.vertexStride = sizeof(Render::Common::VertexFormat);
            pGeometryDesc.triangleCount = aiTriangleIndices.size() / 3;
            
            MTLPrimitiveAccelerationStructureDescriptor *accelDescriptor = [MTLPrimitiveAccelerationStructureDescriptor descriptor];
            accelDescriptor.geometryDescriptors = @[ pGeometryDesc ];

            id<MTLDevice> nativeDevice = (__bridge id<MTLDevice>)mpDevice->getNativeDevice();
            id<MTLCommandQueue> nativeQueue = (__bridge id<MTLCommandQueue>)mpComputeCommandQueue->getNativeCommandQueue();
            
            // Query for the sizes needed to store and build the acceleration structure.
            MTLAccelerationStructureSizes accelSizes = [nativeDevice accelerationStructureSizesWithDescriptor:accelDescriptor];
            mAccelerationStructure = [nativeDevice newAccelerationStructureWithSize:accelSizes.accelerationStructureSize];
            mAccelerationStructure.label = @"Acceleration Structure";
            
            // Allocate scratch space Metal uses to build the acceleration structure.
            id <MTLBuffer> scratchBuffer = [nativeDevice newBufferWithLength:accelSizes.buildScratchBufferSize options:MTLResourceStorageModePrivate];

            // command buffer and encoder to build the acceleration structure
            id <MTLCommandBuffer> commandBuffer = [nativeQueue commandBuffer];
            id <MTLAccelerationStructureCommandEncoder> commandEncoder = [commandBuffer accelerationStructureCommandEncoder];

            // Allocate a buffer for Metal to write the compacted accelerated structure's size into.
            id <MTLBuffer> compactedSizeBuffer = [nativeDevice newBufferWithLength:sizeof(uint32_t) options:MTLResourceStorageModeShared];

            // Schedule the actual acceleration structure build.
            [commandEncoder buildAccelerationStructure:mAccelerationStructure
                                            descriptor:accelDescriptor
                                         scratchBuffer:scratchBuffer
                                   scratchBufferOffset:0];

            // Compute and write the compacted acceleration structure size into the buffer. You
            // must already have a built acceleration structure because Metal determines the compacted
            // size based on the final size of the acceleration structure. Compacting an acceleration
            // structure can potentially reclaim significant amounts of memory because Metal must
            // create the initial structure using a conservative approach.

            [commandEncoder writeCompactedAccelerationStructureSize:mAccelerationStructure
                                                           toBuffer:compactedSizeBuffer
                                                             offset:0];

            // End encoding, and commit the command buffer so the GPU can start building the
            // acceleration structure.
            [commandEncoder endEncoding];

            [commandBuffer commit];

            // The sample waits for Metal to finish executing the command buffer so that it can
            // read back the compacted size.

            // Note: Don't wait for Metal to finish executing the command buffer if you aren't compacting
            // the acceleration structure, as doing so requires CPU/GPU synchronization. You don't have
            // to compact acceleration structures, but do so when creating large static acceleration
            // structures, such as static scene geometry. Avoid compacting acceleration structures that
            // you rebuild every frame, as the synchronization cost may be significant.

            [commandBuffer waitUntilCompleted];

            uint32_t compactedSize = *(uint32_t *)compactedSizeBuffer.contents;

            // Allocate a smaller acceleration structure based on the returned size.
            mCompactedAccelerationStructure = [nativeDevice newAccelerationStructureWithSize:compactedSize];
            mCompactedAccelerationStructure.label = @"Compacted Acceleration Structure";
            
            // Create another command buffer and encoder.
            commandBuffer = [nativeQueue commandBuffer];
            commandEncoder = [commandBuffer accelerationStructureCommandEncoder];

            // Encode the command to copy and compact the acceleration structure into the
            // smaller acceleration structure.
            [commandEncoder copyAndCompactAccelerationStructure:mAccelerationStructure
                                        toAccelerationStructure:mCompactedAccelerationStructure];

            // End encoding and commit the command buffer. You don't need to wait for Metal to finish
            // executing this command buffer as long as you synchronize any ray-intersection work
            // to run after this command buffer completes. The sample relies on Metal's default
            // dependency tracking on resources to automatically synchronize access to the new
            // compacted acceleration structure.
            [commandEncoder endEncoding];
            [commandBuffer commit];
            
            [commandBuffer waitUntilCompleted];
            
            maAccelerationStructures["bistro"] = std::make_unique<RenderDriver::Metal::CAccelerationStructure>();
            maAccelerationStructures["bistro"]->setNativeAccelerationStructure((__bridge void*)mCompactedAccelerationStructure);

            mapAccelerationStructures["bistro"] = maAccelerationStructures["bistro"].get();
            
            /*
            // Load the shaders from default library
            NSError *error;
            std::string computeShaderFilePath = std::string(getSaveDir()) + "/shader-output/ray-trace-shadow.metallib";
            DEBUG_PRINTF("%s\n", computeShaderFilePath.c_str());
            FILE* fp = fopen(computeShaderFilePath.c_str(), "rb");
            WTFASSERT(fp, "can\'t open file \"%s\"", computeShaderFilePath.c_str());
            fseek(fp, 0, SEEK_END);
            uint64_t iShaderFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<char> acShaderFileContent(iShaderFileSize);
            fread(acShaderFileContent.data(), sizeof(char), iShaderFileSize, fp);
            fclose(fp);
            dispatch_data_t shaderData = dispatch_data_create(
                acShaderFileContent.data(),
                iShaderFileSize,
                dispatch_get_main_queue(),
                DISPATCH_DATA_DESTRUCTOR_DEFAULT);
            id<MTLLibrary> mRayTraceLibrary = [
                nativeDevice newLibraryWithData:
                    shaderData error: &error];
            
            id<MTLFunction> rayTracingFunction = [mRayTraceLibrary newFunctionWithName:@"rayGen"];
            mRayTracingShadowComputePipeline = [nativeDevice
                newComputePipelineStateWithFunction:rayTracingFunction
                error:&error];

            int iDebug = 1;
            */
            
        }

        /*
        **
        */
        void CRenderer::platformRayTraceCommand(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iScreenWidth,
            uint32_t iScreenHeight
        )
        {
            WTFASSERT(0, "Implement me");
        }

        /*
        **
        */
        void CRenderer::platformRayTraceShaderSetup(
            Render::Common::CRenderJob* pRenderJob
        )
        {
            // nothing to do
        }

        /*
        **
        */
        void CRenderer::platformCopyTexturePageToAtlas(
            char const* pImageData,
            RenderDriver::Common::CImage* pDestImage,
            uint2 const& pageCoord,
            uint32_t iTexturePageDimension)
        {
            WTFASSERT(0, "Implement me");
            
            RenderDriver::Metal::CDevice* pDeviceMetal = static_cast<RenderDriver::Metal::CDevice*>(mpDevice.get());

            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::Metal::CBuffer>());
            RenderDriver::Metal::CBuffer& uploadBuffer = static_cast<RenderDriver::Metal::CBuffer&>(*mapUploadBuffers.back());

            uint32_t iImageSize = iTexturePageDimension * iTexturePageDimension * 4;
            RenderDriver::Common::BufferDescriptor uploadBufferDesc =
            {
                /* .miSize          */      iImageSize,
                /* .mFormat         */      RenderDriver::Common::Format::UNKNOWN,
                /* .mFlags          */      RenderDriver::Common::ResourceFlagBits::None,
                /* .mHeapType       */      RenderDriver::Common::HeapType::Upload,
                /* .mOverrideState  */      RenderDriver::Common::ResourceStateFlagBits::None,
                /* .mpCounterBuffer */      nullptr,
            };
            uploadBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferSrc;
            uploadBuffer.create(uploadBufferDesc, *mpDevice);
            uploadBuffer.setID("Upload Image Buffer");

            uploadBuffer.setData((void*)pImageData, iImageSize);

            // set copy region
            

            // native buffer and image
            
            // prepare command buffer
            RenderDriver::Common::CommandBufferState const& commandBufferState = mpUploadCommandBuffer->getState();
            if(commandBufferState == RenderDriver::Common::CommandBufferState::Closed)
            {
                mpUploadCommandBuffer->reset();
            }

            // previous layout
            

            // transition to dest optimal
            

            // do the copy
            
            // transition back

            
            mpUploadCommandBuffer->close();

            mpCopyCommandQueue->execCommandBufferSynchronized(
                *mpUploadCommandBuffer,
                *mpDevice);

            RenderDriver::Metal::CCommandQueue* pCopyCommandQueueMetal = static_cast<RenderDriver::Metal::CCommandQueue*>(mpCopyCommandQueue.get());
            
            mpUploadCommandAllocator->reset();
            mpUploadCommandBuffer->reset();
        }

        /*
        **
        */
        void CRenderer::platformCopyTexturePageToAtlas2(
            char const* pImageData,
            RenderDriver::Common::CImage* pDestImage,
            uint2 const& pageCoord,
            uint32_t iTexturePageDimension,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue,
            RenderDriver::Common::CBuffer& uploadBuffer)
        {
            WTFASSERT(0, "Implement me");
            
            RenderDriver::Metal::CDevice* pDeviceMetal = static_cast<RenderDriver::Metal::CDevice*>(mpDevice.get());

            // recording
            commandBuffer.reset();

            // set copy region
            

            // do the copy
            

            // close and execute
            commandBuffer.close();
            commandQueue.execCommandBufferSynchronized(
                commandBuffer,
                *mpDevice);

            commandBuffer.reset();

        }

        /*
        **
        */
        void CRenderer::platformCreateCommandBuffer(
            std::unique_ptr<RenderDriver::Common::CCommandAllocator>& threadCommandAllocator,
            std::unique_ptr<RenderDriver::Common::CCommandBuffer>& threadCommandBuffer)
        {
            WTFASSERT(0, "Implement me");
            
            threadCommandAllocator = std::make_unique<RenderDriver::Metal::CCommandAllocator>();
            threadCommandBuffer = std::make_unique<RenderDriver::Metal::CCommandBuffer>();
        }

        /*
        **
        */
        void CRenderer::platformCreateBuffer(
            std::unique_ptr<RenderDriver::Common::CBuffer>& buffer,
            uint32_t iSize)
        {
            WTFASSERT(0, "Implement me");
            
            buffer = std::make_unique<RenderDriver::Metal::CBuffer>();
            RenderDriver::Common::BufferDescriptor bufferDesc = {};
            bufferDesc.miSize = 64 * 64 * 4;
            bufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferDest;
            bufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
            buffer->create(
                bufferDesc,
                *mpDevice);
        }

        /*
        **
        */
        void CRenderer::platformCreateCommandQueue(
            std::unique_ptr<RenderDriver::Common::CCommandQueue>& commandQueue,
            RenderDriver::Common::CCommandQueue::Type const& type)
        {
            WTFASSERT(0, "Implement me");
            
            commandQueue = std::make_unique<RenderDriver::Metal::CCommandQueue>();
            RenderDriver::Common::CCommandQueue::CreateDesc commandQueueDesc = {};
            commandQueueDesc.mpDevice = mpDevice.get();
            commandQueueDesc.mType = type;
            commandQueue->create(commandQueueDesc);
        }

        /*
        **
        */
        void CRenderer::platformTransitionInputImageAttachments(
            Render::Common::CRenderJob* pRenderJob,
            std::vector<char>& acPlatformAttachmentInfo,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            bool bReverse)
        {
            // no need to transition image barriers
        }

        /*
        **
        */
        void CRenderer::platformTransitionOutputAttachmentsRayTrace(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            WTFASSERT(0, "Implement me");
            
            for(auto& outputAttachmentKeyValue : pRenderJob->mapOutputImageAttachments)
            {
                if(outputAttachmentKeyValue.second == nullptr)
                {
                    continue;
                }

                RenderDriver::Metal::CImage* pImageMetal = (RenderDriver::Metal::CImage*)outputAttachmentKeyValue.second;
                
            }
        }

        /*
        **
        */
        void CRenderer::setAttachmentImage(
            std::string const& dstRenderJobName,
            std::string const& dstAttachmentName,
            std::string const& srcRenderJobName,
            std::string const& srcAttachmentName)
        {
            WTFASSERT(0, "Implement me");
            
            Render::Common::CRenderJob* pDstRenderJob = mapRenderJobs[dstRenderJobName];

            Render::Common::CRenderJob* pSrcRenderJob = mapRenderJobs[srcRenderJobName];
            RenderDriver::Common::CImage* pSrcImage = pSrcRenderJob->mapOutputImageAttachments[srcAttachmentName];
            RenderDriver::Metal::CImage* pSrcImageMetal = (RenderDriver::Metal::CImage*)pSrcImage;
            
            pDstRenderJob->mapOutputImageAttachments[dstAttachmentName] = pSrcImage;

            // update descriptor set
        }
    
        /*
        **
        */
        void CRenderer::platformBeginComputePass(
            Render::Common::CRenderJob& renderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer
        )
        {
            static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer).beginComputePass(nullptr);
                        
            RenderDriver::Metal::CCommandBuffer& commandBufferMetal = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer);
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)commandBufferMetal.getNativeCommandList();
            id<MTLComputeCommandEncoder> computeCommandEncoder = commandBufferMetal.getNativeComputeCommandEncoder();
            
            [nativeCommandBuffer setLabel: [NSString stringWithUTF8String: commandBuffer.getID().c_str()]];
            [computeCommandEncoder setLabel: [NSString stringWithUTF8String: std::string(renderJob.mName + " Compute Command Encoder").c_str()]];
        }
        
        /*
        **
        */
        void CRenderer::platformBeginCopyPass(
            Render::Common::CRenderJob& renderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer).beginCopy();
            
            RenderDriver::Metal::CCommandBuffer& commandBufferMetal = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer);
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)commandBufferMetal.getNativeCommandList();
            id<MTLBlitCommandEncoder> blitCommandEncoder = commandBufferMetal.getNativeBlitCommandEncoder();
            
            [nativeCommandBuffer setLabel: [NSString stringWithUTF8String: commandBuffer.getID().c_str()]];
            [blitCommandEncoder setLabel: [NSString stringWithUTF8String: std::string(renderJob.mName + " Blit Command Encoder").c_str()]];
        }
        
        /*
        **
        */
        void CRenderer::platformBeginRayTracingPass(
            Render::Common::CRenderJob& renderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer
        )
        {
            static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer).beginComputePass(nullptr);
                        
            RenderDriver::Metal::CCommandBuffer& commandBufferMetal = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer);
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)commandBufferMetal.getNativeCommandList();
            id<MTLComputeCommandEncoder> computeCommandEncoder = commandBufferMetal.getNativeComputeCommandEncoder();
            
            [nativeCommandBuffer setLabel: [NSString stringWithUTF8String: commandBuffer.getID().c_str()]];
            [computeCommandEncoder setLabel: [NSString stringWithUTF8String: std::string(renderJob.mName + " Compute Command Encoder").c_str()]];
        }
    
        /*
        **
        */
        void CRenderer::platformPreSwapChainPassSubmission(
            Render::Common::CRenderJob const& renderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            id<MTLDrawable> nativeDrawble = static_cast<RenderDriver::Metal::CSwapChain*>(mpSwapChain.get())->getNativeDrawable();
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)commandBuffer.getNativeCommandList();
            [nativeCommandBuffer presentDrawable: nativeDrawble];
        }

        /*
        **
        */
        void CRenderer::platformBeginIndirectCommandBuffer(
           Render::Common::CRenderJob& renderJob,
           RenderDriver::Common::CCommandQueue& commandQueue)
        {
            // TODO: set correct num meshes
            
            id<MTLCommandQueue> nativeCommandQueue = (__bridge id<MTLCommandQueue>)commandQueue.getNativeCommandQueue();
            id<MTLCommandBuffer> nativeCommandBuffer = [nativeCommandQueue commandBuffer];
            
            id<MTLBuffer> meshIndexRangeBuffer = (__bridge id<MTLBuffer>)renderJob.mapUniformBuffers["meshIndexRanges"]->getNativeBuffer();
            id<MTLBlitCommandEncoder> resetBlitEncoder = [nativeCommandBuffer blitCommandEncoder];
            resetBlitEncoder.label = @"Reset ICB Blit Encoder";
            [resetBlitEncoder resetCommandsInBuffer:mGenerateIndirectDrawCommandBuffer
                                          withRange:NSMakeRange(0, 4096)];
            [resetBlitEncoder endEncoding];
            
            id<MTLBuffer> indexBuffer = (__bridge id<MTLBuffer>)mapIndexBuffers["bistro"]->getNativeBuffer();
            id<MTLBuffer> vertexBuffer = (__bridge id<MTLBuffer>)mapVertexBuffers["bistro"]->getNativeBuffer();
            id<MTLBuffer> visibleMeshBuffer = (__bridge id<MTLBuffer>)maRenderJobs["Mesh Culling Compute"]->mapOutputBufferAttachments["Visible Mesh ID"]->getNativeBuffer();
            
            id<MTLComputeCommandEncoder> computeEncoder = [nativeCommandBuffer computeCommandEncoder];
            computeEncoder.label = @"Generate Draw Command Encoder";
            [computeEncoder setBuffer: meshIndexRangeBuffer offset:0 atIndex:0];
            [computeEncoder setBuffer: mIndirectDrawCommandArgumentBuffer offset:0 atIndex:1];
            [computeEncoder setBuffer: visibleMeshBuffer offset:0 atIndex:2];
            [computeEncoder setBuffer: vertexBuffer offset:0 atIndex:3];
            [computeEncoder setBuffer: indexBuffer offset:0 atIndex:4];
            [computeEncoder setComputePipelineState:mIndirectDrawCommandComputePipeline];
            [computeEncoder useResource:mGenerateIndirectDrawCommandBuffer usage:MTLResourceUsageWrite];
            [computeEncoder dispatchThreads:MTLSizeMake(2500, 1, 1)
                      threadsPerThreadgroup:MTLSizeMake(256, 1, 1)];
            [computeEncoder endEncoding];
            
            [nativeCommandBuffer commit];
            [nativeCommandBuffer waitUntilCompleted];
        }
    
    }   // Metal

}   // Render
