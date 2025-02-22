#pragma once

#include <render/Renderer.h>
#include <render-driver/Metal/DeviceMetal.h>
#include <render-driver/Metal/SwapChainMetal.h>
#include <render-driver/Metal/CommandBufferMetal.h>
#include <render-driver/Metal/CommandAllocatorMetal.h>
#include <render-driver/Metal/UtilsMetal.h>
//#include <render-driver/Metal/AccelerationStructureMetal.h>

#include <render/Metal/RenderJobMetal.h>

#include <Metal/Metal.h>

#include <renderdoc/renderdoc_app.h>
#include <mutex>

namespace Render
{
    namespace Metal
    {
        enum class VertexInputs
        {
            Position = 0,
            Normal,
            TexCoord0,

            NUM_VERTEX_INPUTS
        };

        struct RendererDescriptor : public Render::Common::RendererDescriptor
        {
        };

        struct RenderPassDescriptor2 : public Render::Common::RenderPassDescriptor2
        {
            Render::Common::CRenderJob*     mpRenderJob;
        };
    
        class CRenderer : public Render::Common::CRenderer
        {
        public:
            CRenderer() = default;
            virtual ~CRenderer() = default;

            virtual uint64_t getTotalVertexBufferGPUAddress();
            virtual uint64_t getTotalIndexBufferGPUAddress();
            virtual void platformSetup(Render::Common::RendererDescriptor const& desc);

            void transitionBarriers(
                std::vector<RenderDriver::Common::Utils::TransitionBarrierInfo> const& aBarriers);

            void transitionImageLayouts(
                std::vector<RenderDriver::Metal::Utils::ImageLayoutTransition> const& aTransitions);

            void initImgui();
            
#if 0
            void updateImageDescriptors(
                RenderDriver::Common::CDescriptorSet* pDescriptorSet,
                std::vector<VkDescriptorImageInfo> const& aImageInfo,
                std::vector<std::pair<uint32_t, uint32_t>> const& aiBindingIndices,
                std::vector<VkDescriptorType> const& aDescriptorTypes);

            inline VkRenderPass getImguiRenderPass()
            {
                return mImguiRenderPass;
            }
#endif // #if 0
            
            inline void setImguiFrameBufferHandle(PLATFORM_OBJECT_HANDLE handle)
            {
                mImguiFrameBufferHandle = handle;
            }

            virtual void setAttachmentImage(
                std::string const& dstRenderJobName,
                std::string const& dstAttachmentName,
                std::string const& srcRenderJobName,
                std::string const& srcAttachmentName
            );

            RenderDriver::Common::CBuffer* getVertexBuffer(std::string const& name)
            {
                return mapVertexBuffers[name];
            }

            RenderDriver::Common::CBuffer* getIndexBuffer(std::string const& name)
            {
                return mapIndexBuffers[name];
            }

        protected:
            uint32_t                                        miDescriptorHeapOffset;

        protected:
            
            PLATFORM_OBJECT_HANDLE                          mImguiFrameBufferHandle;

#if defined(USE_RAY_TRACING)
            struct ShaderBindingTable
            {
                std::unique_ptr<RenderDriver::Metal::CBuffer>          mBuffer;
                void*                                                   mpMapped;
            };
            std::map<std::string, ShaderBindingTable> maShaderBindingTables;

            std::unique_ptr<RenderDriver::Metal::CBuffer> mpBottomLevelAccelerationStructureBuffer;
            std::unique_ptr<RenderDriver::Metal::CBuffer> mpInstanceBuffer;
            std::unique_ptr<RenderDriver::Metal::CBuffer> mTLASBuffer;
            std::unique_ptr<RenderDriver::Metal::CBuffer> mTransformBuffer;

#endif // USE_RAY_TRACING

        protected:
            std::mutex                                    mUploadDataMutex;

        protected:
            
            virtual void platformBeginRenderPass2(Render::Common::RenderPassDescriptor2 const& renderPassDesc);
            virtual void platformEndRenderPass2(Render::Common::RenderPassDescriptor2 const& renderPassDesc);

