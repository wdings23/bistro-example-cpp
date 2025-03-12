#include <render/Renderer.h>

#include <render-driver/SwapChain.h>
#include <render-driver/Fence.h>
#include <mesh-clusters/MeshClusterManager.h>

#include <render/render_job_enums.h>
#include <render/RenderJob.h>

#include <utils/JobManager.h>
#include <render/Camera.h>
#include <utils/LogPrint.h>
#include <utils/wtfassert.h>

#include <sstream>
#include <thread>
#include <mutex>

#include <stb_image.h>
#include <stb_image_write.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr/tinyexr.h>
#include <tinyexr/miniz.c>

#include <chrono>

#if defined(RUN_TEST_LOCALLY)
std::vector<char> gaReadOnlyBufferCopy(1 << 27);
std::vector<char> gaReadWriteBufferCopy(1 << 27);
std::unique_ptr<RenderDriver::Common::CBuffer> gpCopyBuffer;
#endif // TEST_HLSL_LOCALLY

#if !defined(_MSC_VER)
    extern void getAssetsDir(std::string& fullPath, std::string const& fileName);
#endif // _MSC_VER

std::mutex sMutex;
std::mutex sPresentMutex;

namespace Render
{
    namespace Common
    {
        std::vector<CCamera>                 gaCameras;
        float3                               gLightDirection;
        float3                               gPrevLightDirection;
        float                                gfEmissiveRadiance;
        float                                gfClearReservoir;

        /*
        **
        */
        void CRenderer::setup(RendererDescriptor const& desc)
        {
            mDesc = desc;
            JobManager::instance();

            miFenceValue = 1;

            uint32_t const iNumCamera = 3;
            gaCameras.resize(iNumCamera);
            for(uint32_t iCamera = 0; iCamera < iNumCamera; iCamera++)
            {
                gaCameras[iCamera].setFar(100.0f);
                gaCameras[iCamera].setNear(0.4f);
                //gaCameras[iCamera].setLookAt(vec3(-1.5588f, -0.572f, -0.156f));
                //gaCameras[iCamera].setPosition(vec3(6.395f, 2.70f, 0.59f));

                gaCameras[iCamera].setLookAt(vec3(-3.44f, 1.53f, 0.625f));
                gaCameras[iCamera].setPosition(vec3(-7.08f, 1.62f, 0.675f));
                gaCameras[iCamera].setProjectionType(ProjectionType::PROJECTION_PERSPECTIVE);

                CameraUpdateInfo cameraInfo;
                cameraInfo.mfFieldOfView = 3.14159f * 0.5f;
                cameraInfo.mfViewHeight = 200.0f;
                cameraInfo.mfViewWidth = 200.0f;
                cameraInfo.mUp = vec3(0.0f, 1.0f, 0.0f);
                cameraInfo.mfNear = 0.4f;
                cameraInfo.mfFar = 100.0f;
                gaCameras[iCamera].update(cameraInfo);
            }

            platformSetup(desc);

            mDesc = desc;
        }

        /*
        **
        */
        void CRenderer::loadRenderJobInfo(
            std::string const& renderJobFilePath,
            std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>>& aExternalBufferMap)
        {
            auto directoryEnd = renderJobFilePath.find_last_of("\\") - 1;
            std::string renderJobDirectory = renderJobFilePath.substr(0, directoryEnd);
            auto topDirectoryEnd = renderJobDirectory.find_last_of("\\");
            std::string topDirectory = renderJobDirectory.substr(0, topDirectoryEnd);

            FILE* fp = fopen(renderJobFilePath.c_str(), "rb");
            fseek(fp, 0, SEEK_END);
            size_t iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<char> acBuffer(iFileSize + 1);
            acBuffer[iFileSize] = 0;
            fread(acBuffer.data(), sizeof(char), iFileSize, fp);
            fclose(fp);

            rapidjson::Document doc;
            doc.Parse(acBuffer.data());
            auto const& aJobs = doc["Jobs"].GetArray();
            uint32_t iNumJobs = aJobs.Size();
                
            std::vector<std::string> aRenderJobNames;
            for(auto const& job : aJobs)
            {
                aRenderJobNames.push_back(job["Name"].GetString());
            }
                
            platformInitializeRenderJobs(
                aRenderJobNames
            );

            std::vector<Render::Common::CRenderJob*> apRenderJobs;
            for(auto const& keyValue : mapRenderJobs)
            {
                apRenderJobs.push_back(keyValue.second);
            }

            std::vector<std::map<std::string, std::string>> aAttachmentJSONInfo(iNumJobs);

            uint32_t iJob = 0;
            for(auto const& job: aJobs)
            {
                uint32_t aiDispatch[3] = {1, 1, 1};

                std::string name = job["Name"].GetString();
                std::string pipeline = job["Pipeline"].GetString();
                std::string type = job["Type"].GetString();
                std::string passType = job["PassType"].GetString();

#if !defined(USE_RAY_TRACING)
                if(type == "Ray Trace")
                {
                    continue;
                }
#endif // !USE_RAY_TRACING


                if(job.HasMember("Dispatch"))
                {
                    auto const& dispatchSizeArray = job["Dispatch"].GetArray();
                    aiDispatch[0] = dispatchSizeArray[0].GetInt();
                    aiDispatch[1] = dispatchSizeArray[1].GetInt();
                    aiDispatch[2] = dispatchSizeArray[2].GetInt();
                }

                aAttachmentJSONInfo[iJob]["Name"] = name;
                aAttachmentJSONInfo[iJob]["Type"] = type;
                aAttachmentJSONInfo[iJob]["PassType"] = passType;

                maRenderJobNames.push_back(name);

                ++iJob;
            }

            std::vector<std::map<std::string, std::string>> aExtraAttachmentJSONInfo;

            // output attachments first for attachment linkages
            iJob = 0;
            for(auto const& job : aJobs)
            {
                std::string name = job["Name"].GetString();
                std::string pipeline = job["Pipeline"].GetString();
                std::string type = job["Type"].GetString();
                std::string passType = job["PassType"].GetString();

#if !defined(USE_RAY_TRACING)
                if(type == "Ray Trace")
                {
                    continue;
                }
#endif // !USE_RAY_TRACING

                std::string fullPath = topDirectory + "\\render-jobs\\" + pipeline;

#if !defined(_MSC_VER)
                getAssetsDir(fullPath, std::string("render-jobs/") + pipeline);
#endif // _MSC_VER
                
                uint3 dispatchSize(1, 1, 1);
                Render::Common::CRenderJob::CreateInfo createInfo
                {
                    name,
                    mpDevice.get(),
                    mpCopyCommandQueue.get(),
                    fullPath,
                    topDirectory,
                    mDesc.miScreenWidth,
                    mDesc.miScreenHeight,
                    &apRenderJobs,
                    &aAttachmentJSONInfo,
                    &aExtraAttachmentJSONInfo,
                    mpDefaultUniformBuffer,
                    dispatchSize,
                    nullptr,
                    mpSwapChain.get(),
                    maSamplers,
                    mpPlatformInstance, 
                    &mapAccelerationStructures,
                    
                };
                mapRenderJobs[name]->mName = name;
                mapRenderJobs[name]->createOutputAttachments(createInfo);
                ++iJob;
            }

            //platformTransitionOutputAttachments();

            // create the rest
            iJob = 0;
            uint64_t iSemaphoreValue = 0;
            for(auto const& job : aJobs)
            {
                std::string name = job["Name"].GetString();
                std::string pipeline = job["Pipeline"].GetString();
                std::string type = job["Type"].GetString();
                std::string passType = job["PassType"].GetString();

#if !defined(USE_RAY_TRACING)
                if(type == "Ray Trace")
                {
                    mapRenderJobs[name]->mType = Render::Common::JobType::RayTrace;
                    continue;
                }

                if(name.find("Light Composite") != std::string::npos ||
                    name.find("Temporal Accumulation Graphics") != std::string::npos ||
                    name.find("Diffuse Filter Graphics") != std::string::npos)
                {
                    continue;
                }
#endif // !USE_RAY_TRACING

                std::string fullPath = topDirectory + "\\render-jobs\\" + pipeline;

                uint3 dispatchSize(1, 1, 1);
                if(type == "Compute" && job.HasMember("Dispatch"))
                {
                    auto const& dispatchArray = job["Dispatch"].GetArray();
                    dispatchSize = uint3(
                        dispatchArray[0].GetInt(),
                        dispatchArray[1].GetInt(),
                        dispatchArray[2].GetInt()
                    );
                }

                std::vector<char> acConstantBufferData;
                if(job.HasMember("ConstantBuffer"))
                {
                    auto const& array = job["ConstantBuffer"].GetArray();

                    uint32_t iOffset = 0;
                    for(uint32_t i = 0; i < array.Size(); i++)
                    {
                        auto const& constantBufferInfo = array[i];
                        char const* szDataType = constantBufferInfo["data type"].GetString();
                        if(std::string(szDataType) == "int")
                        {
                            int32_t iData = constantBufferInfo["data"].GetInt();
                            acConstantBufferData.resize(acConstantBufferData.size() + 4);
                            int* piData = ((int*)acConstantBufferData.data()) + iOffset;
                            *piData = iData;
                            iOffset += 4;
                        }
                        else if(std::string(szDataType) == "float")
                        {
                            float fData = constantBufferInfo["data"].GetFloat();
                            acConstantBufferData.resize(acConstantBufferData.size() + 4);
                            float* pfData = ((float*)acConstantBufferData.data()) + iOffset;
                            *pfData = fData;
                            iOffset += 4;
                        }
                    }
                }

                // initialize external data
                std::function<void(Render::Common::CRenderJob*)>* pfnInitDataFunc = nullptr;
                if(mapfnRenderJobData.find(name) != mapfnRenderJobData.end())
                {
                    pfnInitDataFunc = mapfnRenderJobData[name];
                }

#if !defined(_MSC_VER)
                getAssetsDir(fullPath, std::string("render-jobs/") + pipeline);
#endif // _MSC_VER
                
                Render::Common::CRenderJob::CreateInfo createInfo
                {
                    name,
                    mpDevice.get(),
                    mpCopyCommandQueue.get(),
                    fullPath,
                    topDirectory,
                    mDesc.miScreenWidth,
                    mDesc.miScreenHeight,
                    &apRenderJobs,
                    &aAttachmentJSONInfo,
                    &aExtraAttachmentJSONInfo,
                    mpDefaultUniformBuffer,
                    dispatchSize,
                    &iSemaphoreValue,
                    mpSwapChain.get(),
                    maSamplers,
                    mpPlatformInstance,
                    &mapAccelerationStructures,
                    mpGraphicsCommandQueue.get(),
                    mpComputeCommandQueue.get(),
                    pfnInitDataFunc,
                    (acConstantBufferData.size() > 0) ? &acConstantBufferData : nullptr
                };
                mapRenderJobs[name]->create(createInfo);

                if(mapRenderJobs[name]->mType == JobType::RayTrace)
                {
                    platformRayTraceShaderSetup(
                        mapRenderJobs[name]
                    );
                }

                ++iJob;
            }

            // create command buffers for render jobs
            platformCreateRenderJobCommandBuffers(
                aRenderJobNames
            );

            //separateOutRenderJobsByType();
            
            // create fences for each render jobs
            platformCreateRenderJobFences();

            platformPostSetup(aExternalBufferMap);
        }

