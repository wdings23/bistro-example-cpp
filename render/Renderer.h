#pragma once

#include <stdint.h>

#include <render-driver/Format.h>
#include <serialize_utils.h>
#include <render-driver/Image.h>
#include <render-driver/Fence.h>
#include <render-driver/Utils.h>
#include <render-driver/CommandQueue.h>

#include <render/Camera.h>
#include <render/VertexFormat.h>
#include <render/RenderJob.h>

#include <functional>
#include <atomic>

#define USE_RAY_TRACING 1

namespace Render
{
    namespace Common
    {
        extern std::vector<CCamera>                 gaCameras;

        enum RenderDriverType
        {
            None = 0,
            DX12 = 1,
            Vulkan = 2,

            NumRenderTypes,
        };

        struct RendererDescriptor
        {
            uint32_t                                                miScreenWidth;
            uint32_t                                                miScreenHeight;
            RenderDriver::Common::Format                            mFormat;
            uint8_t                                                 miSamples;
            RenderDriver::Common::SwapChainFlagBits                 miSwapChainFlags;
            uint32_t                                                miNumBuffers;

            uint32_t                                                miViewportWidth;
            uint32_t                                                miViewportHeight;
            float                                                   mfViewportMaxDepth;

            std::function<void()>                                   mInitializeDataFunc;

            std::string                                             mRenderJobsFilePath;
        };

        struct MeshDataUpdateDescriptor
        {
            uint64_t                miDataOffset;
            uint64_t                miDataSize;
            void* mpUpdateData;
        };

        struct MeshDataUpdateFromGPUDescriptor
        {
            uint32_t                                        miSrcDataOffset;
            uint32_t                                        miDestDataOffset;
            uint32_t                                        miDataSize;
            RenderDriver::Common::CBuffer* mpSrcBuffer;
            RenderDriver::Common::CCommandBuffer* mpCommandBuffer;
        };

        struct UploadMeshDescriptor
        {
            VertexFormat const* mpVertexBuffer;
            uint64_t                miVertexDataSize;

            uint32_t const* mpIndexBuffer;
            uint64_t                miIndexDataSize;

            uint32_t                miDestVertexDataOffset;
            uint32_t                miDestIndexDataOffset;
        };

        struct TotalMeshDescriptor
        {
            VertexFormat const* mpVertexBuffer;
            uint64_t                miVertexDataSize;

            uint32_t const* mpIndexBuffer;
            uint64_t                miIndexDataSize;

            uint32_t                miNumMeshes;
            uint32_t const* maiNumVertices;
            uint64_t const* maiMeshVertexOffsets;

            uint32_t const* maiNumIndices;
            uint64_t const* maiMeshIndexOffsets;
        };

        struct UpdateCameraDescriptor
        {
            uint32_t                miCameraIndex;
            vec3                    mLookAt;
            vec3                    mPosition;
            vec3                    mUp;
            float                   mfFov;

            float                   mfNear;
            float                   mfFar;
        };

        struct BufferDataWriteDescriptor
        {
            RenderDriver::Common::CBuffer* mpBuffer;
            void* mpSrcData;
            uint32_t                        miDataSize;
            uint32_t                        miOffset;
            std::function<void(void*)>      mfnDone;
            void* mpCallbackData;
        };

        struct InitializeVertexAndIndexBufferDescriptor
        {
            uint32_t                miVertexDataSize;
            uint32_t                miIndexDataSize;
        };

        enum class CopyBufferFlags
        {
            USE_BARRIERS =              0x1,
            EXECUTE_RIGHT_AWAY =        0x2,
            WAIT_AFTER_EXECUTION =      0x4,

            BEGIN_MARKER =           0x10000,
            END_MARKER =             0x20000
        };

        struct CopyBufferDescriptor
        {
            uint32_t                                        miFlags = 0;

            RenderDriver::Common::CCommandBuffer*           mpCommandBuffer;
            RenderDriver::Common::CCommandAllocator*        mpCommandAllocator;