            virtual void platformUploadResourceData(
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset);
            
            virtual void platformSetComputeDescriptorSet(
                RenderDriver::Common::CDescriptorSet& descriptorSet,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CPipelineState& pipelineState);

            virtual void platformSetRayTraceDescriptorSet(
                RenderDriver::Common::CDescriptorSet& descriptorSet,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CPipelineState& pipelineState);

            virtual void platformSetGraphicsDescriptorSet(
                RenderDriver::Common::CDescriptorSet& descriptorSet,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CPipelineState& pipelineState);

            virtual void platformSetDescriptorHeap(
                RenderDriver::Common::CDescriptorHeap& descriptorHeap,
                RenderDriver::Common::CCommandBuffer& commandBuffer);

            virtual void platformSetResourceViews(
                std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CDescriptorHeap& descriptorHeap,
                uint32_t iTripleBufferIndex,
                Render::Common::JobType jobType);

            virtual void platformSetViewportAndScissorRect(
                uint32_t iWidth,
                uint32_t iHeight,
                float fMaxDepth,
                RenderDriver::Common::CCommandBuffer& commandBuffer);

            virtual void platformSetGraphicsRoot32BitConstant(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iValue,
                uint32_t iRootParameterIndex,
                uint32_t iOffset);

            void platformSetGraphicsRoot32BitConstants(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                void const* paValues,
                uint32_t iNumValues,
                uint32_t iRootParameterIndex,
                uint32_t iOffsetIn32Bits);

            virtual void platformSetComputeRoot32BitConstant(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iValue,
                uint32_t iRootParameterIndex,
                uint32_t iOffset);

            void platformSetComputeRoot32BitConstants(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CPipelineState& pipelineState,
                void const* paValues,
                uint32_t iNumValues,
                uint32_t iRootParameterIndex,
                uint32_t iOffsetIn32Bits);

            virtual void platformSetRenderTargetAndClear2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apRenderTargetDescriptorHeaps,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apDepthStencilDescriptorHeaps,
                uint32_t iNumRenderTargetAttachments,
                std::vector<std::vector<float>> const& aafClearColors,
                std::vector<bool> const& abClear);

            virtual void platformCopyBufferToCPUMemory(
                RenderDriver::Common::CBuffer* pGPUBuffer,
                void* pCPUBuffer,
                uint64_t iSrcOffset,
                uint64_t iDataSize);

            virtual void platformCopyBufferToCPUMemory2(
                RenderDriver::Common::CBuffer* pGPUBuffer,
                void* pCPUBuffer,
                uint64_t iSrcOffset,
                uint64_t iDataSize,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandQueue& commandQueue
            );

            virtual void platformCopyImageToCPUMemory(
                RenderDriver::Common::CImage* pGPUMemory,
                std::vector<float>& afImageData);

            virtual void platformUpdateTextureInArray(
                RenderDriver::Common::CImage& image,
                void const* pRawSrcData,
                uint32_t iImageWidth,
                uint32_t iImageHeight,
                RenderDriver::Common::Format const& format,
                uint32_t iTextureArrayIndex,
                uint32_t iBaseDataSize);

            virtual void platformCopyImage2(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CImage& srcImage,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                bool bSrcWritable = false);

            virtual void platformCopyBufferToBuffer(
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pSrcBuffer,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint64_t iDataSize);

            virtual float3 platformGetWorldFromScreenPosition(
                uint32_t iX,
                uint32_t iY,
                uint32_t iScreenWidth,
                uint32_t iScreenHeight);

            virtual void platformTransitionBarriers(
                RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iNumBarrierInfo,
                bool bReverseState,
                void* szUserData = nullptr);

            virtual void platformBeginRenderDebuggerCapture(std::string const& captureFilePath);
            virtual void platformEndRenderDebuggerCapture();

            virtual void platformBeginRenderDocCapture(std::string const& captureFilePath);
            virtual void platformEndRenderDocCapture();