        /*
        **
        */
        void CRenderer::draw()
        {
            platformBeginDebugMarker("Draw");
            auto start = std::chrono::high_resolution_clock::now();
            //execRenderJobs2();
            execRenderJobs3();
            miTotalExecRenderJobTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

            DEBUG_PRINTF("frame %d time elapsed: %d us\n", miFrameIndex, miTotalExecRenderJobTime);
            
#if 0
            RenderDriver::Common::SwapChainPresentDescriptor desc;
            desc.miSyncInterval = 0;
            desc.miFlags = 0x200;
            desc.mpDevice = mpDevice.get();
            desc.mpPresentQueue = mpGraphicsCommandQueue.get();

            //desc.miSyncInterval = 1;
            //desc.miFlags = 0;

            {
                std::lock_guard lock(sPresentMutex);
                beginDebugMarker("Present");
                mpSwapChain->present(desc);
                endDebugMarker();
            }

            if(mbCapturingRenderDebuggerFrame)
            {
                platformEndRenderDebuggerCapture();
                mbCapturingRenderDebuggerFrame = false;
            }

            // move to next frame
            {
                platformSwapChainMoveToNextFrame();
            }

            if(mbCapturingRenderDocDebugFrame && miFrameIndex - miStartCaptureFrame >= 5)
            {
                platformEndRenderDocCapture();
                mbCapturingRenderDocDebugFrame = false;
            }

            platformEndDebugMarker();

#if defined(_DEBUG)
            WTFASSERT(mpUploadCommandBuffer->getNumCommands() <= 0, "There are still un-executed copy commands left in the upload command buffer (%d)\n",
                mpUploadCommandBuffer->getNumCommands());
#endif // _DEBUG 
#endif // #if 0
        }

        /*
        **
        */
        void CRenderer::presentSwapChain()
        {
            RenderDriver::Common::SwapChainPresentDescriptor desc;
            desc.miSyncInterval = 0;
            desc.miFlags = 0x200;
            desc.mpDevice = mpDevice.get();
            desc.mpPresentQueue = mpGraphicsCommandQueue.get();

            {
                std::lock_guard lock(sPresentMutex);
                beginDebugMarker("Present");
                
                if(mRenderDriverType == RenderDriverType::Metal)
                {
                    mapRenderJobCommandBuffers["Swap Chain Graphics"]->reset();
                }
                else
                {
                    mpSwapChain->present(desc);
                }
                
                endDebugMarker();
            }

            if(mbCapturingRenderDebuggerFrame)
            {
                platformEndRenderDebuggerCapture();
                mbCapturingRenderDebuggerFrame = false;
            }

            // move to next frame
            {
                platformSwapChainMoveToNextFrame();
            }

            if(mbCapturingRenderDocDebugFrame && miFrameIndex - miStartCaptureFrame >= 5)
            {
                platformEndRenderDocCapture();
                mbCapturingRenderDocDebugFrame = false;
            }

            platformEndDebugMarker();

#if defined(_DEBUG)
            WTFASSERT(mpUploadCommandBuffer->getNumCommands() <= 0, "There are still un-executed copy commands left in the upload command buffer (%d)\n",
                mpUploadCommandBuffer->getNumCommands());
#endif // _DEBUG 
        }

        /*
        **
        */
        void CRenderer::postDraw()
        {
            if(mpfnPostDraw != nullptr)
            {
                (*mpfnPostDraw)(this);
            }

            for(auto& uploadBuffer : mapUploadBuffers)
            {
                uploadBuffer->releaseNativeBuffer();
            }

            mapUploadBuffers.clear();

#if 0
            // get the texture pages needed
            auto& pTexturePageQueueBuffer = mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Texture Page Queue MIP"];
            uint32_t iBufferSize = (uint32_t)pTexturePageQueueBuffer->getDescriptor().miSize;
            std::vector<char> acTexturePageQueueData(iBufferSize);
            platformCopyBufferToCPUMemory(
                pTexturePageQueueBuffer,
                acTexturePageQueueData.data(),
                0,
                iBufferSize
            );

            // get the number of pages
            auto& pTexturePageCountBuffer = mapRenderJobs["Texture Page Queue Compute"]->mapOutputBufferAttachments["Counters"];
            std::vector<char> acCounter(256);
            platformCopyBufferToCPUMemory(
                pTexturePageCountBuffer,
                acCounter.data(),
                0,
                256
            );

            uint32_t const iTexturePageSize = 64;

            // texture page info
            struct TexturePage
            {
                int32_t miPageUV;
                int32_t miTextureID;
                int32_t miHashIndex;

                int32_t miMIP;
            };
            uint32_t iNumRequests = *((uint32_t*)acCounter.data());
            for(uint32_t i = 0; i < iNumRequests; i++)
            {
                char* acData = acTexturePageQueueData.data() + i * sizeof(TexturePage);
                TexturePage& texturePage = *((TexturePage*)acData);
                if(texturePage.miTextureID >= 65536)
                {
                    continue;
                }

                uint32_t iY = (texturePage.miPageUV >> 16);
                uint32_t iX = (texturePage.miPageUV & 0x0000ffff);
                
                uint32_t iMipDenom = (uint32_t)pow(2, texturePage.miMIP);
                uint2 const& textureDimension = maAlbedoTextureDimensions[texturePage.miTextureID];
                uint2 mipTextureDimension(
                    textureDimension.x / iMipDenom,
                    textureDimension.y / iMipDenom
                );

                uint2 numDivs(
                    max(1, mipTextureDimension.x / iTexturePageSize),
                    max(1, mipTextureDimension.y / iTexturePageSize)
                );

                // texture page coordinate
                uint2 pageCoord(
                    (uint32_t)abs((int32_t)iX) % numDivs.x,
                    (uint32_t)abs((int32_t)iY) % numDivs.y
                );
            }
#endif // #if 0

            
        }

        /*
        **
        */
        void CRenderer::addDuplicateRenderJobs(
            std::string const& renderJobName,
            uint32_t iNumRenderJobs,
            std::vector<uint3> const& aDispatchesPerJob)
        {
        }

        /*
        **
        */
        void CRenderer::executeUploadCommandBufferAndWait(bool bWaitCPU)
        {
            // exec command buffer
            mpUploadCommandBuffer->close();
            mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);

            // wait until done
            RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
            placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
            placeFenceDesc.miFenceValue = ++miCopyCommandFenceValue;
            placeFenceDesc.mType = RenderDriver::Common::FenceType::CPU;
            mpUploadFence->place(placeFenceDesc);
            //mpUploadFence->waitGPU(mpCopyCommandQueue.get());

            if(bWaitCPU)
            {
                mpUploadFence->waitCPU(UINT64_MAX);
            }

            mpUploadCommandAllocator->reset();
            mpUploadCommandBuffer->reset();
        }

