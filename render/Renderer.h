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
#include <render/RenderJobSerializer.h>
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

        struct RenderPassDescriptor
        {
            RenderDriver::Common::CCommandBuffer*       mpCommandBuffer = nullptr;
            RenderDriver::Common::CPipelineState*       mpPipelineState = nullptr;
            uint32_t                                    miOutputWidth = 0;
            uint32_t                                    miOutputHeight = 0;
            float4                                      mClearValue = float4(0.0f, 0.0f, 0.0f, 0.0f);
            uint32_t                                    miOffsetX = 0;
            uint32_t                                    miOffsetY = 0;
            float                                       mfDepthClearValue = 1.0f;
            Render::Common::RenderJobInfo const*        mpRenderJobInfo;
            uint32_t                                    miSwapChainFrameBufferindex = 0;
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

            void copyToCPUBufferFromGPU(
                std::vector<char>& acBuffer,
                std::string const& bufferName);

            virtual inline RenderDriver::Common::CBuffer* getTotalMeshesVertexBuffer()
            {
                return mpTotalMeshesVertexBuffer.get();
            }

            virtual inline RenderDriver::Common::CDevice* getDevice()
            {
                return mpDevice.get();
            }

            virtual inline RenderDriver::Common::CPhysicalDevice* getPhysicalDevice()
            {
                return mpPhysicalDevice.get();
            }

            virtual inline Render::Common::Serializer* getSerializer()
            {
                return mpSerializer.get();
            }

            void setResourceBuffer(
                std::string const& shaderResourceName,
                RenderDriver::Common::CBuffer* pBuffer);

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

            void copyCPUToBufferImmediate(
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset);

            void executeUploadCommandBufferAndWait(bool bWaitCPU = true);
            

            void setRenderJobDispatch(
                std::string const& renderJobName,
                uint32_t iX, 
                uint32_t iY, 
                uint32_t iZ);

            inline uint64_t getTotalRenderJobsTime() { return miTotalExecRenderJobTime; }
            
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

            void allocateCopyCommandBuffers(
                std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>& aCopyCommandBuffers,
                std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>& aCopyCommandAllocators,
                uint32_t iNumCopyCommandBuffers);

            void allocateUploadBuffer(
                std::shared_ptr<RenderDriver::Common::CBuffer>& buffer,
                uint32_t iSize);

            void uploadResourceDataWithCommandBufferAndUploadBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandAllocator& commandAllocator,
                RenderDriver::Common::CBuffer& uploadBuffer,
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset);

            uint64_t getUploadFenceValue()
            {
                return mpUploadFence->getFenceValue();
            }

            void signalUploadFenceValue(uint64_t iFenceValue)
            {
                platformSignalUploadFenceValue(iFenceValue);
            }

            void waitForCopyJobs();

            void encodeCopyCommandBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandAllocator& commandAllocator,
                RenderDriver::Common::CBuffer& uploadBuffer,
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset);

            void executeUploadCommandBuffers(
                RenderDriver::Common::CCommandBuffer* const* apCommandBuffers,
                uint32_t iNumCommandBuffers);

            inline RenderDriver::Common::CCommandBuffer* getUploadCommandBuffer()
            {
                return mpUploadCommandBuffer.get();
            }

            inline RenderDriver::Common::CBuffer* getScratchBuffer()
            {
                return mpScratchBuffer0.get();
            }

            inline RenderDriver::Common::CSwapChain* getSwapChain()
            {
                return mpSwapChain.get();
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

            std::unique_ptr<Render::Common::Serializer>                         mpSerializer;

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
            //std::unique_ptr<RenderDriver::Common::CFence>                       mpCopyTextureFence;

            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpReadBackBuffer;
            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpIntermediateProbeImageBuffer;

            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpTotalMeshesVertexBuffer;
            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpTotalMeshesIndexBuffer;

            std::unique_ptr<RenderDriver::Common::CBuffer>                      mpScratchBuffer0;

            bool                                                                mbCapturingRenderDocDebugFrame = false;
            bool                                                                mbCapturingRenderDebuggerFrame = false;

            RenderDriver::Common::CDescriptorHeap* mpImguiDescriptorHeap;
            //RenderDriver::Common::CCommandAllocator* mpImguiCommandAllocator;
            //RenderDriver::Common::CCommandBuffer* mpImguiCommandBuffer;
            //RenderDriver::Common::CFence* mpImguiFence;

            //std::unique_ptr<RenderDriver::Common::CCommandAllocator> mpImguiCommandAllocator;
            //std::unique_ptr<RenderDriver::Common::CCommandBuffer> mpImguiCommandBuffer;
            //std::unique_ptr<RenderDriver::Common::CFence> mpImguiFence;

            struct WaitParentInfo
            {
                RenderJobInfo const* mpParentRenderJob = nullptr;
                RenderDriver::Common::CCommandQueue::Type               mParentCommandQueueType;
                uint64_t                                                miStartOnFenceValue;
                RenderDriver::Common::CFence* mpParentRenderJobFence = nullptr;
            };

            struct RenderJobFenceInfo
            {
                Render::Common::RenderJobInfo* mpRenderJob = nullptr;
                uint32_t                            miNumParents = 0;
                std::vector<WaitParentInfo>         maOrigWaitParentInfo;
                std::vector<WaitParentInfo>         maWaitParentInfo;
                //std::vector<WaitChildInfo>          maWaitChildrenInfo;
                uint32_t                            miRenderJobIndex;
                
                // for render job duplications, ie breaking up the render jobs to re-use shader resources 
                uint3                               mDispatches = { 0, 0, 0 };  // this is needed due to ability to duplicate render jobs and wanting different number of dispatches
                uint32_t                            miOrigRenderJobIndex = UINT32_MAX;

                //RenderDriver::Common::CFence* mpFence;
            };

            std::vector<std::vector<std::unique_ptr<RenderDriver::Common::CFence>>>   maaRenderJobFences;
            std::vector<std::vector<RenderJobFenceInfo>>                              maRenderJobsByType;
            std::vector<std::unique_ptr<RenderDriver::Common::CFence>>                mapCommandQueueFences;

            struct RenderJobCallbackInfo
            {
                std::function<void(std::string const&, void*)>  mCallbackFunction;
                void* mpCallbackData;
            };

            std::map<std::string, RenderJobCallbackInfo> maBeginRenderJobCallbacks;
            std::map<std::string, RenderJobCallbackInfo> maFinishRenderJobCallbacks;


            uint32_t miFrameIndex = 1;

            //std::unique_ptr<RenderDriver::Common::CFence>                              mpSwapChainFence;
            
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
            void initializeTotalVertexAndIndexBuffer(Render::Common::InitializeVertexAndIndexBufferDescriptor const& desc);

            void clearUploadBuffers();
            void updateRenderJobData(
                Render::Common::RenderJobInfo const& renderJob);
            
            void copyBufferToBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CBuffer& dest,
                RenderDriver::Common::CBuffer& src,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint32_t iDataSize);

            void execRenderJobs();
            void execGraphicsJob(
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex);
            void execComputeJob(
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex);
            void execCopyJob(
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex);

            RenderDriver::Common::CCommandBuffer* filloutGraphicsJobCommandBuffer(
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex);
            RenderDriver::Common::CCommandBuffer* filloutComputeJobCommandBuffer(
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex,
                uint3 const& dispatches);
            void filloutCopyJobCommandBuffer(
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex);


            void filloutGraphicsJobCommandBuffer2(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer,
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex);
            void filloutComputeJobCommandBuffer2(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer,
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex,
                uint3 const& dispatches);
            void filloutCopyJobCommandBuffer2(
                RenderDriver::Common::CCommandBuffer* pCommandBuffer,
                RenderDriver::Common::CCommandAllocator* pCommandAllocator,
                uint32_t iRenderJob,
                uint32_t iTripleBufferIndex);
            
            void createOctahedronTexture(
                std::vector<float>& afData,
                uint32_t iImageSize,
                float fInc,
                bool bConvertRange);

            void loadBlueNoiseImageData();

            void separateOutRenderJobsByType();

            void testRenderGraph(uint32_t iCurrSwapChainBackBufferIndex);

            void execRenderJobs2();
            virtual void buildRenderJobFenceInfo(std::map<std::string, std::vector<WaitParentInfo>>& aaWaitParents);

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
            virtual void platformBeginRenderPass(RenderPassDescriptor& renderPassDesc) = 0;
            virtual void platformEndRenderPass(RenderPassDescriptor& renderPassDesc) = 0;

           

            virtual void platformCreateVertexIndexBufferViews(uint32_t iVertexBufferSize, uint32_t iIndexBufferSize) = 0;
            virtual void platformUploadResourceData(
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset) = 0;
            virtual void platformUpdateMeshDataFromGPUBuffer(Render::Common::MeshDataUpdateFromGPUDescriptor const& desc) = 0;

            virtual void platformSetPipelineState(
                RenderDriver::Common::CPipelineState& pipelineState,
                RenderDriver::Common::CCommandBuffer& commandBuffer) = 0;

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

            virtual void platformSwapChainClear(
                RenderDriver::Common::CSwapChain* pSwapChain,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iTripleBufferIndex,
                float const* afClearColor) = 0;

            virtual void platformSetVertexAndIndexBuffers(
                RenderDriver::Common::CCommandBuffer& commandBuffer) = 0;

            virtual void platformSetRenderTargetAndClear(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apRenderTargetDescriptorHeaps,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apDepthStencilDescriptorHeaps,
                uint32_t iNumRenderTargetAttachments,
                float const* afClearColor,
                std::vector<bool> const& abClear) = 0;

            virtual void platformSetRenderTargetAndClear2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apRenderTargetDescriptorHeaps,
                std::vector<RenderDriver::Common::CDescriptorHeap*>& apDepthStencilDescriptorHeaps,
                uint32_t iNumRenderTargetAttachments,
                std::vector<std::vector<float>> const& aafClearColors,
                std::vector<bool> const& abClear) = 0;

            virtual void platformExecuteIndirectDrawMeshInstances(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iDrawListAddress,
                uint32_t iDrawCountBufferOffset,
                Render::Common::PassType passType,
                RenderDriver::Common::CBuffer* pIndirectDrawMeshList,
                std::string const& renderJobName) = 0;

            virtual void platformExecuteIndirectCompute(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iRenderJobIndirectCommandAddress,
                RenderDriver::Common::CBuffer* pIndirectComputeList,
                std::string const& renderJobName) = 0;

            virtual void platformCopyBufferToCPUMemory(
                RenderDriver::Common::CBuffer* pGPUBuffer,
                void* pCPUBuffer,
                uint64_t iSrcOffset,
                uint64_t iDataSize) = 0;

            virtual void platformCopyBufferToCPUMemory2(
                RenderDriver::Common::CBuffer* pGPUBuffer,
                void* pCPUBuffer,
                uint64_t iSrcOffset,
                uint64_t iDataSize,
                RenderDriver::Common::CBuffer* pTempBuffer) = 0;

            virtual void platformCopyBufferToBuffer(
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pSrcBuffer,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint64_t iDataSize) = 0;

            virtual void platformCopyBufferToBuffer3(
                RenderDriver::Common::CBuffer* pDestBuffer,
                RenderDriver::Common::CBuffer* pSrcBuffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandAllocator& commandAllocator,
                uint32_t iSrcOffset,
                uint32_t iDestOffset,
                uint64_t iDataSize,
                bool bCloseAndExecute) = 0;

            virtual void platformCopyBufferToBufferNoWait(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
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

            virtual void platformCopyImage(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CImage& srcImage,
                bool bSrcWritable = false) = 0;

            virtual void platformCopyImage2(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CImage& srcImage,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                bool bSrcWritable = false) = 0;

            virtual void platformCopyImageToBuffer(
                RenderDriver::Common::CBuffer& destBuffer,
                RenderDriver::Common::CImage& srcImage,
                uint32_t iDestBufferOffset) = 0;

            virtual void platformCopyImageToBuffer2(
                RenderDriver::Common::CBuffer& destBuffer,
                RenderDriver::Common::CImage& srcImage,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iDestBufferOffset) = 0;

            virtual void platformCopyBufferToImage(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CBuffer& srcBuffer,
                uint32_t iSrcOffset) = 0;

            virtual void platformCopyBufferToImage2(
                RenderDriver::Common::CImage& destImage,
                RenderDriver::Common::CBuffer& srcBuffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iSrcOffset) = 0;

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

            virtual void platformResetUploadBuffers(bool) = 0;

            virtual void platformBeginRenderDebuggerCapture(std::string const& captureFilePath) = 0;
            virtual void platformEndRenderDebuggerCapture() = 0;

            virtual void platformBeginRenderDocCapture(std::string const& captureFilePath) = 0;
            virtual void platformEndRenderDocCapture() = 0;

            virtual void platformBeginRenderJobDebugEventMark(std::string const& renderJobName) = 0;
            virtual void platformEndRenderJobDebugEventMark(std::string const& renderJobName) = 0;

            virtual void platformRenderImgui(uint32_t iFrameIndex) = 0;
            virtual void platformRenderImgui2(
                uint32_t iFrameIndex,
                RenderDriver::Common::CCommandBuffer& commandBuffer) = 0;

            virtual void platformCreateRenderJobFences() = 0;

            virtual void platformPostSetup() = 0;

            virtual void platformSwapChainMoveToNextFrame() = 0;

            virtual void platformUploadResourceDataImmediate(
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset) = 0;

            virtual void platformCopyBufferToBuffer2(CopyBufferDescriptor const& desc) = 0;
            virtual void platformInitBuffers(
                std::vector<std::shared_ptr<RenderDriver::Common::CBuffer>>& aBuffers,
                uint32_t iNumBuffers) = 0;

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

            virtual void platformAllocateCopyCommandBuffers(
                std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>& aCopyCommandBuffers,
                std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>& aCopyCommandAllocators,
                uint32_t iNumCopyCommandBuffers) = 0;

            virtual void platformUploadResourceDataWithCommandBufferAndUploadBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandAllocator& commandAllocator,
                RenderDriver::Common::CBuffer& uploadBuffer,
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset) = 0;

            virtual void platformAllocateUploadBuffer(
                std::shared_ptr<RenderDriver::Common::CBuffer>& buffer,
                uint32_t iSize) = 0;

            virtual void platformSignalUploadFenceValue(uint64_t iFenceValue) = 0;

            virtual void platformEncodeUploadBufferCommand(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CCommandAllocator& commandAllocator,
                RenderDriver::Common::CBuffer& uploadBuffer,
                RenderDriver::Common::CBuffer& buffer,
                void* pRawSrcData,
                uint64_t iDataSize,
                uint64_t iDestDataOffset) = 0;

            virtual void platformExecuteCopyCommandBuffers(
                RenderDriver::Common::CCommandBuffer* const* apCommandBuffers,
                uint32_t iNumCommandBuffers) = 0;

            virtual void platformCreateTotalVertexAndIndexBuffer(
                Render::Common::InitializeVertexAndIndexBufferDescriptor const& desc) = 0;

            virtual void platformPreFenceSignal(
                RenderDriver::Common::CFence* pFence,
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                uint32_t iQueueType,
                uint64_t iSignalValue) = 0;

            virtual void platformPostFenceSignal(
                RenderDriver::Common::CFence* pFence,
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                uint32_t iQueueType,
                uint64_t iSignalValue) = 0;

            virtual void platformTransitionImageLayouts(
                Render::Common::RenderJobInfo const& renderJobInfo,
                RenderDriver::Common::CCommandBuffer& commandBuffer) = 0;

            virtual void platformInitializeRenderJobs(
                std::vector<std::string> const& aRenderJobNames) = 0;

            virtual void platformCreateRenderJobCommandBuffers(
                std::vector<std::string> const& aRenderJobNames
            ) = 0;

            virtual void platformTransitionBarriers2(
                RenderDriver::Common::Utils::TransitionBarrierInfo const* aBarrierInfo,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iNumBarrierInfo,
                bool bReverseState,
                RenderDriver::Common::CCommandQueue::Type const& queueType
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