            virtual void platformCreateRenderJobFences();

            virtual void platformPostSetup();

            virtual void platformSwapChainMoveToNextFrame();

            virtual void platformCopyCPUToGPUBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pUploadBuffer,
                void* pCPUData,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDataSize,
                uint32_t iFlag = 0);

            virtual void platformCopyCPUToGPUBuffer2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pUploadBuffer,
                void* pCPUData,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDestDataSize,
                uint32_t iTotalDataSize,
                uint32_t iFlag = 0);

            virtual void platformCopyCPUToGPUBuffer3(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandQueue& commandQueue,
                RenderDriver::Common::CBuffer* pDestBuffer,
                void* pCPUData,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDataSize,
                RenderDriver::Common::CBuffer& uploadBuffer);

            virtual void platformExecuteCopyCommandBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iFlag);

            virtual void platformBeginDebugMarker(
                std::string const& name,
                RenderDriver::Common::CCommandQueue* pCommandQueue = nullptr);
            virtual void platformEndDebugMarker(
                RenderDriver::Common::CCommandQueue* pCommandQueue = nullptr);

            virtual void platformBeginDebugMarker2(
                std::string const& name,
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr);
            virtual void platformEndDebugMarker2(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr);

            virtual void platformBeginDebugMarker3(
                std::string const& name,
                float4 const& color,
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr);
            virtual void platformEndDebugMarker3(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr);

            virtual void platformInitializeRenderJobs(
                std::vector<std::string> const& aRenderJobNames);

            virtual void platformCreateRenderJobCommandBuffers(
                std::vector<std::string> const& aRenderJobNames
            );

            virtual void platformCreateVertexAndIndexBuffer(
                std::string const& name,
                uint32_t iVertexBufferSize,
                uint32_t iIndexBufferSize
            );

            virtual void platformSetVertexAndIndexBuffers2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                std::string const& meshName
            );

            virtual void platformCreateSwapChainCommandBuffer();

            virtual void platformTransitionBarrier3(
                RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarriers,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iNumBarriers,
                RenderDriver::Common::CCommandQueue::Type const& queueType
            );

            virtual void platformTransitionOutputAttachments();

            virtual void platformCreateAccelerationStructures(
                std::vector<float4> const& aTrianglePositions,
                std::vector<uint32_t> const& aiTriangleIndices,
                std::vector<std::pair<uint32_t, uint32_t>> const& aMeshRanges,
                uint32_t iNumMeshes
            );

            virtual void platformRayTraceCommand(
                Render::Common::CRenderJob* pRenderJob,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iScreenWidth,
                uint32_t iScreenHeight
            );

            virtual void platformRayTraceShaderSetup(
                Render::Common::CRenderJob* pRenderJob
            );

            void platformCopyTexturePageToAtlas(
                char const* pImageData,
                RenderDriver::Common::CImage* pDestImage,
                uint2 const& pageCoord,
                uint32_t iTexturePageDimension
            );

            virtual void platformCopyTexturePageToAtlas2(
                char const* pImageData,
                RenderDriver::Common::CImage* pDestImage,
                uint2 const& pageCoord,
                uint32_t iTexturePageDimension,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandQueue& commandQueue,
                RenderDriver::Common::CBuffer& uploadBuffer
            );

            virtual void platformCreateCommandBuffer(
                std::unique_ptr<RenderDriver::Common::CCommandAllocator>& threadCommandAllocator,
                std::unique_ptr<RenderDriver::Common::CCommandBuffer>& threadCommandBuffer
            );

            virtual void platformCreateBuffer(
                std::unique_ptr<RenderDriver::Common::CBuffer>& buffer,
                uint32_t iSize
            );

            virtual void platformCreateCommandQueue(
                std::unique_ptr<RenderDriver::Common::CCommandQueue>& commandQueue,
                RenderDriver::Common::CCommandQueue::Type const& type
            );

            virtual void platformTransitionInputImageAttachments(
                Render::Common::CRenderJob* pRenderJob,
                std::vector<char>& acPlatformAttachmentInfo,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                bool bReverse
            );

            virtual void platformTransitionOutputAttachmentsRayTrace(
                Render::Common::CRenderJob* pRenderJob,
                RenderDriver::Common::CCommandBuffer& commandBuffer
            );
            
            virtual void platformBeginComputePass(
                  Render::Common::CRenderJob& renderJob,
                  RenderDriver::Common::CCommandBuffer& commandBuffer);

        protected:
            std::map<uint64_t, std::vector<uint8_t>>        mShaderBinariesDB;
            std::map<uint64_t, std::string>                 mShaderBinaryFilePath;
            std::map<uint64_t, std::vector<uint8_t>>        mShaderDebugInfo;
            std::map<uint64_t, std::vector<uint8_t>>        mShaderDebugSourceInfo;
            std::map<uint64_t, std::vector<uint8_t>>        mShaderBinaryPDB;

        public:
            std::vector<uint8_t> const* getShaderBinary(uint64_t iHash)
            {
                if(mShaderBinariesDB.find(iHash) != mShaderBinariesDB.end())
                {
                    return &mShaderBinariesDB[iHash];
                }

                return nullptr;
            }

            std::vector<uint8_t> const* getShaderDebugSourceBinary(uint64_t iHash)
            {
                if(mShaderDebugSourceInfo.find(iHash) != mShaderDebugSourceInfo.end())
                {
                    return &mShaderDebugSourceInfo[iHash];
                }

                return nullptr;
            }

            std::vector<uint8_t> const* getShaderDebugInfo(uint64_t iHash)
            {
                if(mShaderBinaryPDB.find(iHash) != mShaderBinaryPDB.end())
                {
                    return &mShaderBinaryPDB[iHash];
                }

                return nullptr;
            }

            std::string const* getShaderFilePath(uint64_t iHash)
            {
                if(mShaderBinaryFilePath.find(iHash) != mShaderBinaryFilePath.end())
                {
                    return &mShaderBinaryFilePath[iHash];
                }

                return nullptr;
            }

            void setShaderDebugInfo(uint64_t iHash, std::vector<uint8_t> const& data)
            {
                mShaderDebugInfo[iHash] = data;
            }

            void setShaderDebugSourceInfo(uint64_t iHash, std::vector<uint8_t> const& data)
            {
                mShaderDebugSourceInfo[iHash] = data;
            }
                
            protected:
                std::map<std::string, std::unique_ptr<Render::Metal::CRenderJob>> maRenderJobs;
                std::map<std::string, std::unique_ptr<RenderDriver::Metal::CCommandBuffer>> maRenderJobCommandBuffers;
                std::map<std::string, std::unique_ptr<RenderDriver::Metal::CCommandAllocator>> maRenderJobCommandAllocators;
                std::map<std::string, std::unique_ptr<RenderDriver::Metal::CBuffer>> maVertexBuffers;
                std::map<std::string, std::unique_ptr<RenderDriver::Metal::CBuffer>> maIndexBuffers;
                std::map<std::string, std::unique_ptr<RenderDriver::Metal::CBuffer>> maTrianglePositionBuffers;

                std::unique_ptr<RenderDriver::Metal::CCommandBuffer> mSwapChainCommandBuffer;
                std::unique_ptr<RenderDriver::Metal::CCommandAllocator> mSwapChainCommandAllocator;
        
                std::unique_ptr<RenderDriver::Metal::CBuffer>           mDefaultUniformBuffer;
                //std::map<std::string, std::unique_ptr<RenderDriver::Metal::CAccelerationStructure>> maAccelerationStructures;
        
                id<MTLSamplerState>                           mDefaultNativeLinearSamplerState;
                id<MTLSamplerState>                           mDefaultNativePointSamplerState;
                
                std::unique_ptr<RenderDriver::Metal::CBuffer> mFillerBuffer;
                std::unique_ptr<RenderDriver::Metal::CImage> mFillerTexture;
            };

    }   // Metal

}   // Render