        /*
        **
        */
        void CRenderer::copyBufferToBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CBuffer& dest,
            RenderDriver::Common::CBuffer& src,
            uint32_t iDestOffset,
            uint32_t iSrcOffset,
            uint32_t iDataSize)
        {
            dest.copy(src, commandBuffer, iDestOffset, iSrcOffset, iDataSize);
        }

        /*
        **
        */
        void CRenderer::copyTexturePageToAtlas(
            const char* acTexturePageImageData,
            RenderDriver::Common::CImage* pDestImage,
            uint2 const& pageCoord,
            uint32_t iTexturePageDimension)
        {
            std::lock_guard lock(sPresentMutex);

            platformCopyTexturePageToAtlas(
                acTexturePageImageData,
                pDestImage,
                pageCoord,
                iTexturePageDimension
            );
        }

        /*
        **
        */
        void CRenderer::copyTexturePageToAtlas2(
            const char* acTexturePageImageData,
            RenderDriver::Common::CImage* pDestImage,
            uint2 const& pageCoord,
            uint32_t iTexturePageDimension,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue,
            RenderDriver::Common::CBuffer& uploadBuffer)
        {
            std::lock_guard lock(sPresentMutex);

            platformCopyTexturePageToAtlas2(
                acTexturePageImageData,
                pDestImage,
                pageCoord,
                iTexturePageDimension,
                commandBuffer,
                commandQueue,
                uploadBuffer
            );
        }

        /*
        **
        */
        void CRenderer::copyCPUToBuffer2(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer,
            RenderDriver::Common::CBuffer* pGPUBuffer,
            RenderDriver::Common::CBuffer* pUploadBuffer,
            void* pData,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint32_t iDataSize,
            uint32_t iFlag)
        {
            std::lock_guard lock(sPresentMutex);

            platformCopyCPUToGPUBuffer(
                *pCommandBuffer,
                pGPUBuffer,
                pUploadBuffer,
                pData,
                iSrcOffset,
                iDestOffset,
                iDataSize,
                iFlag);

            if((iFlag & static_cast<uint32_t>(CopyBufferFlags::EXECUTE_RIGHT_AWAY)) > 0)
            {
                platformExecuteCopyCommandBuffer(*pCommandBuffer, iFlag);
                
            }
        }

        /*
        **
        */
        void CRenderer::copyCPUToBuffer3(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer,
            RenderDriver::Common::CBuffer* pGPUBuffer,
            RenderDriver::Common::CBuffer* pUploadBuffer,
            void* pData,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint32_t iDestDataSize,
            uint32_t iTotalDataSize,
            uint32_t iFlag)
        {
            platformCopyCPUToGPUBuffer2(
                *pCommandBuffer,
                pGPUBuffer,
                pUploadBuffer,
                pData,
                iSrcOffset,
                iDestOffset,
                iDestDataSize,
                iTotalDataSize,
                iFlag);

            if((iFlag & static_cast<uint32_t>(CopyBufferFlags::EXECUTE_RIGHT_AWAY)) > 0)
            {
                platformExecuteCopyCommandBuffer(*pCommandBuffer, iFlag);
            }
        }

        /*
        **
        */
        void CRenderer::copyCPUToBuffer4(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pData,
            uint32_t iDestOffset,
            uint32_t iDataSize,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue,
            RenderDriver::Common::CBuffer& uploadBuffer)
        {
            platformCopyCPUToGPUBuffer3(
                commandBuffer,
                commandQueue,
                pGPUBuffer,
                pData,
                0,
                iDestOffset,
                iDataSize,
                uploadBuffer
            );
        }

        /*
        **
        */
        void CRenderer::uploadMeshData(Render::Common::UploadMeshDescriptor const& desc)
        {
            platformUploadResourceData(
                *mpTotalMeshesVertexBuffer,
                (void*)desc.mpVertexBuffer,
                static_cast<uint32_t>(desc.miVertexDataSize),
                desc.miDestVertexDataOffset);

            platformUploadResourceData(
                *mpTotalMeshesIndexBuffer,
                (void*)desc.mpIndexBuffer,
                static_cast<uint32_t>(desc.miIndexDataSize),
                desc.miDestIndexDataOffset);

            platformExecuteCopyCommandBuffer(
                *mpUploadCommandBuffer,
                static_cast<uint32_t>(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION));

            mpUploadCommandAllocator->reset();
            mpUploadCommandBuffer->reset();
        }

        /*
        **
        */
        void CRenderer::updateCamera(Render::Common::UpdateCameraDescriptor const& desc)
        {
            Render::Common::gaCameras[0].setNear(desc.mfNear);
            Render::Common::gaCameras[0].setFar(desc.mfFar);
            Render::Common::gaCameras[0].setLookAt(desc.mLookAt);
            Render::Common::gaCameras[0].setPosition(desc.mPosition);

            CameraUpdateInfo cameraInfo;
            cameraInfo.mfFieldOfView = desc.mfFov;
            cameraInfo.mfViewHeight = 200.0f;
            cameraInfo.mfViewWidth = 200.0f;
            cameraInfo.mUp = desc.mUp;
            cameraInfo.mfFar = desc.mfFar;
            cameraInfo.mfNear = desc.mfNear;
            cameraInfo.mProjectionJitter = desc.mJitter;

            Render::Common::gaCameras[2] = Render::Common::gaCameras[0];
            Render::Common::gaCameras[0].update(cameraInfo);
        }

        /*
        **
        */
        void CRenderer::updateShaderResource(
            std::string const& renderJobName,
            std::string const& shaderResourceName,
            void* pData,
            uint32_t iDataSize)
        {
        }

        /*
        **
        */
        void CRenderer::updateLightProbeTextureAtlas(uint32_t iCurrProbeIndex)
        {
            mbCopyLightProbeImages = true;
            miCurrProbeImageAtlas = iCurrProbeIndex;
        }

        /*
        **
        */
        uint64_t CRenderer::getTotalVertexBufferGPUAddress()
        {
            return 0;
        }

        /*
        **
        */
        uint64_t CRenderer::getTotalIndexBufferGPUAddress()
        {
            return 0;
        }

        /*
        **
        */
        void CRenderer::updateTextureInArray(
            RenderDriver::Common::CImage& image,
            void const* pRawSrcData,
            uint32_t iImageWidth,
            uint32_t iImageHeight,
            RenderDriver::Common::Format const& format,
            uint32_t iTextureArrayIndex,
            uint32_t iBaseDataSize)
        {
            platformUpdateTextureInArray(
                image,
                pRawSrcData,
                iImageWidth,
                iImageHeight,
                format,
                iTextureArrayIndex,
                iBaseDataSize);
        }

        /*
        **
        */
        void CRenderer::copyBufferToCPUMemory(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pCPUBuffer,
            uint64_t iSrcOffset,
            uint64_t iDataSize)
        {
            platformCopyBufferToCPUMemory(
                pGPUBuffer,
                pCPUBuffer,
                iSrcOffset,
                iDataSize);
        }

        /*
        **
        */
        void CRenderer::copyBufferToCPUMemory2(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pCPUBuffer,
            uint64_t iSrcOffset,
            uint64_t iDataSize,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue)
        {
            platformCopyBufferToCPUMemory2(
                pGPUBuffer,
                pCPUBuffer,
                iSrcOffset,
                iDataSize,
                commandBuffer,
                commandQueue);
        }
    
        /*
        **
        */
        void CRenderer::copyBufferToCPUMemory3(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pCPUBuffer,
            uint64_t iSrcOffset,
            uint64_t iDataSize,
            RenderDriver::Common::CBuffer& readBackBuffer,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandQueue& commandQueue)
        {
            platformCopyBufferToCPUMemory3(
                pGPUBuffer,
                pCPUBuffer,
                iSrcOffset,
                iDataSize,
                readBackBuffer,
                commandBuffer,
                commandQueue);
        }

        /*
        **
        */
        void CRenderer::copyCPUToBuffer(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pData,
            uint32_t iDestOffset,
            uint32_t iDataSize,
            uint32_t iFlags)
        {
            platformUploadResourceData(
                *pGPUBuffer,
                pData,
                iDataSize,
                iDestOffset);
        }

        /*
        **
        */
        void CRenderer::copyBufferToBuffer(
            RenderDriver::Common::CBuffer* pDestBuffer,
            RenderDriver::Common::CBuffer* pSrcBuffer,
            uint32_t iSrcOffset,
            uint32_t iDestOffset,
            uint64_t iDataSize)
        {
            platformCopyBufferToBuffer(
                pDestBuffer,
                pSrcBuffer,
                iSrcOffset,
                iDestOffset,
                iDataSize);
        }

        /*
        **
        */
        float3 CRenderer::getWorldFromScreenPosition(
            uint32_t iX,
            uint32_t iY,
            uint32_t iScreenWidth,
            uint32_t iScreenHeight)
        {
            return platformGetWorldFromScreenPosition(iX, iY, iScreenWidth, iScreenHeight);
        }

        /*
        **
        */
        void CRenderer::takeScreenShot(
            std::vector<float>& aImageData,
            RenderDriver::Common::CImage& image)
        {
            platformCopyImageToCPUMemory(&image, aImageData);
        }

        /*
        **
        */
        void CRenderer::captureRenderDocFrame(std::string const& filePath)
        {
            platformBeginRenderDocCapture(filePath);
            mbCapturingRenderDocDebugFrame = true;
        }

        /*
        **
        */
        void CRenderer::captureRenderDebuggerFrame(std::string const& filePath)
        {
            platformBeginRenderDebuggerCapture(filePath);
            mbCapturingRenderDebuggerFrame = true;
        }

        /*
        **
        */
        void CRenderer::registerRenderJobFinishCallback(
            std::string const& renderJobName,
            std::function<void(std::string const&, void*)> const& callbackFunction,
            void* pData)
        {
            RenderJobCallbackInfo callbackInfo;
            callbackInfo.mCallbackFunction = callbackFunction;
            callbackInfo.mpCallbackData = pData;
            maFinishRenderJobCallbacks[renderJobName] = callbackInfo;
        }

        /*
        **
        */
        void CRenderer::registerRenderJobBeginCallback(
            std::string const& renderJobName,
            std::function<void(std::string const&, void*)> const& callbackFunction,
            void* pData)
        {
            RenderJobCallbackInfo callbackInfo;
            callbackInfo.mCallbackFunction = callbackFunction;
            callbackInfo.mpCallbackData = pData;
            maBeginRenderJobCallbacks[renderJobName] = callbackInfo;
        }

        /*
        **
        */
        void CRenderer::beginDebugMarker2(
            std::string const& labelStr,
            float4 const& color,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            platformBeginDebugMarker3(
                labelStr,
                color,
                &commandBuffer);
        }

        /*
        **
        */
        void CRenderer::endDebugMarker2(RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            platformEndDebugMarker3(&commandBuffer);
        }

        /*
        **
        */
        void CRenderer::transitionInputAttachmentsStart(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer
        )
        {
            RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[32];
            uint32_t iNumBarriers = 0;
            for(auto const& inputAttachmentKeyValue : pRenderJob->mapInputImageAttachments)
            {
                //DEBUG_PRINTF("*** INPUT ATTACHMENT ***\n");
                bool bDepthStencil = false;
                std::string const& attachmentName = inputAttachmentKeyValue.first;
                if(attachmentName.find("Depth Output") != std::string::npos)
                {
                    bDepthStencil = true;
                }

                if(bDepthStencil)
                {
                    RenderDriver::Common::Utils::TransitionBarrierInfo barrier;
                    aBarriers[iNumBarriers].mpImage = pRenderJob->mapInputImageAttachments[attachmentName];
                    aBarriers[iNumBarriers].mBefore = RenderDriver::Common::ResourceStateFlagBits::DepthStencilAttachment;
                    aBarriers[iNumBarriers].mAfter = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumBarriers].mbWriteableBefore = true;
                    aBarriers[iNumBarriers].mbWriteableAfter = false;
                    aBarriers[iNumBarriers].mCommandBufferType = RenderDriver::Common::CommandBufferType::Graphics;
                    ++iNumBarriers;
                }
                else
                {
                    RenderDriver::Common::Utils::TransitionBarrierInfo barrier;
                    aBarriers[iNumBarriers].mpImage = pRenderJob->mapInputImageAttachments[attachmentName];
                    aBarriers[iNumBarriers].mBefore = RenderDriver::Common::ResourceStateFlagBits::ColorAttachment;
                    aBarriers[iNumBarriers].mAfter = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumBarriers].mbWriteableBefore = true;
                    aBarriers[iNumBarriers].mbWriteableAfter = false;
                    aBarriers[iNumBarriers].mCommandBufferType = RenderDriver::Common::CommandBufferType::Graphics;
                    ++iNumBarriers;
                }
            }

            platformTransitionBarrier3(
                aBarriers,
                commandBuffer,
                iNumBarriers,
                RenderDriver::Common::CCommandQueue::Type::Graphics
            );
        }

        /*
        **
        */
        void CRenderer::transitionInputAttachmentsEnd(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer
        )
        {
            for(auto const& inputAttachmentKeyValue : pRenderJob->mapInputImageAttachments)
            {
                DEBUG_PRINTF("*** INPUT ATTACHMENT ***\n");
                std::string const& attachmentName = inputAttachmentKeyValue.first;
                if(attachmentName == "Depth Output")
                {
                    RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                    aBarriers[0].mpImage = pRenderJob->mapInputImageAttachments[attachmentName];
                    aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;
                    aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::DepthWrite;

                    platformTransitionBarrier3(
                        aBarriers,
                        commandBuffer,
                        1,
                        RenderDriver::Common::CCommandQueue::Type::Graphics
                    );
                }
                else
                {
                    RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                    aBarriers[0].mpImage = pRenderJob->mapInputImageAttachments[attachmentName];
                    aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::None;
                    aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::None;

                    platformTransitionBarrier3(
                        aBarriers,
                        commandBuffer,
                        1,
                        RenderDriver::Common::CCommandQueue::Type::Graphics
                    );
                }
            }
        }

        /*
        **
        */
        void CRenderer::filloutGraphicsJobCommandBuffer3(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer
        )
        {
            platformBeginDebugMarker3(
                pRenderJob->mName.c_str(),
                float4(0.0f, 1.0f, 0.0f, 1.0f),
                &commandBuffer
            );

//DEBUG_PRINTF("render job: \"%s\"\n", pRenderJob->mName.c_str());
            
            // queue type
            RenderDriver::Common::CCommandQueue::Type queueType = RenderDriver::Common::CCommandQueue::Type::Graphics;
            if(pRenderJob->mType == Render::Common::JobType::Compute)
            {
                queueType = RenderDriver::Common::CCommandQueue::Type::Compute;
            }
            else if(pRenderJob->mType == Render::Common::JobType::Copy)
            {
                queueType = RenderDriver::Common::CCommandQueue::Type::Copy;
            }

            // resource barrier and clear swap chain
            if(pRenderJob->mPassType == PassType::SwapChain)
            {
                mpSwapChain->transitionRenderTarget(
                    commandBuffer,
                    RenderDriver::Common::ResourceStateFlagBits::Present,
                    RenderDriver::Common::ResourceStateFlagBits::RenderTarget);

                RenderDriver::Common::ClearRenderTargetDescriptor clearDesc;
                clearDesc.mafClearColor[0] = 0.0f;
                clearDesc.mafClearColor[1] = 0.0f;
                clearDesc.mafClearColor[2] = 0.3f;
                clearDesc.mafClearColor[3] = 0.0f;
                clearDesc.miFrameIndex = mpSwapChain->getCurrentBackBufferIndex();
                clearDesc.mpCommandBuffer = &commandBuffer;

                mpSwapChain->clear(clearDesc);
            }

            // transition barriers to read state
            std::vector<char> acPlatformAttachmentInfo;
            platformTransitionInputImageAttachments(
                pRenderJob,
                acPlatformAttachmentInfo,
                commandBuffer,
                false
            );
            
            // begin render pass
            RenderPassDescriptor2 renderPassDesc = {};
            renderPassDesc.mpCommandBuffer = &commandBuffer;
            renderPassDesc.mpPipelineState = pRenderJob->mpPipelineState;
            renderPassDesc.miOffsetX = renderPassDesc.miOffsetY = 0;
            renderPassDesc.miOutputWidth = uint32_t(float(mDesc.miScreenWidth) * pRenderJob->mViewportScale.x);
            renderPassDesc.miOutputHeight = uint32_t(float(mDesc.miScreenHeight) * pRenderJob->mViewportScale.y);
            renderPassDesc.mpRenderJob = pRenderJob;
            renderPassDesc.miSwapChainFrameBufferindex = mpSwapChain->getCurrentBackBufferIndex();
            platformBeginRenderPass2(renderPassDesc);

            // pipeline state and descriptor set
            platformSetGraphicsDescriptorSet(
                *(pRenderJob->mpDescriptorSet),
                commandBuffer,
                *(pRenderJob->mpPipelineState)
            );

            RenderDriver::Common::CDescriptorHeap& descriptorHeap = *(pRenderJob->mpDescriptorHeap);
            platformSetDescriptorHeap(descriptorHeap, commandBuffer);

            // TODO: set shader resources correctly for DX12
            // set resource views
            std::vector<SerializeUtils::Common::ShaderResourceInfo> aShaderResources;
            uint32_t iTripleBufferIndex = miFrameIndex % 3;
            platformSetResourceViews(
                aShaderResources,
                commandBuffer,
                descriptorHeap,
                iTripleBufferIndex,
                Render::Common::JobType::Graphics);

            // viewport and scissor
            platformSetViewportAndScissorRect(
                uint32_t(float(mDesc.miScreenWidth) * pRenderJob->mViewportScale.x),
                uint32_t(float(mDesc.miScreenHeight) * pRenderJob->mViewportScale.y),
                mDesc.mfViewportMaxDepth,
                commandBuffer);

            if(pRenderJob->mPassType == PassType::FullTriangle || pRenderJob->mPassType == PassType::SwapChain)
            {
                platformSetVertexAndIndexBuffers2(
                    commandBuffer,
                    "full-screen-triangle"
                );
            }
            else if(pRenderJob->mPassType == PassType::DrawMeshes || pRenderJob->mPassType == PassType::DepthPrepass)
            {
                platformSetVertexAndIndexBuffers2(
                    commandBuffer,
                    "bistro"
                );
            }

            // root constants are set at the end of the descriptor set
            //if(pRenderJob->miNumRootConstants)
            //{
            //    platformSetGraphicsRoot32BitConstants(
            //        commandBuffer,
            //        pRenderJob->mafRootConstants,
            //        pRenderJob->miNumRootConstants,
            //        static_cast<uint32_t>(pRenderJob->maShaderResourceInfo.size()),
            //        0);
            //}

            std::vector<RenderDriver::Common::CDescriptorHeap*> apRenderTargetDescriptorHeaps;
            std::vector<RenderDriver::Common::CDescriptorHeap*> apDepthStencilDescriptorHeaps;

            // render targets to clear and barriers to transition
            uint32_t iNumRenderTargetAttachments = (uint32_t)pRenderJob->mapOutputImageAttachments.size();
            std::vector<bool> abClear(iNumRenderTargetAttachments);
            std::vector<std::vector<float>> aafClearColors(iNumRenderTargetAttachments);
            uint32_t iIndex = 0;
            for(uint32_t i = 0; i < pRenderJob->mapOutputImageAttachments.size(); i++)
            {
                aafClearColors[iIndex].resize(4);
                aafClearColors[iIndex][0] = 0.0f; aafClearColors[iIndex][1] = 0.0f; aafClearColors[iIndex][2] = 0.3f; aafClearColors[iIndex][3] = 0.0f;
                ++iIndex;
            }

            // depth stencil descriptor heaps
            if(iNumRenderTargetAttachments > 0)
            {
                RenderDriver::Common::CDescriptorHeap& depthStencilDescriptorHeap = *pRenderJob->mpDepthStencilDescriptorHeap;
                apDepthStencilDescriptorHeaps.push_back(&depthStencilDescriptorHeap);
            }

            // clear
            if(iNumRenderTargetAttachments > 0)
            {
                platformSetRenderTargetAndClear2(
                    commandBuffer,
                    apRenderTargetDescriptorHeaps,
                    apDepthStencilDescriptorHeaps,
                    static_cast<uint32_t>(apRenderTargetDescriptorHeaps.size()),
                    aafClearColors,
                    abClear);
            }

            commandBuffer.setPipelineState(
                *(pRenderJob->mpPipelineState),
                *mpDevice
            );

            if(pRenderJob->mPassType == PassType::FullTriangle || pRenderJob->mPassType == PassType::SwapChain)
            {
                commandBuffer.drawIndexInstanced(
                    3,
                    1,
                    0,
                    0,
                    0,
                    mapIndexBuffers["full-screen-triangle"]);
            }
            else if(pRenderJob->mPassType == PassType::DrawMeshes || pRenderJob->mPassType == PassType::DepthPrepass)
            {
                auto& pDrawIndexedCallsBuffer = mapRenderJobs["Mesh Culling Compute"]->mapOutputBufferAttachments["Draw Indexed Calls"];
                auto& pDrawIndexedCallCountBuffer = mapRenderJobs["Mesh Culling Compute"]->mapOutputBufferAttachments["Draw Indexed Call Count"];
                auto& pMeshIndexBuffer = mapIndexBuffers["bistro"];
                
                uint32_t iCountBufferOffset = (pRenderJob->mName == "Secondary Deferred Indirect Graphics") ? sizeof(uint32_t) * 2 : 0;
                
                uint32_t iDrawCommandOffset = 0;
                if(pRenderJob->mName == "Secondary Deferred Indirect Graphics")
                {
                    std::vector<uint32_t> aiCountData(256);
                    platformCopyBufferToCPUMemory(
                      pDrawIndexedCallCountBuffer,
                      aiCountData.data(),
                      0,
                      32);
                    iDrawCommandOffset = aiCountData[0];
                }
                
                commandBuffer.drawIndirectCount(
                    *pDrawIndexedCallsBuffer,
                    *pDrawIndexedCallCountBuffer,
                    *pMeshIndexBuffer,
                    iCountBufferOffset,
                    iDrawCommandOffset
                );
            }

            platformEndRenderPass2(renderPassDesc);

            // transition barriers back to original
            platformTransitionInputImageAttachments(
                pRenderJob,
                acPlatformAttachmentInfo,
                commandBuffer,
                true
            );

            platformEndDebugMarker3(&commandBuffer);
        }

        /*
        **
        */
        void CRenderer::filloutComputeJobCommandBuffer3(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            int3 const& dispatches
        )
        {
            platformBeginDebugMarker3(
                pRenderJob->mName.c_str(),
                float4(0.0f, 1.0f, 0.0f, 1.0f),
                &commandBuffer
            );

//DEBUG_PRINTF("render job: \"%s\"\n", pRenderJob->mName.c_str());

            uint32_t iTripleBufferIndex = miFrameIndex % 3;

            uint32_t iNumDispatchX = pRenderJob->maiDispatchSize[0];
            uint32_t iNumDispatchY = pRenderJob->maiDispatchSize[1];
            uint32_t iNumDispatchZ = pRenderJob->maiDispatchSize[2];
            if(dispatches.x > 0 && dispatches.y > 0 && dispatches.z > 0)
            {
                iNumDispatchX = dispatches.x;
                iNumDispatchY = dispatches.y;
                iNumDispatchZ = dispatches.z;
            }

            if(iNumDispatchX <= 0 && iNumDispatchY <= 0 && iNumDispatchZ <= 0)
            {
                commandBuffer.close();
            }
            
            // TODO: need platformBeginComputePass() to initialize command buffer and command encode for metal
            platformBeginComputePass(
                *pRenderJob,
                commandBuffer);
            
            
            // set pipeline and all the shader resource bindings
            platformSetComputeDescriptorSet(
                *pRenderJob->mpDescriptorSet, 
                commandBuffer, 
                *pRenderJob->mpPipelineState
            );
            platformSetDescriptorHeap(
                *pRenderJob->mpDescriptorHeap, 
                commandBuffer
            );

            // set resource views
            // TODO: set the correct shadre resources for DX12
            std::vector<SerializeUtils::Common::ShaderResourceInfo> aShaderResources;
            platformSetResourceViews(
                aShaderResources,
                commandBuffer,
                *pRenderJob->mpDescriptorHeap,
                iTripleBufferIndex,
                Render::Common::JobType::Compute
            );

            // transition barriers to read state
            std::vector<char> acPlatformAttachmentInfo;
            platformTransitionInputImageAttachments(
                pRenderJob,
                acPlatformAttachmentInfo,
                commandBuffer,
                false
            );

            commandBuffer.setPipelineState(
                *pRenderJob->mpPipelineState, 
                *mpDevice
            );

            commandBuffer.dispatch(
                iNumDispatchX, 
                iNumDispatchY, 
                iNumDispatchZ
            );

            // transition barrier back to original
            platformTransitionInputImageAttachments(
                pRenderJob,
                acPlatformAttachmentInfo,
                commandBuffer,
                true
            );

            platformEndDebugMarker3(&commandBuffer);
        }

        /*
        **
        */
        void CRenderer::filloutRayTraceJobCommandBuffer(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer
        )
        {
            platformBeginDebugMarker3(
                pRenderJob->mName.c_str(),
                float4(0.0f, 1.0f, 0.0f, 1.0f),
                &commandBuffer
            );

            // transition barriers to read state
            std::vector<char> acPlatformAttachmentInfo;
            platformTransitionInputImageAttachments(
                pRenderJob,
                acPlatformAttachmentInfo,
                commandBuffer,
                false
            );

            //transitionInputAttachmentsStart(
            //    pRenderJob,
            //    commandBuffer
            //);

            for(auto const& outputAttachmentKeyValue : pRenderJob->mapOutputImageAttachments)
            {
                std::string const& attachmentName = outputAttachmentKeyValue.first;
                RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                aBarriers[0].mpImage = pRenderJob->mapOutputImageAttachments[attachmentName];
                aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::None;
                aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;

                platformTransitionBarrier3(
                    aBarriers,
                    commandBuffer,
                    1,
                    RenderDriver::Common::CCommandQueue::Type::Graphics
                );
            }

            platformBeginRayTracingPass(
                *pRenderJob,
                commandBuffer);
            
            platformSetRayTraceDescriptorSet(
                *pRenderJob->mpDescriptorSet,
                commandBuffer,
                *pRenderJob->mpPipelineState
            );
            platformSetDescriptorHeap(
                *pRenderJob->mpDescriptorHeap,
                commandBuffer
            );

            // set pipeline and all the shader resource bindings
            commandBuffer.setPipelineState(
                *pRenderJob->mpPipelineState,
                *mpDevice
            );
            
            platformRayTraceCommand(
                pRenderJob,
                commandBuffer,
                mDesc.miScreenWidth,
                mDesc.miScreenHeight
            );

            platformTransitionInputImageAttachments(
                pRenderJob,
                acPlatformAttachmentInfo,
                commandBuffer,
                true
            );

            platformTransitionOutputAttachmentsRayTrace(
                pRenderJob,
                commandBuffer
            );

            platformEndDebugMarker3(
                &commandBuffer
            );

        }

        /*
        **
        */
        void CRenderer::execRenderJobs3()
        {
            RenderDriver::Common::CCommandQueue* pGraphicsCommandQueue = mpGraphicsCommandQueue.get();
            RenderDriver::Common::CCommandQueue* pComputeCommandQueue = mpComputeCommandQueue.get();
            RenderDriver::Common::CCommandQueue* pCopyCommandQueue = mpCopyCommandQueue.get();

            platformPreRenderJobExec();
            
            uint32_t iJobIndex = 0;
            bool bHasSwapChainPass = false;
            for(auto const& renderJobName : maRenderJobNames)
            {
#if !defined(USE_RAY_TRACING)
                if(renderJobName.find("Light Composite") != std::string::npos ||
                   renderJobName.find("Temporal Accumulation Graphics") != std::string::npos ||
                   renderJobName.find("Diffuse Filter Graphics") != std::string::npos)
                {
                    continue;
                }
#endif // USE_RAY_TRACING

                Render::Common::CRenderJob* pRenderJob = mapRenderJobs[renderJobName];
                RenderDriver::Common::CCommandBuffer* pCommandBuffer = mapRenderJobCommandBuffers[renderJobName];
                if(mRenderDriverType == RenderDriverType::Metal)
                {
                    pCommandBuffer = mapRenderJobCommandBuffers["Mesh Culling Compute"];
                }
                RenderDriver::Common::CCommandBuffer& commandBuffer = *pCommandBuffer;
                
                if(pRenderJob->mPassType == Render::Common::PassType::SwapChain)
                {
                    bHasSwapChainPass = true;
                }

                if(mRenderDriverType != RenderDriverType::Metal)
                {
                    commandBuffer.reset();
                }
                
                // fill out command buffers
                if(pRenderJob->mType == Render::Common::JobType::Graphics)
                {
                    filloutGraphicsJobCommandBuffer3(
                        pRenderJob,
                        commandBuffer
                    );

                    // transition swap chain image to present
                    if(pRenderJob->mPassType == Render::Common::PassType::SwapChain)
                    {
                        RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                        uint32_t iTripleBufferIndex = mpSwapChain->getCurrentBackBufferIndex();
                        aBarriers[0].mpImage = mpSwapChain->getColorRenderTarget(iTripleBufferIndex);
                        aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::None;
                        aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::Present2;

                        platformTransitionBarrier3(
                            aBarriers,
                            commandBuffer,
                            1,
                            RenderDriver::Common::CCommandQueue::Type::Graphics
                        );
                    }
                }
                else if(pRenderJob->mType == Render::Common::JobType::Compute)
                {
                    filloutComputeJobCommandBuffer3(
                        pRenderJob,
                        commandBuffer,
                        int3(pRenderJob->maiDispatchSize[0], pRenderJob->maiDispatchSize[1], pRenderJob->maiDispatchSize[2])
                    );
                }
                else if(pRenderJob->mType == Render::Common::JobType::Copy)
                {
                    platformBeginDebugMarker3(
                        pRenderJob->mName.c_str(),
                        float4(0.0f, 1.0f, 0.0f, 1.0f),
                        &commandBuffer
                    );

                    platformBeginCopyPass(
                        *pRenderJob,
                        commandBuffer
                    );
                    
                    for(auto& copyAttachment : pRenderJob->maCopyAttachmentMapping)
                    {
#if !defined(USE_RAY_TRACING)
                        Render::Common::CRenderJob* pParentJob = mapRenderJobs[copyAttachment.second.second];
                        if(pParentJob->mType == Render::Common::JobType::RayTrace)
                        {
                            continue;
                        }
#endif // USE_RAY_TRACING

                        std::string destAttachmentName = copyAttachment.second.second + "-" + copyAttachment.second.first;

                        WTFASSERT(pRenderJob->mapInputImageAttachments.count(destAttachmentName) > 0,
                            "Can't find input \"%s\"",
                            copyAttachment.first.c_str());

                        WTFASSERT(pRenderJob->mapOutputImageAttachments.count(copyAttachment.first) > 0,
                            "Can't find input \"%s\"",
                            copyAttachment.first.c_str());

                        RenderDriver::Common::CImage* pSrcImage = pRenderJob->mapInputImageAttachments[destAttachmentName];
                        RenderDriver::Common::CImage* pDestImage = pRenderJob->mapOutputImageAttachments[copyAttachment.first];

                        platformCopyImage2(
                            *pDestImage,
                            *pSrcImage,
                            commandBuffer
                        );
                    }
                
                    platformEndDebugMarker3(&commandBuffer);
                }
                else if(pRenderJob->mType == Render::Common::JobType::RayTrace)
                {
                    filloutRayTraceJobCommandBuffer(
                        pRenderJob,
                        commandBuffer
                    );
                }

                commandBuffer.close();

                if(mRenderDriverType == RenderDriverType::Metal)
                {
                    //debugRenderJobOutputAttachments(pRenderJob);
                    
                    ++iJobIndex;
                    continue;
                }
                
                // execute the command buffer
                if(pRenderJob->mType == Render::Common::JobType::Graphics)
                {
                    if(mRenderDriverType == RenderDriverType::Metal &&
                       pRenderJob->mPassType == Render::Common::PassType::SwapChain)
                    {
                        platformPreSwapChainPassSubmission(
                            *pRenderJob,
                            commandBuffer);
                    }
                    
                    pGraphicsCommandQueue->execCommandBuffer3(
                        commandBuffer,
                        &pRenderJob->miWaitSemaphoreValue,
                        &pRenderJob->miSignalSemaphoreValue,
                        pRenderJob->mpWaitFence,
                        pRenderJob->mpSignalFence
                        );
                    
                    /*for(auto& keyValue : pRenderJob->mapOutputImageAttachments)
                    {
                        if(keyValue.second == nullptr)
                        {
                            continue;
                        }
                        
                        RenderDriver::Common::CImage* pImage = keyValue.second;
                        std::string baseName = pRenderJob->mName + "-" + keyValue.first;
                        std::replace(baseName.begin(), baseName.end(), ' ', '-');
                        
                        std::vector<float> afImageData;
                        platformCopyImageToCPUMemory(pImage, afImageData);
                        
                        int32_t iImageWidth = pImage->getDescriptor().miWidth;
                        int32_t iImageHeight = pImage->getDescriptor().miHeight;
                        std::string outputDir = std::string("/Users/dingwings/Downloads/debug-output-attachments/") + baseName + ".hdr";
                        int32_t iRet = stbi_write_hdr(outputDir.c_str(), iImageWidth, iImageHeight, 4, afImageData.data());
                        WTFASSERT(iRet > 0, "can\'t write file: %s", outputDir.c_str());
                        DEBUG_PRINTF("wrote to: %s\n", outputDir.c_str());
                    }*/
            
                }
                else if(pRenderJob->mType == Render::Common::JobType::Compute)
                {
                    pComputeCommandQueue->execCommandBuffer3(
                        commandBuffer,
                        &pRenderJob->miWaitSemaphoreValue,
                        &pRenderJob->miSignalSemaphoreValue,
                        pRenderJob->mpWaitFence,
                        pRenderJob->mpSignalFence
                    );
                }
                else if(pRenderJob->mType == Render::Common::JobType::Copy)
                {
                    pCopyCommandQueue->execCommandBuffer3(
                        commandBuffer,
                        &pRenderJob->miWaitSemaphoreValue,
                        &pRenderJob->miSignalSemaphoreValue,
                        pRenderJob->mpWaitFence,
                        pRenderJob->mpSignalFence
                    );
                }
                else if(pRenderJob->mType == Render::Common::JobType::RayTrace)
                {
                    pGraphicsCommandQueue->execCommandBuffer3(
                        commandBuffer,
                        &pRenderJob->miWaitSemaphoreValue,
                        &pRenderJob->miSignalSemaphoreValue,
                        pRenderJob->mpWaitFence,
                        pRenderJob->mpSignalFence
                    );
                    
                    /*for(auto& keyValue : pRenderJob->mapOutputImageAttachments)
                    {
                        if(keyValue.second == nullptr)
                        {
                            continue;
                        }
                        
                        RenderDriver::Common::CImage* pImage = keyValue.second;
                        std::string baseName = pRenderJob->mName + "-" + keyValue.first;
                        std::replace(baseName.begin(), baseName.end(), ' ', '-');
                        
                        std::vector<float> afImageData;
                        platformCopyImageToCPUMemory(pImage, afImageData);
                        
                        int32_t iImageWidth = pImage->getDescriptor().miWidth;
                        int32_t iImageHeight = pImage->getDescriptor().miHeight;
                        std::string outputDir = std::string("/Users/dingwings/Downloads/debug-output-attachments/") + baseName + ".hdr";
                        int32_t iRet = stbi_write_hdr(outputDir.c_str(), iImageWidth, iImageHeight, 4, afImageData.data());
                        WTFASSERT(iRet > 0, "can\'t write file: %s", outputDir.c_str());
                        DEBUG_PRINTF("wrote to: %s\n", outputDir.c_str());
                    }*/
                }

                ++iJobIndex;

            }
            
            // transition swap chain image to present if no swap chain pass is listed
            if(!bHasSwapChainPass)
            {
                if(mpSwapChainCommandBuffer == nullptr)
                {
                    platformCreateSwapChainCommandBuffer();
                }

                mpSwapChainCommandBuffer->reset();

                RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                uint32_t iTripleBufferIndex = mpSwapChain->getCurrentBackBufferIndex();
                aBarriers[0].mpImage = mpSwapChain->getColorRenderTarget(iTripleBufferIndex);
                aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::None;
                aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::Present2;
                
                platformTransitionBarrier3(
                    aBarriers,
                    *mpSwapChainCommandBuffer,
                    1,
                    RenderDriver::Common::CCommandQueue::Type::Graphics
                );

                mpSwapChainCommandBuffer->close();
                pGraphicsCommandQueue->execCommandBuffer(
                    *mpSwapChainCommandBuffer,
                    *mpDevice
                );
            }
            
            if(mRenderDriverType == Render::Common::RenderDriverType::Metal)
            {
                platformPostRenderJobExec();
            }
            
            // final wait for queue to finish
            mapCommandQueueFences[RenderDriver::Common::CCommandQueue::Type::Graphics]->waitCPU2(
                UINT32_MAX,
                pGraphicsCommandQueue
            );
            mapCommandQueueFences[RenderDriver::Common::CCommandQueue::Type::Compute]->waitCPU2(
                UINT32_MAX,
                pComputeCommandQueue
            );

            if(mRenderDriverType == RenderDriverType::Metal)
            {
                platformPrepSwapChain();
            }
            
            // reset all render job fences
            for(auto const& renderJobName : maRenderJobNames)
            {
#if !defined(USE_RAY_TRACING)
                if(renderJobName.find("Light Composite") != std::string::npos ||
                   renderJobName.find("Temporal Accumulation Graphics") != std::string::npos ||
                   renderJobName.find("Diffuse Filter Graphics") != std::string::npos)
                {
                    continue;
                }
#endif // USE_RAY_TRACING

                Render::Common::CRenderJob* pRenderJob = mapRenderJobs[renderJobName];
                            
                pRenderJob->mpSignalFence->reset2();
                if(pRenderJob->mpWaitFence != nullptr)
                {
                    pRenderJob->mpWaitFence->reset2();
                }
                
                mapRenderJobCommandBuffers[pRenderJob->mName]->reset();
            }

            ++miFrameIndex;
        }

        /*
        **
        */
        void CRenderer::initData()
        {
            FILE* fp = nullptr;
#if defined(_MSC_VER)
            fp = fopen("d:\\Downloads\\Bistro_v4\\bistro2-triangles.bin", "rb");
#else
            std::string fullPath;
            getAssetsDir(fullPath, "bistro2-triangles.bin");
            fp = fopen(fullPath.c_str(), "rb");
#endif // _MSC_VER
            
            // load binary data
            fseek(fp, 0, SEEK_END);
            size_t iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<char> acMeshBuffer(iFileSize);
            fread(acMeshBuffer.data(), sizeof(char), iFileSize, fp);
            fclose(fp);

            uint32_t const* paiMeshBuffer = (uint32_t*)acMeshBuffer.data();

            // info header
            miNumMeshes = *paiMeshBuffer++;
            uint32_t iNumTotalVertices = *paiMeshBuffer++;
            uint32_t iNumTotalTriangles = *paiMeshBuffer++;
            uint32_t iVertexSize = *paiMeshBuffer++;
            //uint32_t iTriangleStartOffset = *paiMeshBuffer++;
            ++paiMeshBuffer;
            
            // mesh ranges
            //std::vector<std::pair<uint32_t, uint32_t>> aMeshRanges;
            for(uint32_t iMesh = 0; iMesh < miNumMeshes; iMesh++)
            {
                uint32_t iOffsetStart = *paiMeshBuffer++;
                uint32_t iOffsetEnd = *paiMeshBuffer++;
                maMeshRanges.push_back(std::make_pair(iOffsetStart, iOffsetEnd));
            }

            // mesh bounding boxes
            float4* pExtent = (float4*)paiMeshBuffer;
            //std::vector<std::pair<float4, float4>> aMeshExtents;
            for(uint32_t iMesh = 0; iMesh < miNumMeshes; iMesh++)
            {
                float4 minPosition = *pExtent++;
                float4 maxPosition = *pExtent++;
                maMeshExtents.push_back(std::make_pair(minPosition, maxPosition));
            }
            WTFASSERT(sizeof(GPUVertexFormat) == iVertexSize, "vertex size not equal");
            
            // vertices
            GPUVertexFormat* pVertices = (GPUVertexFormat*)pExtent;
            std::vector<GPUVertexFormat> aVertexBuffer(iNumTotalVertices);
            memcpy(aVertexBuffer.data(), pVertices, sizeof(GPUVertexFormat) * iNumTotalVertices);
            pVertices += iNumTotalVertices;

            for(uint32_t i = 0; i < aVertexBuffer.size(); i++)
            {
                aVertexBuffer[i].mTexCoord.z = aVertexBuffer[i].mPosition.w;
            }
            
            // indices
            uint32_t const* pIndices = (uint32_t const*)pVertices;
            std::vector<uint32_t> aiIndexBuffer(iNumTotalTriangles * 3);
            memcpy(aiIndexBuffer.data(), pIndices, sizeof(uint32_t) * iNumTotalTriangles * 3);

            uint32_t iVertexBufferSize = iNumTotalVertices * sizeof(GPUVertexFormat);
            uint32_t iIndexBufferSize = iNumTotalTriangles * 3 * sizeof(uint32_t);

            // register vertex and index gpu buffers
            std::string meshName = "bistro";
            platformCreateVertexAndIndexBuffer(
                meshName,
                iVertexBufferSize,
                iIndexBufferSize);

            // copy data to registered gpu buffers
            uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
            copyCPUToBuffer(
                mapVertexBuffers[meshName],
                aVertexBuffer.data(),
                0,
                iVertexBufferSize,
                iFlags
            );
            copyCPUToBuffer(
                mapIndexBuffers[meshName],
                aiIndexBuffer.data(),
                0,
                iIndexBufferSize,
                iFlags
            );
            
            // triangle positions
            {
                std::vector<float4> aPositions(iNumTotalVertices);
                for(uint32_t i = 0; i < iNumTotalVertices; i++)
                {
                    aPositions[i] = float4(aVertexBuffer[i].mPosition, 1.0f);
                }
                copyCPUToBuffer(
                    mapTrianglePositionBuffers[meshName],
                    aPositions.data(),
                    0,
                    iNumTotalVertices * sizeof(float4),
                    iFlags
                );

//#if defined(USE_RAY_TRACING)
                platformCreateAccelerationStructures(
                    aPositions,
                    aiIndexBuffer,
                    maMeshRanges,
                    miNumMeshes
                );
//#endif // USE_RAY_TRACING
            }

            // full screen triangle
            Render::Common::VertexFormat aTriangleVertices[3];
            aTriangleVertices[0].mPosition = float4(-1.0f, 3.0f, 0.0f, 1.0f);
            aTriangleVertices[0].mNormal = float4(0.0f, 0.0f, 1.0f, 1.0f);
            aTriangleVertices[0].mTexCoord = float4(0.0f, -1.0f, 0.0f, 0.0f);

            aTriangleVertices[1].mPosition = float4(-1.0f, -1.0f, 0.0f, 1.0f);
            aTriangleVertices[1].mNormal = float4(0.0f, 0.0f, 1.0f, 1.0f);
            aTriangleVertices[1].mTexCoord = float4(0.0f, 1.0f, 0.0f, 0.0f);

            aTriangleVertices[2].mPosition = float4(3.0f, -1.0f, 0.0f, 1.0f);
            aTriangleVertices[2].mNormal = float4(0.0f, 0.0f, 1.0f, 1.0f);
            aTriangleVertices[2].mTexCoord = float4(2.0f, 1.0f, 0.0f, 0.0f);

            uint32_t aiTriangleIndices[3] = {0, 1, 2};

            platformCreateVertexAndIndexBuffer(
                "full-screen-triangle",
                sizeof(Render::Common::VertexFormat) * 3,
                sizeof(uint32_t) * 3
            );

            copyCPUToBuffer(
                mapVertexBuffers["full-screen-triangle"],
                aTriangleVertices,
                0,
                sizeof(Render::Common::VertexFormat) * 3,
                iFlags
            );
            copyCPUToBuffer(
                mapIndexBuffers["full-screen-triangle"],
                aiTriangleIndices,
                0,
                sizeof(uint32_t) * 3,
                iFlags
            );

        }

        /*
        **
        */
        void CRenderer::prepareRenderJobData()
        {
            //if(mpfnPrepareRenderJobData)
            //{
            //    (*mpfnPrepareRenderJobData)(this);
            //}

            uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);

            // upload mesh index ranges and bounding boxes
            auto& pMeshIndexRangeBuffer = mapRenderJobs["Mesh Culling Compute"]->mapUniformBuffers["meshIndexRanges"];
            auto& pMeshBoundingBoxBuffer = mapRenderJobs["Mesh Culling Compute"]->mapUniformBuffers["meshBoundingBoxes"];
            auto& pUniformDataBuffer = mapRenderJobs["Mesh Culling Compute"]->mapUniformBuffers["uniformData"];
            copyCPUToBuffer(
                pMeshIndexRangeBuffer,
                maMeshRanges.data(),
                0,
                miNumMeshes * sizeof(uint32_t) * 2,
                iFlags
            );
            copyCPUToBuffer(
                pMeshBoundingBoxBuffer,
                maMeshExtents.data(),
                0,
                miNumMeshes * sizeof(float4) * 2,
                iFlags
            );
            copyCPUToBuffer(
                pUniformDataBuffer,
                &miNumMeshes,
                0,
                sizeof(uint32_t),
                iFlags
            );

            // material id for texture atlas pages
            {
                auto& pMaterialIDBuffer = mapRenderJobs["Texture Page Queue Compute"]->mapUniformBuffers["meshMaterialIDs"];
                
                std::string fullPath;
#if defined(_MSC_VER)
                fullPath = "d:\\downloads\\Bistro_v4\\bistro2.mid";
#else
                getAssetsDir(fullPath, "bistro2.mid");
#endif // _MSC_VER
                
                FILE* fp = fopen(fullPath.c_str(), "rb");
                fseek(fp, 0, SEEK_END);
                size_t iFileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                std::vector<char> acBuffer(iFileSize);
                fread(acBuffer.data(), sizeof(char), iFileSize, fp);
                fclose(fp);
                copyCPUToBuffer(
                    pMaterialIDBuffer,
                    acBuffer.data(),
                    0,
                    (uint32_t)iFileSize,
                    iFlags
                );
            }

