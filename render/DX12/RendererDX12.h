#pragma once

#include <render/Renderer.h>
#include <render/DX12/RenderJobSerializerDX12.h>

#include <render-driver/DX12/PhysicalDeviceDX12.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/CommandQueueDX12.h>
#include <render-driver/DX12/SwapChainDX12.h>
#include <render-driver/DX12/DescriptorHeapDX12.h>
#include <render-driver/DX12/RenderTargetDX12.h>
#include <render-driver/DX12/CommandBufferDX12.h>

#include <render/RenderJob.h>

#include <renderdoc/renderdoc_app.h>
#include <mutex>

namespace Render
{
    namespace DX12
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
            HWND                mHWND;
        };

        class CRenderer : public Render::Common::CRenderer
        {
        public:
            CRenderer() = default;
            virtual ~CRenderer() = default;

            virtual uint64_t getTotalVertexBufferGPUAddress();
            virtual uint64_t getTotalIndexBufferGPUAddress();
            virtual void platformSetup(Render::Common::RendererDescriptor const& desc);

        protected:
            uint32_t                                      miDescriptorHeapOffset;

            D3D12_VERTEX_BUFFER_VIEW                      mTotalVertexBufferInputView;
            D3D12_INDEX_BUFFER_VIEW                       mTotalIndexBufferView;

        protected:
            RENDERDOC_API_1_5_0*                          mpRenderDocAPI;
            HWND                                          mHWND;

            //GFSDK_Aftermath_ContextHandle                 mAfterMathCommandListContext;
            
        protected:
            std::mutex                                    mUploadDataMutex;

        protected:
            virtual void platformBeginRenderPass(Render::Common::RenderPassDescriptor& renderPassDesc);
            virtual void platformEndRenderPass(Render::Common::RenderPassDescriptor& renderPassDesc);

            virtual void platformCreateVertexIndexBufferViews(uint32_t iVertexBufferSize, uint32_t iIndexBufferSize);
            virtual void platformUploadResourceData(
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset);
            virtual void platformUpdateMeshDataFromGPUBuffer(Render::Common::MeshDataUpdateFromGPUDescriptor const& desc);

            virtual void platformSetPipelineState(
                RenderDriver::Common::CPipelineState& pipelineState,
                RenderDriver::Common::CCommandBuffer& commandBuffer);

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

            virtual void platformSwapChainClear(
                RenderDriver::Common::CSwapChain* pSwapChain,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iTripleBufferIndex,
                float const* afClearColor);

            virtual void platformSetVertexAndIndexBuffers(
                RenderDriver::Common::CCommandBuffer& commandBuffer);

            virtual void platformSetRenderTargetAndClear(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apRenderTargetDescriptorHeaps,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apDepthStencilDescriptorHeaps,
                uint32_t iNumRenderTargetAttachments,
                float const* afClearColor,
                std::vector<bool> const& abClear);

            virtual void platformSetRenderTargetAndClear2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apRenderTargetDescriptorHeaps,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apDepthStencilDescriptorHeaps,
                uint32_t iNumRenderTargetAttachments,
                std::vector<std::vector<float>> const& aafClearColors,
                std::vector<bool> const& abClear);

            virtual void platformExecuteIndirectDrawMeshInstances(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iDrawListAddress,
                uint32_t iDrawCountBufferOffset,
                Render::Common::PassType passType,
                RenderDriver::Common::CBuffer* pIndirectDrawMeshList,
                std::string const& renderJobName);

            virtual void platformExecuteIndirectCompute(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iRenderJobIndirectCommandAddress,
                RenderDriver::Common::CBuffer* pIndirectComputeList,
                std::string const& renderJobName);

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
                RenderDriver::Common::CBuffer* pTempBuffer);

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

            virtual void platformCopyImage(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CImage& srcImage,
                bool bSrcWritable = false);

            virtual void platformCopyImage2(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CImage& srcImage,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                bool bSrcWritable = false);

            virtual void platformCopyImageToBuffer(
                RenderDriver::Common::CBuffer& destBuffer,
                RenderDriver::Common::CImage& srcImage,
                uint32_t iDestOffset);

            virtual void platformCopyImageToBuffer2(
                RenderDriver::Common::CBuffer& destBuffer,
                RenderDriver::Common::CImage& srcImage,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iDestOffset);

            virtual void platformCopyBufferToImage(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CBuffer& srcBuffer,
                uint32_t iSrcOffset);

            virtual void platformCopyBufferToImage2(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CBuffer& srcBuffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iSrcOffset);

            virtual void platformCopyBufferToBuffer(
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pSrcBuffer,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint64_t iDataSize);

            virtual void platformCopyBufferToBuffer3(
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pSrcBuffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandAllocator& commandAllocator,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint64_t iDataSize,
                bool bCloseAndExecute);

            virtual void platformCopyBufferToBufferNoWait(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pSrcBuffer,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint64_t iDataSize);

            virtual void platformCopyBufferToBuffer2(Render::Common::CopyBufferDescriptor const& desc);

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

            virtual void platformResetUploadBuffers(bool bCloseAndResetUploadCommandBuffer);

            virtual void platformBeginRenderDebuggerCapture(std::string const& captureFilePath);
            virtual void platformEndRenderDebuggerCapture();

            virtual void platformBeginRenderDocCapture(std::string const& captureFilePath);
            virtual void platformEndRenderDocCapture();

            virtual void platformBeginRenderJobDebugEventMark(std::string const& renderJobName);
            virtual void platformEndRenderJobDebugEventMark(std::string const& renderJobName);
        
            virtual void platformRenderImgui(uint32_t iFrameIndex);
            virtual void platformRenderImgui2(
                uint32_t iFrameIndex,
                RenderDriver::Common::CCommandBuffer& commandBuffer);

            virtual void platformCreateRenderJobFences();

            virtual void platformPostSetup();

            virtual void platformSwapChainMoveToNextFrame();

            virtual void platformUploadResourceDataImmediate(
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset);

            virtual void platformInitBuffers(
                std::vector<std::shared_ptr<RenderDriver::Common::CBuffer>>& aBuffers,
                uint32_t iNumBuffers);
        
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

            virtual void platformAllocateCopyCommandBuffers(
                std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>& aCopyCommandBuffers,
                std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>& aCopyCommandAllocators,
                uint32_t iNumCopyCommandBuffers);

            virtual void platformUploadResourceDataWithCommandBufferAndUploadBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandAllocator& commandAllocator,
                RenderDriver::Common::CBuffer& uploadBuffer,
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset);
            
            virtual void platformAllocateUploadBuffer(
                std::shared_ptr<RenderDriver::Common::CBuffer>& buffer,
                uint32_t iSize);

            virtual void platformSignalUploadFenceValue(uint64_t iFenceValue);

            virtual void platformEncodeUploadBufferCommand(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandAllocator& commandAllocator,
                RenderDriver::Common::CBuffer& uploadBuffer,
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset);

            virtual void platformExecuteCopyCommandBuffers(
                RenderDriver::Common::CCommandBuffer* const* apCommandBuffers,
                uint32_t iNumCommandBuffers);

            virtual void platformCreateTotalVertexAndIndexBuffer(
                Render::Common::InitializeVertexAndIndexBufferDescriptor const& desc);

            virtual void platformPreFenceSignal(
                RenderDriver::Common::CFence* pFence,
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                uint32_t iQueueType,
                uint64_t iSignalValue);

            virtual void platformPostFenceSignal(
                RenderDriver::Common::CFence* pFence,
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                uint32_t iQueueType,
                uint64_t iSignalValue);

            virtual void platformTransitionImageLayouts(
                Render::Common::RenderJobInfo const& renderJobInfo,
                RenderDriver::Common::CCommandBuffer& commandBuffer);

            virtual void platformInitializeRenderJobs(
                std::vector<std::string> const& aRenderJobNames);

            virtual void platformCreateRenderJobCommandBuffers(
                std::vector<std::string> const& aRenderJobNames
            );

            virtual void platformTransitionBarriers2(
                RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iNumBarrierInfo,
                bool bReverseState,
                RenderDriver::Common::CCommandQueue::Type const& queueType
            );

            virtual void platformBeginRenderPass2(
                Render::Common::RenderPassDescriptor2 const& desc
            );

            virtual void platformEndRenderPass2(
                Render::Common::RenderPassDescriptor2 const& desc
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
                //std::map<std::string, std::unique_ptr<Render::DX12::CRenderJob>> maRenderJobs;
                std::map<std::string, std::unique_ptr<RenderDriver::DX12::CCommandBuffer>> maRenderJobCommandBuffers;
                std::map<std::string, std::unique_ptr<RenderDriver::DX12::CCommandAllocator>> maRenderJobCommandAllocators;
                std::map<std::string, std::unique_ptr<RenderDriver::DX12::CBuffer>> maVertexBuffers;
                std::map<std::string, std::unique_ptr<RenderDriver::DX12::CBuffer>> maIndexBuffers;

                std::unique_ptr<RenderDriver::DX12::CCommandBuffer> mSwapChainCommandBuffer;
                std::unique_ptr<RenderDriver::DX12::CCommandAllocator> mSwapChainCommandAllocator;

                std::unique_ptr<RenderDriver::DX12::CBuffer> mDefaultUniformBuffer;
        };

    }   // DX12

}   // Render