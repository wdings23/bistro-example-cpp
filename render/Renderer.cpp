#include <render/Renderer.h>

#include <render-driver/SwapChain.h>
#include <render-driver/Fence.h>
#include <render-driver/DX12/FenceDX12.h>
#include <mesh-clusters/MeshClusterManager.h>

#include <render/render_job_enums.h>
#include <render/RenderJob.h>

#include <JobManager.h>
#include <Camera.h>
#include <LogPrint.h>
#include <wtfassert.h>

#include <sstream>
#include <thread>
#include <mutex>

#include <stb_image.h>
#include <stb_image_write.h>

#include <chrono>


#if defined(RUN_TEST_LOCALLY)
std::vector<char> gaReadOnlyBufferCopy(1 << 27);
std::vector<char> gaReadWriteBufferCopy(1 << 27);
std::unique_ptr<RenderDriver::Common::CBuffer> gpCopyBuffer;
#endif // TEST_HLSL_LOCALLY

std::mutex sMutex;

namespace Render
{
    namespace Common
    {
        std::vector<CCamera>                 gaCameras;

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
            std::string const& renderJobFilePath)
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
                if(type == "RayTrace")
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
                if(type == "RayTrace")
                {
                    continue;
                }
#endif // !USE_RAY_TRACING

                std::string fullPath = topDirectory + "\\render-jobs\\" + pipeline;

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
                    &mapAccelerationStructures
                };
                mapRenderJobs[name]->mName = name;
                mapRenderJobs[name]->createOutputAttachments(createInfo);
                ++iJob;
            }

            // create command buffers for render jobs
            platformCreateRenderJobCommandBuffers(
                aRenderJobNames
            );

            platformTransitionOutputAttachments();

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
                if(type == "RayTrace")
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

            

            //separateOutRenderJobsByType();
            
            // create fences for each render jobs
            platformCreateRenderJobFences();

            platformPostSetup();
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

            
            RenderDriver::Common::SwapChainPresentDescriptor desc;
            desc.miSyncInterval = 0;
            desc.miFlags = 0x200;
            desc.mpDevice = mpDevice.get();
            desc.mpPresentQueue = mpGraphicsCommandQueue.get();

            //desc.miSyncInterval = 1;
            //desc.miFlags = 0;

            beginDebugMarker("Present");
            mpSwapChain->present(desc);
            endDebugMarker();

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
                auto start = std::chrono::high_resolution_clock::now();

                mpUploadFence->waitCPU(UINT64_MAX);

                uint64_t iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
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

            if((iFlags & static_cast<uint32_t>(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY)) > 0)
            {
                platformExecuteCopyCommandBuffer(*mpUploadCommandBuffer, iFlags);
                mpUploadCommandAllocator->reset();
                mpUploadCommandBuffer->reset();
            }
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
                    RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                    aBarriers[0].mpImage = pRenderJob->mapInputImageAttachments[attachmentName];
                    aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::None;
                    aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;

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
                    aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;

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

            // transition to pixel shader resource for attachment in swap chain pass
            if(pRenderJob->mPassType == PassType::SwapChain)
            {
                for(auto const& imageKeyValue : pRenderJob->mapInputImageAttachments)
                {
                    RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                    aBarriers[0].mpImage = pRenderJob->mapInputImageAttachments[imageKeyValue.first];
                    aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::None;
                    aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;
                
                    platformTransitionBarrier3(
                        aBarriers,
                        commandBuffer,
                        1,
                        queueType
                    );
                }
            }
            else
            {
                transitionInputAttachmentsStart(
                    pRenderJob,
                    commandBuffer
                );
            }

            // set the coorect output image layout for drawing
            for(auto& attachmentKeyValue : pRenderJob->mapOutputImageAttachments)
            {
                //DEBUG_PRINTF("*** OUTPUT ATTACHMENT ***\n");
                RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                aBarriers[0].mpImage = pRenderJob->mapOutputImageAttachments[attachmentKeyValue.first];
                aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::None;
                aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::ColorAttachment;
            
                platformTransitionBarrier3(
                    aBarriers,
                    commandBuffer,
                    1,
                    queueType
                );
            }

            if(pRenderJob->mpDepthImage)
            {
                RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[1];
                aBarriers[0].mpImage = pRenderJob->mpDepthImage;
                aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::None;
                aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::DepthStencilAttachment;

                platformTransitionBarrier3(
                    aBarriers,
                    commandBuffer,
                    1,
                    queueType
                );
            }
            

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

            float afClearColor[] = {0.0f, 0.0f, 0.3f, 0.0f};

            if(pRenderJob->mPassType == PassType::FullTriangle || pRenderJob->mPassType == PassType::SwapChain)
            {
                platformSetVertexAndIndexBuffers2(
                    commandBuffer,
                    "full-screen-triangle"
                );
            }
            else if(pRenderJob->mPassType == PassType::DrawMeshes)
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
            for(auto& attachmentKeyValue : pRenderJob->mapOutputImageAttachments)
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
                    0);
            }
            else if(pRenderJob->mPassType == PassType::DrawMeshes)
            {
                auto& pDrawIndexedCallsBuffer = mapRenderJobs["Mesh Culling Compute"]->mapOutputBufferAttachments["Draw Indexed Calls"];
                auto& pDrawIndexedCallCountBuffer = mapRenderJobs["Mesh Culling Compute"]->mapOutputBufferAttachments["Draw Indexed Call Count"];

                uint32_t iCountBufferOffset = (pRenderJob->mName == "Secondary Deferred Indirect Graphics") ? sizeof(uint32_t) * 2 : 0;

                commandBuffer.drawIndirectCount(
                    *pDrawIndexedCallsBuffer,
                    *pDrawIndexedCallCountBuffer,
                    iCountBufferOffset
                );
            }

            platformEndRenderPass2(renderPassDesc);

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

            // transition barriers for shader resources
            uint32_t iNumBarriers = 0;
            std::vector<RenderDriver::Common::Utils::TransitionBarrierInfo> aBarriers(pRenderJob->maShaderResourceInfo.size());
            for(auto& shaderKeyValue : pRenderJob->maShaderResourceInfo)
            {
                auto& shaderResourceName = shaderKeyValue.first;
                auto& shaderResourceMap = shaderKeyValue.second;
                std::string const& type = shaderResourceMap["type"];
                std::string const& usage = shaderResourceMap["usage"];
                if(type == "buffer" && usage == "write")
                {
                    aBarriers[iNumBarriers].mpBuffer = pRenderJob->mapUniformBuffers[shaderResourceName];
                    aBarriers[iNumBarriers].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumBarriers].mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;

                    WTFASSERT(aBarriers[iNumBarriers].mpBuffer, "no buffer");
                }
                else if(type == "texture" && usage == "write")
                {
                    aBarriers[iNumBarriers].mpImage = pRenderJob->mapUniformImages[shaderResourceName];
                    aBarriers[iNumBarriers].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumBarriers].mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;

                    WTFASSERT(aBarriers[iNumBarriers].mpImage, "no image");
                }
                else if(type == "buffer" && usage == "uniform")
                {
                    aBarriers[iNumBarriers].mpBuffer = pRenderJob->mapUniformBuffers[shaderResourceName];
                    aBarriers[iNumBarriers].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumBarriers].mAfter = RenderDriver::Common::ResourceStateFlagBits::NonPixelShaderResource;

                    WTFASSERT(aBarriers[iNumBarriers].mpBuffer, "no buffer");
                }
                else if(type == "texture")
                {
                    aBarriers[iNumBarriers].mpImage = pRenderJob->mapUniformImages[shaderResourceName];
                    aBarriers[iNumBarriers].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumBarriers].mAfter = RenderDriver::Common::ResourceStateFlagBits::NonPixelShaderResource;

                    WTFASSERT(aBarriers[iNumBarriers].mpImage, "no image");
                }

                ++iNumBarriers;
            }
            RenderDriver::Common::CCommandQueue::Type queueType = RenderDriver::Common::CCommandQueue::Type::Compute;
            transitionInputAttachmentsStart(
                pRenderJob,
                commandBuffer
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

            transitionInputAttachmentsStart(
                pRenderJob,
                commandBuffer
            );

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

            // set pipeline and all the shader resource bindings
            commandBuffer.setPipelineState(
                *pRenderJob->mpPipelineState,
                *mpDevice
            );

            
            platformSetRayTraceDescriptorSet(
                *pRenderJob->mpDescriptorSet,
                commandBuffer,
                *pRenderJob->mpPipelineState
            );
            platformSetDescriptorHeap(
                *pRenderJob->mpDescriptorHeap,
                commandBuffer
            );

            platformRayTraceCommand(
                pRenderJob,
                commandBuffer,
                mDesc.miScreenWidth,
                mDesc.miScreenHeight
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
            RenderDriver::Common::CCommandQueue* pGPUCopyCommandQueue = mpGPUCopyCommandQueue.get();

            uint32_t iJobIndex = 0;
            bool bHasSwapChainPass = false;
            uint32_t iTripleBufferIndex = miFrameIndex % 3;
            for(auto const& renderJobName : maRenderJobNames)
            {
                Render::Common::CRenderJob* pRenderJob = mapRenderJobs[renderJobName];
                RenderDriver::Common::CCommandBuffer& commandBuffer = *mapRenderJobCommandBuffers[renderJobName];
                
                if(pRenderJob->mPassType == Render::Common::PassType::SwapChain)
                {
                    bHasSwapChainPass = true;
                }

                // fill out command buffers
                commandBuffer.reset();
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
                    for(auto& copyAttachment : pRenderJob->maCopyAttachmentMapping)
                    {
                        std::string destAttachmentName = copyAttachment.second.second + "-" + copyAttachment.second.first;

                        WTFASSERT(pRenderJob->mapInputImageAttachments.count(destAttachmentName) > 0,
                            "Can't find input \"%s\"",
                            copyAttachment.first.c_str());

                        WTFASSERT(pRenderJob->mapOutputImageAttachments.count(copyAttachment.first) > 0,
                            "Can't find input \"%s\"",
                            copyAttachment.first.c_str());

                        RenderDriver::Common::CImage* pSrcImage = pRenderJob->mapInputImageAttachments[destAttachmentName];
                        RenderDriver::Common::CImage* pDestImage = pRenderJob->mapOutputImageAttachments[copyAttachment.first];

                        RenderDriver::Common::CCommandBuffer& commandBuffer = *mapRenderJobCommandBuffers["Copy Render Targets"];
                        commandBuffer.reset();
                        platformCopyImage2(
                            *pDestImage,
                            *pSrcImage,
                            commandBuffer
                        );
                    }
                }
                else if(pRenderJob->mType == Render::Common::JobType::RayTrace)
                {
                    filloutRayTraceJobCommandBuffer(
                        pRenderJob,
                        commandBuffer
                    );
                }

                commandBuffer.close();

                // execute the command buffer
                if(pRenderJob->mType == Render::Common::JobType::Graphics)
                {
                    pGraphicsCommandQueue->execCommandBuffer3(
                        commandBuffer,
                        &pRenderJob->miWaitSemaphoreValue,
                        &pRenderJob->miSignalSemaphoreValue,
                        pRenderJob->mpWaitFence,
                        pRenderJob->mpSignalFence
                    );
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
            
            // final wait for queue to finish
            mapCommandQueueFences[RenderDriver::Common::CCommandQueue::Type::Graphics]->waitCPU2(
                UINT32_MAX,
                pGraphicsCommandQueue
            );
            mapCommandQueueFences[RenderDriver::Common::CCommandQueue::Type::Compute]->waitCPU2(
                UINT32_MAX,
                pComputeCommandQueue
            );

            // reset all render job fences
            for(auto const& renderJobName : maRenderJobNames)
            {
                Render::Common::CRenderJob* pRenderJob = mapRenderJobs[renderJobName];
                pRenderJob->mpSignalFence->reset2();
                if(pRenderJob->mpWaitFence != nullptr)
                {
                    pRenderJob->mpWaitFence->reset2();
                }
            }

            ++miFrameIndex;
        }

        /*
        **
        */
        void CRenderer::initData()
        {
            // load binary data
            FILE* fp = fopen("d:\\Downloads\\Bistro_v4\\bistro2-triangles.bin", "rb");
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
            uint32_t iTriangleStartOffset = *paiMeshBuffer++;
            
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
            WTFASSERT(sizeof(VertexFormat) == iVertexSize, "vertex size not equal");
            
            // vertices
            VertexFormat* pVertices = (VertexFormat*)pExtent;
            std::vector<VertexFormat> aVertexBuffer(iNumTotalVertices);
            memcpy(aVertexBuffer.data(), pVertices, sizeof(VertexFormat) * iNumTotalVertices);
            pVertices += iNumTotalVertices;

            // indices
            uint32_t const* pIndices = (uint32_t const*)pVertices;
            std::vector<uint32_t> aiIndexBuffer(iNumTotalTriangles * 3);
            memcpy(aiIndexBuffer.data(), pIndices, sizeof(uint32_t) * iNumTotalTriangles * 3);

            uint32_t iVertexBufferSize = iNumTotalVertices * sizeof(VertexFormat);
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

#if defined(USE_RAY_TRACING)
                platformCreateAccelerationStructures(
                    aPositions,
                    aiIndexBuffer,
                    maMeshRanges,
                    miNumMeshes
                );
#endif // USE_RAY_TRACING
            }



            vec4        mPosition;
            vec4        mNormal;
            vec4        mTexCoord;

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
            // upload mesh index ranges and bounding boxes
            auto& pMeshIndexRangeBuffer = mapRenderJobs["Mesh Culling Compute"]->mapUniformBuffers["meshIndexRanges"];
            auto& pMeshBoundingBoxBuffer = mapRenderJobs["Mesh Culling Compute"]->mapUniformBuffers["meshBoundingBoxes"];
            auto& pUniformDataBuffer = mapRenderJobs["Mesh Culling Compute"]->mapUniformBuffers["uniformData"];
            copyCPUToBuffer(
                pMeshIndexRangeBuffer,
                maMeshRanges.data(),
                0,
                miNumMeshes * sizeof(uint32_t) * 2
            );
            copyCPUToBuffer(
                pMeshBoundingBoxBuffer,
                maMeshExtents.data(),
                0,
                miNumMeshes * sizeof(float4) * 2
            );
            copyCPUToBuffer(
                pUniformDataBuffer,
                &miNumMeshes,
                0,
                sizeof(uint32_t)
            );
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
            
            float4x4* pfMatrixData = (float4x4*)pfData;
            if(mRenderDriverType == RenderDriverType::Vulkan)
            {
                *pfMatrixData++ = transpose(Render::Common::gaCameras[0].getViewProjectionMatrix());
                *pfMatrixData++ = transpose(mPrevViewProjectionMatrix);
                *pfMatrixData++ = transpose(Render::Common::gaCameras[0].getViewMatrix());
                *pfMatrixData++ = transpose(Render::Common::gaCameras[0].getProjectionMatrix());

                *pfMatrixData++ = transpose(Render::Common::gaCameras[0].getViewProjectionMatrix());
                *pfMatrixData++ = transpose(mPrevViewProjectionMatrix);
            }
            else
            {
                *pfMatrixData++ = Render::Common::gaCameras[0].getViewProjectionMatrix();
                *pfMatrixData++ = mPrevViewProjectionMatrix;
                *pfMatrixData++ = Render::Common::gaCameras[0].getViewMatrix();
                *pfMatrixData++ = Render::Common::gaCameras[0].getProjectionMatrix();

                *pfMatrixData++ = Render::Common::gaCameras[0].getViewProjectionMatrix();
                *pfMatrixData++ = mPrevViewProjectionMatrix;
            }

            float4* pFloat4Data = (float4*)pfMatrixData;
            *pFloat4Data++ = float4(Render::Common::gaCameras[0].getPosition(), 1.0f);
            *pFloat4Data++ = float4(Render::Common::gaCameras[0].getLookAt(), 1.0f);
            
            *pFloat4Data++ = float4(5.0f, 5.0f, 5.0f, 1.0f);
            *pFloat4Data++ = float4(normalize(vec3(1.0f, 1.0f, 0.3f)), 1.0f);
            *pFloat4Data++ = 1.0f;

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

        }

    }   // Common

} // Render