#if 0
            // material
            std::vector<std::string> aAlbedoTextureNames;
            std::vector<std::string> aNormalTextureNames;
            //std::vector<uint2> aAlbedoTextureDimensions;
            std::vector<uint2> aNormalTextureDimensions;
            {
                auto& pMaterialBuffer = mapRenderJobs["Texture Page Queue Compute"]->mapUniformBuffers["materials"];
                FILE* fp = fopen("d:\\downloads\\Bistro_v4\\bistro2.mat", "rb");
                fseek(fp, 0, SEEK_END);
                size_t iFileSize = ftell(fp);
                fseek(fp, 0, SEEK_SET);
                std::vector<char> acBuffer(iFileSize);
                fread(acBuffer.data(), sizeof(char), iFileSize, fp);
                fclose(fp);

                // material id
                char* pacBuffer = acBuffer.data();
                uint32_t iCurrPos = 0;
                uint32_t iNumMaterials = 0;
                for(;;)
                {
                    uint32_t iMaterialID = *((uint32_t*)(pacBuffer + iCurrPos + 16 * 3));
                    if(iMaterialID >= 99999)
                    {
                        break;
                    }

                    iCurrPos += 16 * 4;
                    iNumMaterials += 1;
                }
                iCurrPos += 16 * 4;
                copyCPUToBuffer(
                    pMaterialBuffer,
                    acBuffer.data(),
                    0,
                    (uint32_t)iCurrPos
                );

                // albedo textures
                uint32_t iNumAlbedoTextures = *((uint32_t*)(pacBuffer + iCurrPos));
                iCurrPos += 4;
                for(uint32_t i = 0; i < iNumAlbedoTextures; i++)
                {
                    std::string albedoTextureName = "";
                    for(;;)
                    {
                        char cChar = *(pacBuffer + iCurrPos);
                        if(cChar == '\n')
                        {
                            aAlbedoTextureNames.push_back(albedoTextureName);
                            break;
                        }
                        albedoTextureName += cChar;
                        iCurrPos += 1;
                    }
                    iCurrPos += 1;
                }

                // normal texture
                uint32_t iNumNormalTextures = *((uint32_t*)(pacBuffer + iCurrPos));
                iCurrPos += 4;
                for(uint32_t i = 0; i < iNumNormalTextures; i++)
                {
                    std::string normalTextureName = "";
                    for(;;)
                    {
                        char cChar = *(pacBuffer + iCurrPos);
                        if(cChar == '\n')
                        {
                            aNormalTextureNames.push_back(normalTextureName);
                            break;
                        }
                        normalTextureName += cChar;
                        iCurrPos += 1;
                    }
                    iCurrPos += 1;
                }

                maAlbedoTextureDimensions.resize(iNumAlbedoTextures);
                fp = fopen("d:\\Downloads\\Bistro_v4\\converted-dds\\albedo-dimensions.txt", "rb");
                fread(maAlbedoTextureDimensions.data(), sizeof(uint2), iNumAlbedoTextures, fp);
                fclose(fp);

                aNormalTextureDimensions.resize(iNumNormalTextures);
                fp = fopen("d:\\Downloads\\Bistro_v4\\converted-dds\\normal-dimensions.txt", "rb");
                fread(aNormalTextureDimensions.data(), sizeof(uint2), iNumNormalTextures, fp);
                fclose(fp);
            }