            RenderDriver::Common::CBuffer*                  mpSrcBuffer;
            RenderDriver::Common::CBuffer*                  mpDestBuffer;
            uint32_t                                        miSrcDataSize;
            uint32_t                                        miDestDataSize;
            uint32_t                                        miSrcDataOffset;
            uint32_t                                        miDestDataOffset;
            void*                                           mpSrcData;
        };

        struct RenderPassDescriptor2
        {
            RenderDriver::Common::CCommandBuffer* mpCommandBuffer = nullptr;
            RenderDriver::Common::CPipelineState* mpPipelineState = nullptr;
            uint32_t                                    miOutputWidth = 0;
            uint32_t                                    miOutputHeight = 0;
            float4                                      mClearValue = float4(0.0f, 0.0f, 0.0f, 0.0f);
            uint32_t                                    miOffsetX = 0;
            uint32_t                                    miOffsetY = 0;
            float                                       mfDepthClearValue = 1.0f;
            Render::Common::CRenderJob const*           mpRenderJob;
            uint32_t                                    miSwapChainFrameBufferindex = 0;
        };

        class CRenderer
        {
        public:
            CRenderer() = default;
            virtual ~CRenderer() = default;

            virtual void setup(RendererDescriptor const& desc);
            virtual void draw();

            void loadRenderJobInfo(std::string const& renderJobFilePath);

            virtual void uploadMeshData(Render::Common::UploadMeshDescriptor const& desc);
            virtual void updateCamera(UpdateCameraDescriptor const& desc);
            virtual void updateShaderResource(
                std::string const& renderJobName,
                std::string const& shaderResourceName,
                void* pData,
                uint32_t iDataSize);
            virtual void updateLightProbeTextureAtlas(uint32_t iCurrProbeIndex);
            virtual float3 getWorldFromScreenPosition(
                uint32_t iX,
                uint32_t iY,
                uint32_t iScreenWidth,
                uint32_t iScreenHeight);

            virtual uint64_t getTotalVertexBufferGPUAddress();
            virtual uint64_t getTotalIndexBufferGPUAddress();

            virtual void updateTextureInArray(
                RenderDriver::Common::CImage& image,
                void const* pRawSrcData,
                uint32_t iImageWidth,
                uint32_t iImageHeight,
                RenderDriver::Common::Format const& format,
                uint32_t iTextureArrayIndex,
                uint32_t iBaseDataSize = 1);

            void copyBufferToCPUMemory(
                RenderDriver::Common::CBuffer* pGPUBuffer,
                void* pCPUBuffer,
                uint64_t iSrcOffset,
                uint64_t iDataSize);

            void copyCPUToBuffer(
                RenderDriver::Common::CBuffer* pGPUBuffer,
                void* pData,
                uint32_t iDestOffset,
                uint32_t iDataSize,
                uint32_t iFlags = 0);

            void copyCPUToBuffer2(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer,
                RenderDriver::Common::CBuffer* pGPUBuffer,
                RenderDriver::Common::CBuffer* pUploadBuffer,
                void* pData,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDataSize,
                uint32_t iFlag);

            void copyCPUToBuffer3(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer,
                RenderDriver::Common::CBuffer* pGPUBuffer,
                RenderDriver::Common::CBuffer* pUploadBuffer,
                void* pData,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDestDataSize,
                uint32_t iTotalDataSize,
                uint32_t iFlag);

            void copyBufferToBuffer(
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pSrcBuffer,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint64_t iDataSize);

            virtual inline RenderDriver::Common::CDevice* getDevice()
            {
                return mpDevice.get();
            }

            virtual inline RenderDriver::Common::CPhysicalDevice* getPhysicalDevice()
            {
                return mpPhysicalDevice.get();
            }

            virtual void takeScreenShot(
                std::vector<float>& aImageData,
                RenderDriver::Common::CImage& image);

            virtual void captureRenderDocFrame(std::string const& filePath);
            virtual void captureRenderDebuggerFrame(std::string const& filePath);