#endif // #if 0

#if 0
            auto getBaseName = [](std::string const& name)
            {
                auto extensionStart = name.find('.');
                auto baseNameStart = name.find_last_of("\\");
                if(baseNameStart == std::string::npos)
                {
                    baseNameStart = name.find_last_of("/");
                }
                if(baseNameStart == std::string::npos)
                {
                    baseNameStart = 0;
                }
                else
                {
                    baseNameStart += 1;
                }

                std::string baseName = name.substr(baseNameStart, extensionStart - baseNameStart);
                return baseName;
            };

            auto getDirectory = [](std::string const& name)
            {
                auto baseNameStart = name.find_last_of("\\");
                if(baseNameStart == std::string::npos)
                {
                    baseNameStart = name.find_last_of("/");
                }
                if(baseNameStart == std::string::npos)
                {
                    baseNameStart = 0;
                }
                

                std::string directory = name.substr(0, baseNameStart);
                return directory;
            };

            // albedo texture dimensions
            for(auto const& albedoTexture : aAlbedoTextureNames)
            {
                std::string baseName = getBaseName(albedoTexture);
                std::string directory = getDirectory(albedoTexture);

                std::string fullPath = "d:\\Downloads\\Bistro_v4\\converted-dds\\" + baseName + ".png";
                int32_t iImageWidth = 0, iImageHeight = 0, iNumChannels = 0;
                stbi_uc* pImageData = stbi_load(fullPath.c_str(), &iImageWidth, &iImageHeight, &iNumChannels, 4);
                stbi_image_free(pImageData);
                aAlbedoTextureDimensions.push_back(uint2(iImageWidth, iImageHeight));
            }

            FILE* fp = fopen("d:\\Downloads\\Bistro_v4\\converted-dds\\albedo-dimensions.txt", "wb");
            fwrite(aAlbedoTextureDimensions.data(), sizeof(uint2), aAlbedoTextureDimensions.size(), fp);
            fclose(fp);

            // normal texture dimensions
            for(auto const& normalTexture : aNormalTextureNames)
            {
                std::string baseName = getBaseName(normalTexture);
                std::string directory = getDirectory(normalTexture);

                std::string fullPath = "d:\\Downloads\\Bistro_v4\\converted-dds\\" + baseName + ".png";
                int32_t iImageWidth = 0, iImageHeight = 0, iNumChannels = 0;
                stbi_uc* pImageData = stbi_load(fullPath.c_str(), &iImageWidth, &iImageHeight, &iNumChannels, 4);
                stbi_image_free(pImageData);
                aNormalTextureDimensions.push_back(uint2(iImageWidth, iImageHeight));
            }

            fp = fopen("d:\\Downloads\\Bistro_v4\\converted-dds\\normal-dimensions.txt", "wb");
            fwrite(aNormalTextureDimensions.data(), sizeof(uint2), aAlbedoTextureDimensions.size(), fp);
            fclose(fp);