            inline RenderDriver::Common::CDescriptorHeap* getImguiDescriptorHeap()
            {
                return mpImguiDescriptorHeap;
            }

            void registerRenderJobBeginCallback(
                std::string const& renderJobName,
                std::function<void(std::string const&, void*)> const& callbackFunction,
                void* pData);

            void registerRenderJobFinishCallback(
                std::string const& renderJobName,
                std::function<void(std::string const&, void*)> const& callbackFunction,
                void* pData);

            inline uint32_t getFrameIndex()
            {
                return miFrameIndex;
            }

            inline RendererDescriptor getDescriptor()
            {
                return mDesc;
            }

            void executeUploadCommandBufferAndWait(bool bWaitCPU = true);
            
            struct RenderJobExecTime
            {
                uint64_t                miTimeDuration;
                uint64_t                miWaitTime;
            };
            inline std::map<std::string, RenderJobExecTime> const& getRenderJobExecTimes() { return maRenderJobExecTimes; }


            inline void beginDebugMarker(std::string const& name)
            {
                platformBeginDebugMarker(name);
            }

            inline void endDebugMarker()
            {
                platformEndDebugMarker();
            }

           
            void beginDebugMarker2(
                std::string const& labelStr,
                float4 const& color,
                RenderDriver::Common::CCommandBuffer& commandBuffer);

            void endDebugMarker2(
                RenderDriver::Common::CCommandBuffer& commandBuffer);

            void addDuplicateRenderJobs(
                std::string const& renderJobName, 
                uint32_t iNumRenderJobs,
                std::vector<uint3> const& aDispatchesPerJob);

            inline RenderDriver::Common::CCommandQueue* getCommandQueue(RenderDriver::Common::CCommandQueue::Type const& queueType)
            {
                switch(queueType)
                {
                case RenderDriver::Common::CCommandQueue::Type::Graphics:
                    return mpGraphicsCommandQueue.get();
                case RenderDriver::Common::CCommandQueue::Type::Compute:
                    return mpComputeCommandQueue.get();
                case RenderDriver::Common::CCommandQueue::Type::Copy:
                    return mpCopyCommandQueue.get();
                default:
                    return nullptr;
                }

                return nullptr;
            }

            inline Render::Common::RenderDriverType getRenderDriverType()
            {
                return mRenderDriverType;
            }

        public:
            virtual void platformSetup(Render::Common::RendererDescriptor const& desc) = 0;

            uint64_t                                                            miCopyCommandFenceValue = 0;

        protected:

            RenderDriverType                                                    mRenderDriverType;

            RendererDescriptor                                                  mDesc;

            uint32_t                                                            miCurrProbeImageAtlas;
            bool                                                                mbCopyLightProbeImages;

            uint64_t                                                            miFenceValue = 0;
            uint64_t                                                            miGraphicsFenceValue;
            
            bool                                                                mbDataUpdated = false;

            std::unique_ptr<RenderDriver::Common::CDevice>                      mpDevice;
            std::unique_ptr<RenderDriver::Common::CPhysicalDevice>              mpPhysicalDevice;
            std::unique_ptr<RenderDriver::Common::CCommandQueue>                mpComputeCommandQueue;
            std::unique_ptr<RenderDriver::Common::CCommandQueue>                mpGraphicsCommandQueue;
            std::unique_ptr<RenderDriver::Common::CCommandQueue>                mpCopyCommandQueue;
            std::unique_ptr<RenderDriver::Common::CCommandQueue>                mpGPUCopyCommandQueue;
            std::unique_ptr<RenderDriver::Common::CSwapChain>                   mpSwapChain;

            std::unique_ptr<RenderDriver::Common::CCommandAllocator>            mpUploadCommandAllocator;
            std::unique_ptr<RenderDriver::Common::CFence>                       mpUploadFence;
            std::unique_ptr<RenderDriver::Common::CCommandBuffer>               mpUploadCommandBuffer;
            std::vector<std::unique_ptr<RenderDriver::Common::CBuffer>>         mapUploadBuffers;

            std::unique_ptr<RenderDriver::Common::CCommandAllocator>            mpCopyTextureCommandAllocator;
            std::unique_ptr<RenderDriver::Common::CCommandBuffer>               mpCopyTextureCommandBuffer;

            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpReadBackBuffer;
            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpIntermediateProbeImageBuffer;

            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpTotalMeshesVertexBuffer;
            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpTotalMeshesIndexBuffer;

            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpScratchBuffer0;

            bool                                                                mbCapturingRenderDocDebugFrame = false;
            bool                                                                mbCapturingRenderDebuggerFrame = false;

            RenderDriver::Common::CDescriptorHeap* mpImguiDescriptorHeap;

            std::vector<std::vector<std::unique_ptr<RenderDriver::Common::CFence>>>   maaRenderJobFences;
            std::vector<std::unique_ptr<RenderDriver::Common::CFence>>                mapCommandQueueFences;

            struct RenderJobCallbackInfo
            {
                std::function<void(std::string const&, void*)>  mCallbackFunction;
                void* mpCallbackData;
            };

            std::map<std::string, RenderJobCallbackInfo> maBeginRenderJobCallbacks;
            std::map<std::string, RenderJobCallbackInfo> maFinishRenderJobCallbacks;


            uint32_t miFrameIndex = 1;
            
            uint64_t miTotalExecRenderJobTime = 0;

            
            std::map<std::string, RenderJobExecTime>  maRenderJobExecTimes;

            std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>      mapQueueCommandAllocators;
            std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>          mapQueueCommandBuffers;

            std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>       mapQueueGraphicsCommandAllocators;
            std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>          mapQueueGraphicsCommandBuffers;

            std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>       mapQueueComputeCommandAllocators;
            std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>          mapQueueComputeCommandBuffers;

            std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>       mapQueueCopyCommandAllocators;
            std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>          mapQueueCopyCommandBuffers;

            std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>       mapQueueGPUCopyCommandAllocators;
            std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>          mapQueueGPUCopyCommandBuffers;

        protected:
            void copyBufferToBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CBuffer& dest,
                RenderDriver::Common::CBuffer& src,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDataSize);

        public:
            void initData();
            void prepareRenderJobData();
            void updateRenderJobData();

        protected:
            std::map<std::string, Render::Common::CRenderJob*> mapRenderJobs;
            std::map<std::string, RenderDriver::Common::CCommandBuffer*> mapRenderJobCommandBuffers;
            std::map<std::string, RenderDriver::Common::CCommandAllocator*> mapRenderJobCommandAllocators;
            
            std::map<std::string, RenderDriver::Common::CBuffer*> mapVertexBuffers;
            std::map<std::string, RenderDriver::Common::CBuffer*> mapIndexBuffers;
            std::map<std::string, RenderDriver::Common::CBuffer*> mapTrianglePositionBuffers;

            RenderDriver::Common::CCommandBuffer*               mpSwapChainCommandBuffer;
            RenderDriver::Common::CBuffer*                      mpDefaultUniformBuffer;

            float4x4                                            mPrevViewProjectionMatrix;
            std::vector<std::string>                            maRenderJobNames;

            void*                                               maSamplers[2];
            void*                                               mpPlatformInstance;

            std::map<std::string, RenderDriver::Common::CAccelerationStructure*> mapAccelerationStructures;

            std::vector<std::pair<uint32_t, uint32_t>> maMeshRanges;
            std::vector<std::pair<float4, float4>> maMeshExtents;
            uint32_t miNumMeshes;

        protected:
            void execRenderJobs3();

            void filloutComputeJobCommandBuffer3(
                Render::Common::CRenderJob* pRenderJob,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                int3 const& dispatches
            );
            void filloutGraphicsJobCommandBuffer3(
                Render::Common::CRenderJob* pRenderJob,
                RenderDriver::Common::CCommandBuffer& commandBuffer
            );
            void filloutRayTraceJobCommandBuffer(
                Render::Common::CRenderJob* pRenderJob,
                RenderDriver::Common::CCommandBuffer& commandBuffer
            );