#endif // #if 0

#if 0
            auto& pTextureDimensionBuffer = mapRenderJobs["Texture Page Queue Compute"]->mapUniformBuffers["Texture Sizes"];
            copyCPUToBuffer(
                pTextureDimensionBuffer,
                maAlbedoTextureDimensions.data(),
                0,
                uint32_t(maAlbedoTextureDimensions.size() * sizeof(uint2))
            );

            auto& pNormalTextureDimensionBuffer = mapRenderJobs["Texture Page Queue Compute"]->mapUniformBuffers["Normal Texture Sizes"];
            copyCPUToBuffer(
                pNormalTextureDimensionBuffer,
                aNormalTextureDimensions.data(),
                0,
                uint32_t(aNormalTextureDimensions.size() * sizeof(uint2)),
                iFlags
            );
#endif // #if 0

            if(mpfnInitData != nullptr)
            {
                (*mpfnInitData)(this);
            }
        }

        /*
        **
        */
        void CRenderer::updateRenderJobData()
        {
            std::vector<char> acData(1024);
            int32_t* piData = (int32_t*)acData.data(); 
            *piData++ = mDesc.miScreenWidth;
            *piData++ = mDesc.miScreenHeight;
            *piData++ = miFrameIndex;
            *piData++ = 0;

            float* pfData = (float*)piData;
            *pfData++ = float(rand() % 100) * 0.01f;
            *pfData++ = float(rand() % 100) * 0.01f;
            *pfData++ = float(rand() % 100) * 0.01f;
            *pfData++ = float(rand() % 100) * 0.01f;
            
            //float4x4 projectionMatrix = Render::Common::gaCameras[0].getProjectionMatrix();
            float4x4 viewMatrix = Render::Common::gaCameras[0].getViewMatrix();
            float4x4 jitterProjectionMatrix = Render::Common::gaCameras[0].getJitterProjectionMatrix();

            if(this->mRenderDriverType == RenderDriverType::Vulkan || this->mRenderDriverType == RenderDriverType::Metal)
            {
                jitterProjectionMatrix.mafEntries[5] *= -1.0f;
            }

            float4x4 jitterViewProjectionMatrix = jitterProjectionMatrix * viewMatrix;

            float4x4* pfMatrixData = (float4x4*)pfData;
            if(mRenderDriverType == RenderDriverType::Vulkan || mRenderDriverType == RenderDriverType::Metal)
            {
                *pfMatrixData++ = transpose(Render::Common::gaCameras[0].getViewProjectionMatrix());
                *pfMatrixData++ = mPrevViewProjectionMatrix;
                *pfMatrixData++ = transpose(Render::Common::gaCameras[0].getViewMatrix());
                *pfMatrixData++ = transpose(Render::Common::gaCameras[0].getProjectionMatrix());

                *pfMatrixData++ = transpose(jitterViewProjectionMatrix);
                *pfMatrixData++ = transpose(mPrevJitterViewProjectionMatrix);
                mPrevJitterViewProjectionMatrix = transpose(jitterViewProjectionMatrix);
                
                *pfMatrixData++ = invert(Render::Common::gaCameras[0].getViewProjectionMatrix());
                
                mPrevViewProjectionMatrix = transpose(Render::Common::gaCameras[0].getViewProjectionMatrix());
            }
            else
            {
                *pfMatrixData++ = Render::Common::gaCameras[0].getViewProjectionMatrix();
                *pfMatrixData++ = mPrevViewProjectionMatrix;
                *pfMatrixData++ = Render::Common::gaCameras[0].getViewMatrix();
                *pfMatrixData++ = Render::Common::gaCameras[0].getProjectionMatrix();

                *pfMatrixData++ = Render::Common::gaCameras[0].getViewProjectionMatrix();
                *pfMatrixData++ = mPrevViewProjectionMatrix;
                *pfMatrixData++ = invert(Render::Common::gaCameras[0].getViewProjectionMatrix());

                mPrevViewProjectionMatrix = Render::Common::gaCameras[0].getViewProjectionMatrix();
            }

            float4* pFloat4Data = (float4*)pfMatrixData;
            *pFloat4Data++ = float4(Render::Common::gaCameras[0].getPosition(), 1.0f);
            *pFloat4Data++ = float4(Render::Common::gaCameras[0].getLookAt(), 1.0f);
            
            //float3 diff = gLightDirection - gPrevLightDirection;
            //float fLengthSquared = dot(diff, diff);
            //float fRebuildSkyProbe = (fLengthSquared >= 0.1f) ? 1.0f : 0.0f;
            
            float4 lightRadiance = float4(1.5f, 1.5f, 1.5f, 1.0f);

            *pFloat4Data++ = lightRadiance;
            *pFloat4Data++ = float4(gLightDirection, 1.0f);

            *pFloat4Data++ = lightRadiance;
            *pFloat4Data++ = float4(gPrevLightDirection, 1.0f);

            pfData = (float*)pFloat4Data;
            *pfData++ = 1.0f;
            *pfData++ = gfEmissiveRadiance;
            *pfData++ = gfClearReservoir;
            *pfData++ = 1.0f;

            gPrevLightDirection = gLightDirection;

            // copy data to registered gpu buffers
            uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
            copyCPUToBuffer(
                mpDefaultUniformBuffer,
                acData.data(),
                0,
                (uint32_t)acData.size(),
                iFlags
            );

            // clear number of draw calls for mesh culling pass
            std::vector<uint32_t> aiResetData(32);
            memset(aiResetData.data(), 0, sizeof(uint32_t) * aiResetData.size());
            auto& pDrawCallCountBuffer = mapRenderJobs["Mesh Culling Compute"]->mapOutputBufferAttachments["Draw Indexed Call Count"];
            copyCPUToBuffer(
                pDrawCallCountBuffer,
                aiResetData.data(),
                0,
                (uint32_t)(aiResetData.size() * sizeof(uint32_t)),
                iFlags
            );

#if defined(USE_RAY_TRACING)
            if(mapRenderJobs.find("Build Irradiance Cache Ray Trace") != mapRenderJobs.end())
            {
                auto& pCounterBuffer = mapRenderJobs["Build Irradiance Cache Ray Trace"]->mapOutputBufferAttachments["Counters"];
                copyCPUToBuffer(
                    pCounterBuffer,
                    aiResetData.data(),
                    0,
                    (uint32_t)(aiResetData.size() * sizeof(uint32_t)),
                    iFlags
                );
            }
#endif // USE_RAY_TRACING

            gfClearReservoir = 0.0f;

        }

        /*
        **
        */
        void CRenderer::createCommandBuffer(
            std::unique_ptr<RenderDriver::Common::CCommandAllocator>& commandAllocator,
            std::unique_ptr<RenderDriver::Common::CCommandBuffer>& commandBuffer)
        {
            platformCreateCommandBuffer(
                commandAllocator,
                commandBuffer
            );
        }

        /*
        **
        */
        void CRenderer::createBuffer(
            std::unique_ptr<RenderDriver::Common::CBuffer>& buffer,
            uint32_t iSize)
        {
            platformCreateBuffer(
                buffer,
                iSize
            );
        }

        /*
        **
        */
        void CRenderer::createCommandQueue(
            std::unique_ptr<RenderDriver::Common::CCommandQueue>& commandQueue,
            RenderDriver::Common::CCommandQueue::Type const& type)
        {
            platformCreateCommandQueue(
                commandQueue,
                type
            );
        }

        /*
        **
        */
        void CRenderer::debugRenderJobOutputAttachments(Render::Common::CRenderJob* pRenderJob)
        {
            for(auto& keyValue : pRenderJob->mapOutputImageAttachments)
            {
                if(keyValue.second == nullptr)
                {
                    continue;
                }
                
                RenderDriver::Common::CImage* pImage = keyValue.second;
                if(pImage->getDescriptor().mFormat != RenderDriver::Common::Format::R32G32B32A32_FLOAT)
                {
                    continue;
                }
                
                std::string baseName = pRenderJob->mName + "-" + keyValue.first;
                std::replace(baseName.begin(), baseName.end(), ' ', '-');
                
                std::vector<float> afImageData;
                platformCopyImageToCPUMemory(pImage, afImageData);
                
                int32_t iImageWidth = pImage->getDescriptor().miWidth;
                int32_t iImageHeight = pImage->getDescriptor().miHeight;
                std::string outputDir = std::string("/Users/dingwings/Downloads/debug-output-attachments/") + baseName + ".exr";
                char const* pError = nullptr;
                int32_t iRet = SaveEXR(
                    afImageData.data(),
                    iImageWidth,
                    iImageHeight,
                    4,
                    0,
                    outputDir.c_str(),
                    &pError);
                WTFASSERT(iRet == TINYEXR_SUCCESS, "Can\'t save \"%s\"", outputDir.c_str());
                
                //int32_t iRet = stbi_write_hdr(outputDir.c_str(), iImageWidth, iImageHeight, 4, afImageData.data());
                //WTFASSERT(iRet > 0, "can\'t write file: %s", outputDir.c_str());
                DEBUG_PRINTF("wrote to: %s\n", outputDir.c_str());
            }
        }
    
    }   // Common

} // Render