            void transitionInputAttachmentsStart(
                Render::Common::CRenderJob* pRenderJob,
                RenderDriver::Common::CCommandBuffer& commandBuffer
            );

            void transitionInputAttachmentsEnd(
                Render::Common::CRenderJob* pRenderJob,
                RenderDriver::Common::CCommandBuffer& commandBuffer
            );
            
        protected:

            virtual void platformUploadResourceData(
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset) = 0;
            
            virtual void platformSetComputeDescriptorSet(
                RenderDriver::Common::CDescriptorSet& descriptorSet,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CPipelineState& pipelineState) = 0;

            virtual void platformSetGraphicsDescriptorSet(
                RenderDriver::Common::CDescriptorSet& descriptorSet,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CPipelineState& pipelineState) = 0;

            virtual void platformSetRayTraceDescriptorSet(
                RenderDriver::Common::CDescriptorSet& descriptorSet,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CPipelineState& pipelineState) = 0;

            virtual void platformSetDescriptorHeap(
                RenderDriver::Common::CDescriptorHeap& descriptorHeap,
                RenderDriver::Common::CCommandBuffer& commandBuffer) = 0;

            virtual void platformSetResourceViews(
                std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CDescriptorHeap& descriptorHeap,
                uint32_t iTripleBufferIndex,
                Render::Common::JobType jobType) = 0;

            virtual void platformSetViewportAndScissorRect(
                uint32_t iWidth,
                uint32_t iHeight,
                float fMaxDepth,
                RenderDriver::Common::CCommandBuffer& commandBuffer) = 0;

            virtual void platformSetGraphicsRoot32BitConstant(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iValue,
                uint32_t iRootParameterIndex,
                uint32_t iOffset) = 0;

            virtual void platformSetGraphicsRoot32BitConstants(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                void const* paValues,
                uint32_t iNumValues,
                uint32_t iRootParameterIndex,
                uint32_t iOffsetIn32Bits) = 0;

            virtual void platformSetComputeRoot32BitConstant(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iValue,
                uint32_t iRootParameterIndex,
                uint32_t iOffset) = 0;

            virtual void platformSetComputeRoot32BitConstants(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CPipelineState& pipelineState,
                void const* paValues,
                uint32_t iNumValues,
                uint32_t iRootParameterIndex,
                uint32_t iOffsetIn32Bits) = 0;

            virtual void platformSetRenderTargetAndClear2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apRenderTargetDescriptorHeaps,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apDepthStencilDescriptorHeaps,
                uint32_t iNumRenderTargetAttachments,
                std::vector<std::vector<float>> const& aafClearColors,
                std::vector<bool> const& abClear) = 0;

            virtual void platformCopyBufferToCPUMemory(
                RenderDriver::Common::CBuffer* pGPUBuffer,
                void* pCPUBuffer,
                uint64_t iSrcOffset,
                uint64_t iDataSize) = 0;

            virtual void platformCopyBufferToBuffer(
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pSrcBuffer,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint64_t iDataSize) = 0;

            virtual void platformCopyImageToCPUMemory(
                RenderDriver::Common::CImage* pGPUMemory,
                std::vector<float>& afImageData) = 0;

            virtual void platformCopyCPUToGPUBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pUploadBuffer,
                void* pCPUData,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDataSize,
                uint32_t iFlag = 0) = 0;

            virtual void platformCopyCPUToGPUBuffer2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pUploadBuffer,
                void* pCPUData,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDestDataSize,
                uint32_t iTotalDataSize,
                uint32_t iFlag = 0) = 0;

            virtual void platformUpdateTextureInArray(
                RenderDriver::Common::CImage& image,
                void const* pRawSrcData,
                uint32_t iImageWidth,
                uint32_t iImageHeight,
                RenderDriver::Common::Format const& format,
                uint32_t iTextureArrayIndex,
                uint32_t iBaseDataSize) = 0;

            virtual void platformCopyImage2(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CImage& srcImage,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                bool bSrcWritable = false) = 0;

            virtual float3 platformGetWorldFromScreenPosition(
                uint32_t iX,
                uint32_t iY,
                uint32_t iScreenWidth,
                uint32_t iScreenHeight) = 0;

            virtual void platformTransitionBarriers(
                RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iNumBarrierInfo,
                bool bReverseState,
                void* szUserData = nullptr) = 0;

            virtual void platformBeginRenderDebuggerCapture(std::string const& captureFilePath) = 0;
            virtual void platformEndRenderDebuggerCapture() = 0;

            virtual void platformBeginRenderDocCapture(std::string const& captureFilePath) = 0;
            virtual void platformEndRenderDocCapture() = 0;

            virtual void platformCreateRenderJobFences() = 0;

            virtual void platformPostSetup() = 0;

            virtual void platformSwapChainMoveToNextFrame() = 0;

            virtual void platformExecuteCopyCommandBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iFlag) = 0;

            virtual void platformBeginDebugMarker(
                std::string const& name,
                RenderDriver::Common::CCommandQueue* pCommandQueue = nullptr) = 0;
            virtual void platformEndDebugMarker(
                RenderDriver::Common::CCommandQueue* pCommandQueue = nullptr) = 0;

            virtual void platformBeginDebugMarker2(
                std::string const& name,
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr) = 0;
            virtual void platformEndDebugMarker2(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr) = 0;

            virtual void platformBeginDebugMarker3(
                std::string const& name,
                float4 const& color,
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr) = 0;
            virtual void platformEndDebugMarker3(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr) = 0;

            virtual void platformInitializeRenderJobs(
                std::vector<std::string> const& aRenderJobNames
            ) = 0;

            virtual void platformCreateRenderJobCommandBuffers(
                std::vector<std::string> const& aRenderJobNames
            ) = 0;

            virtual void platformBeginRenderPass2(
                RenderPassDescriptor2 const& desc
            ) = 0;

            virtual void platformEndRenderPass2(
                Render::Common::RenderPassDescriptor2 const& renderPassDesc
            ) = 0;

            virtual void platformCreateVertexAndIndexBuffer(
                std::string const& name,
                uint32_t iVertexBufferSize,
                uint32_t iIndexBufferSize
            ) = 0;

            virtual void platformSetVertexAndIndexBuffers2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                std::string const& meshName
            ) = 0;

            virtual void platformCreateSwapChainCommandBuffer() = 0;
            virtual void platformTransitionBarrier3(
                RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarriers,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iNumBarriers,
                RenderDriver::Common::CCommandQueue::Type const& queueType
            ) = 0;

            virtual void platformTransitionOutputAttachments() = 0;

            virtual void platformCreateAccelerationStructures(
                std::vector<float4> const& aTrianglePositions,
                std::vector<uint32_t> const& aiTriangleIndices,
                std::vector<std::pair<uint32_t, uint32_t>> const& aMeshRanges,
                uint32_t iNumMeshes
            ) = 0;

            virtual void platformRayTraceCommand(
                Render::Common::CRenderJob* pRenderJob,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iScreenWidth,
                uint32_t iScreenHeight
            ) = 0;

            virtual void platformRayTraceShaderSetup(
                Render::Common::CRenderJob* pRenderJob
            ) = 0;

            protected:
                
                
                struct PreRenderCopyJob
                {
                    std::vector<std::shared_ptr<RenderDriver::Common::CBuffer>>      maUploadBuffers;
                    std::shared_ptr<RenderDriver::Common::CCommandBuffer>            mCommandBuffer;
                    std::shared_ptr<RenderDriver::Common::CCommandAllocator>         mCommandAllocator;
                };

                PreRenderCopyJob mPreRenderCopyJob;

                uint32_t miStartCaptureFrame;

            public:
                uint64_t maiRenderJobFenceValues[RenderDriver::Common::CCommandQueue::Type::NumTypes];
        };

    }   // Common

}   // Render