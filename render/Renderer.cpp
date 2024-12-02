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

            // render job serializer
            mpSerializer->setRenderer(this);
            mpSerializer->setCommandQueue(mpCopyCommandQueue.get());
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
        void CRenderer::separateOutRenderJobsByType()
        {
            std::map<std::string, std::vector<WaitParentInfo>> aaWaitParents;
            buildRenderJobFenceInfo(aaWaitParents);

            maRenderJobsByType.resize(RenderDriver::Common::CCommandQueue::Type::NumTypes);

            auto& aRenderJobs = mpSerializer->getRenderJobs();
            uint32_t iRenderJobIndex = 0;
            for(auto& renderJob : aRenderJobs)
            {
                RenderJobFenceInfo renderJobFenceInfo;
                renderJobFenceInfo.mpRenderJob = &renderJob;
                renderJobFenceInfo.maWaitParentInfo = aaWaitParents[renderJob.mName];
                renderJobFenceInfo.miNumParents = static_cast<uint32_t>(renderJobFenceInfo.maWaitParentInfo.size());
                renderJobFenceInfo.miRenderJobIndex = iRenderJobIndex++;

                if(renderJob.mPassType == Render::Common::PassType::Imgui)
                {
                    maRenderJobsByType[RenderDriver::Common::CCommandQueue::Type::Graphics].push_back(renderJobFenceInfo);
                }
                else
                {
                    if(renderJob.mType == JobType::Graphics)
                    {
                        maRenderJobsByType[RenderDriver::Common::CCommandQueue::Type::Graphics].push_back(renderJobFenceInfo);
                    }
                    else if(renderJob.mType == JobType::Compute)
                    {
                        maRenderJobsByType[RenderDriver::Common::CCommandQueue::Type::Compute].push_back(renderJobFenceInfo);
                    }
                    else if(renderJob.mType == JobType::Copy)
                    {
                        maRenderJobsByType[RenderDriver::Common::CCommandQueue::Type::Copy].push_back(renderJobFenceInfo);
                    }
                    else
                    {
                        WTFASSERT(0, "no render job type (%d)", renderJob.mType);
                    }
                }
            }

            // create fences for each render jobs
            platformCreateRenderJobFences();

            // ptr to parent fences
            for(uint32_t iQueue = 0; iQueue < RenderDriver::Common::CCommandQueue::NumTypes; iQueue++)
            {
                for(uint32_t iRenderJob = 0; iRenderJob < static_cast<uint32_t>(maRenderJobsByType[iQueue].size()); iRenderJob++)
                {
                    maRenderJobsByType[iQueue][iRenderJob].maOrigWaitParentInfo = maRenderJobsByType[iQueue][iRenderJob].maWaitParentInfo;
                }
            }
            

            // child render job info 
            for(uint32_t iQueue = 0; iQueue < RenderDriver::Common::CCommandQueue::NumTypes; iQueue++)
            {
                for(uint32_t iRenderJob = 0; iRenderJob < static_cast<uint32_t>(maRenderJobsByType[iQueue].size()); iRenderJob++)
                {
                    auto const& renderQueueJob = maRenderJobsByType[iQueue][iRenderJob];
                    
                    bool bFound = false;
                    for(uint32_t iCheckQueue = 0; iCheckQueue < RenderDriver::Common::CCommandQueue::NumTypes; iCheckQueue++)
                    {
                        for(uint32_t iCheckRenderJob = 0; iCheckRenderJob < static_cast<uint32_t>(maRenderJobsByType[iCheckQueue].size()); iCheckRenderJob++)
                        {
                            if(iCheckQueue == iQueue && iCheckRenderJob == iRenderJob)
                            {
                                continue;
                            }

                            // don't bother with jobs on the same queue
                            auto& checkRenderQueueJob = maRenderJobsByType[iCheckQueue][iCheckRenderJob];
                            if(checkRenderQueueJob.mpRenderJob->mType == renderQueueJob.mpRenderJob->mType)
                            {
                                continue;
                            }
                        }

                        if(bFound)
                        {
                            break;
                        }
                    }

                    if(!bFound)
                    {
                        DEBUG_PRINTF("Didn\'t find child job on different queue for \"%s\"\n",
                            maRenderJobsByType[iQueue][iRenderJob].mpRenderJob->mName.c_str());
                    }

                }
            }
        }

        /*
        **
        */
        void CRenderer::buildRenderJobFenceInfo(
            std::map<std::string, std::vector<WaitParentInfo>>& aaWaitParents)
        {
            struct FenceInfo
            {
                RenderDriver::Common::CCommandQueue::Type               mParentCommandQueueType;
                uint32_t                                                miStartOnFenceValue;
                RenderJobInfo const* mpRenderJob;
                RenderJobInfo const* mpParentRenderJob;
                uint32_t                                                miEndOnFenceValue;
                RenderDriver::Common::CCommandQueue::Type               mOwnCommandQueueType;
            };

            struct ParentFenceInfo
            {
                RenderJobInfo const* mpRenderJob;
                RenderDriver::Common::CCommandQueue::Type               mCommandQueueType;
                uint32_t                                                miFinishOnFenceValue;
            };


            uint32_t aiFenceValues[RenderDriver::Common::CCommandQueue::Type::NumTypes] = { 0, 0, 0 };

            auto& aRenderJobs = mpSerializer->getRenderJobs();

            // parent fence info, collect the queue fence value at each step
            std::vector<ParentFenceInfo> aParentFenceInfo;
            for(auto const& renderJob : aRenderJobs)
            {
                if(renderJob.mPassType == Render::Common::PassType::Imgui)
                {
                    ParentFenceInfo fenceInfo = {};
                    aiFenceValues[RenderDriver::Common::CCommandQueue::Type::Graphics] += 1;
                    fenceInfo.mCommandQueueType = RenderDriver::Common::CCommandQueue::Type::Graphics;
                    fenceInfo.miFinishOnFenceValue = aiFenceValues[RenderDriver::Common::CCommandQueue::Type::Graphics];
                    fenceInfo.mpRenderJob = &renderJob;
                    aParentFenceInfo.push_back(fenceInfo);

                }
                else
                {
                    ParentFenceInfo fenceInfo = {};
                    if(renderJob.mType == JobType::Compute)
                    {
                        aiFenceValues[RenderDriver::Common::CCommandQueue::Type::Compute] += 1;
                        fenceInfo.mCommandQueueType = RenderDriver::Common::CCommandQueue::Type::Compute;
                        fenceInfo.miFinishOnFenceValue = aiFenceValues[RenderDriver::Common::CCommandQueue::Type::Compute];
                    }
                    else if(renderJob.mType == JobType::Graphics)
                    {
                        aiFenceValues[RenderDriver::Common::CCommandQueue::Type::Graphics] += 1;
                        fenceInfo.mCommandQueueType = RenderDriver::Common::CCommandQueue::Type::Graphics;
                        fenceInfo.miFinishOnFenceValue = aiFenceValues[RenderDriver::Common::CCommandQueue::Type::Graphics];
                    }
                    else if(renderJob.mType == JobType::Copy)
                    {
                        aiFenceValues[RenderDriver::Common::CCommandQueue::Type::Copy] += 1;
                        fenceInfo.mCommandQueueType = RenderDriver::Common::CCommandQueue::Type::Copy;
                        fenceInfo.miFinishOnFenceValue = aiFenceValues[RenderDriver::Common::CCommandQueue::Type::Copy];
                    }
                    else
                    {
                        WTFASSERT(0, "Didn\'t handle pass type %d for \"%s\"", renderJob.mPassType, renderJob.mName.c_str());
                    }

                    fenceInfo.mpRenderJob = &renderJob;
                    aParentFenceInfo.push_back(fenceInfo);
                }
            }

            // gather fence info
            std::map<std::string, std::vector<FenceInfo>> aaFenceInfo;
            for(uint32_t iRenderJob = 0; iRenderJob < static_cast<uint32_t>(aRenderJobs.size()); iRenderJob++)
            {
                auto const& renderJob = aRenderJobs[iRenderJob];

                aaFenceInfo[renderJob.mName] = std::vector<FenceInfo>();
                std::vector<FenceInfo>& aFenceInfo = aaFenceInfo[renderJob.mName];

                aaWaitParents[renderJob.mName] = std::vector<WaitParentInfo>();
                std::vector<WaitParentInfo>& aWaitParentInfo = aaWaitParents[renderJob.mName];

                uint32_t iFinishOnFenceValue = aParentFenceInfo[iRenderJob].miFinishOnFenceValue;
                RenderDriver::Common::CCommandQueue::Type ownCommandQueueType = aParentFenceInfo[iRenderJob].mCommandQueueType;

                // parent render jobs

                for(uint32_t iParent = 0; iParent < static_cast<uint32_t>(renderJob.mapParents.size()); iParent++)
                {
                    auto const* pParent = renderJob.mapParents[iParent];
                    FenceInfo fenceInfo = {};
                    WaitParentInfo waitParentFenceInfo = {};

                    bool bFoundParent = false;
                    for(uint32_t iParentFenceInfo = 0; iParentFenceInfo < iRenderJob; iParentFenceInfo++)
                    {
                        if(aParentFenceInfo[iParentFenceInfo].mpRenderJob == pParent)
                        {
                            fenceInfo.mParentCommandQueueType = aParentFenceInfo[iParentFenceInfo].mCommandQueueType;
                            fenceInfo.miStartOnFenceValue = aParentFenceInfo[iParentFenceInfo].miFinishOnFenceValue;
                            fenceInfo.mpRenderJob = &renderJob;
                            fenceInfo.mpParentRenderJob = pParent;
                            fenceInfo.miEndOnFenceValue = iFinishOnFenceValue;
                            fenceInfo.mOwnCommandQueueType = ownCommandQueueType;
                            aFenceInfo.push_back(fenceInfo);

                            bool bFound = false;
                            bool bReplaced = false;

                            uint32_t iParentFinishOnFenceValue = aParentFenceInfo[iParentFenceInfo].miFinishOnFenceValue;

                            // check for same parent queue type and replace if the fence start time for this parent is greater than the previous (start job only as the very last parent of this queue is done)
                            RenderDriver::Common::CCommandQueue::Type parentQueueType = aParentFenceInfo[iParentFenceInfo].mCommandQueueType;
                            auto iter = std::find_if(
                                aWaitParentInfo.begin(),
                                aWaitParentInfo.end(),
                                [parentQueueType, iParentFinishOnFenceValue, pParent, &bFound, &bReplaced](WaitParentInfo& waitParentInfo)
                                {
                                    bool bRet = false;
                                    if(waitParentInfo.mParentCommandQueueType == parentQueueType)
                                    {
                                        // want the latest fence to wait on
                                        if(waitParentInfo.miStartOnFenceValue < iParentFinishOnFenceValue)
                                        {
                                            waitParentInfo.mpParentRenderJob = pParent;
                                            waitParentInfo.miStartOnFenceValue = iParentFinishOnFenceValue;
                                            bReplaced = true;
                                        }

                                        bFound = true;
                                    }

                                    return bFound && bReplaced;
                                });

                            if(!bFound && !bReplaced)
                            {
                                waitParentFenceInfo.mParentCommandQueueType = aParentFenceInfo[iParentFenceInfo].mCommandQueueType;
                                waitParentFenceInfo.miStartOnFenceValue = aParentFenceInfo[iParentFenceInfo].miFinishOnFenceValue;
                                waitParentFenceInfo.mpParentRenderJob = pParent;
                                aWaitParentInfo.push_back(waitParentFenceInfo);
                            }

                            bFoundParent = true;

                            break;
                        }
                    }   // for parent fence info = 0 up to render job index

                    if(!bFoundParent)
                    {
                        if(renderJob.mType != JobType::Copy)
                        {
                            DEBUG_PRINTF("*** WARNING: didn't link to parent on fence wait info for render job \"%s\" to parent \"%s\", perhaps it was registered later than the current one inside the render job json file?\n",
                                renderJob.mName.c_str(),
                                renderJob.mapParents[iParent]->mName.c_str());
                        }
                    }

                }   // for render job parents
            }   // for render job = 0 to num render jobs

            for(auto const& keyValue : aaWaitParents)
            {
                DEBUG_PRINTF("%s\n", keyValue.first.c_str());
                auto const& aWaitParents = keyValue.second;
                uint32_t iIndex = 0;
                for(auto const& entry : aWaitParents)
                {
                    if(entry.mParentCommandQueueType == RenderDriver::Common::CCommandQueue::Type::Compute)
                    {
                        DEBUG_PRINTF("\tparent %d (compute) %s start on value: %lld\n", iIndex++, entry.mpParentRenderJob->mName.c_str(), entry.miStartOnFenceValue);
                    }
                    else if(entry.mParentCommandQueueType == RenderDriver::Common::CCommandQueue::Type::Graphics)
                    {
                        DEBUG_PRINTF("\tparent %d (graphics) %s start on value: %lld\n", iIndex++, entry.mpParentRenderJob->mName.c_str(), entry.miStartOnFenceValue);
                    }
                    else if(entry.mParentCommandQueueType == RenderDriver::Common::CCommandQueue::Type::Copy)
                    {
                        DEBUG_PRINTF("\tparent %d (copy) %s start on value: %lld\n", iIndex++, entry.mpParentRenderJob->mName.c_str(), entry.miStartOnFenceValue);
                    }
                }

                DEBUG_PRINTF("\n");
            }
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
        void CRenderer::clearUploadBuffers()
        {
            mapUploadBuffers.clear();
            mapUploadBuffers.shrink_to_fit();
        }

        /*
        **
        */
        void CRenderer::addDuplicateRenderJobs(
            std::string const& renderJobName,
            uint32_t iNumRenderJobs,
            std::vector<uint3> const& aDispatchesPerJob)
        {
            // get rid of the old passes
            RenderJobFenceInfo origRenderJobFenceInfo;
            uint32_t iQueue = UINT32_MAX, iRenderJobIndex = UINT32_MAX;
            {
                uint32_t iQueueIndex = 0;
                for(auto& aRenderJobs : maRenderJobsByType)
                {
                    for(;;)
                    {
                        auto iter = std::find_if(
                            aRenderJobs.begin(),
                            aRenderJobs.end(),
                            [renderJobName](RenderJobFenceInfo& renderJobFenceInfo)
                            {
                                return (renderJobFenceInfo.mpRenderJob->mName.find(renderJobName) == 0);
                            });
                        if(iter != aRenderJobs.end())
                        {
                            // save a copy
                            if(origRenderJobFenceInfo.mpRenderJob == nullptr)
                            {
                                origRenderJobFenceInfo = *iter;
                                iRenderJobIndex = static_cast<uint32_t>(std::distance(aRenderJobs.begin(), iter));
                            }

                            aRenderJobs.erase(iter);
                            iQueue = iQueueIndex;
                        }
                        else
                        {
                            break;
                        }
                    }

                    ++iQueueIndex;
                }
            }

            // set new fence values for subsequent jobs
            std::vector<RenderJobFenceInfo*> apDependentJobs;
            for(auto& aRenderJobs : maRenderJobsByType)
            {
                for(auto& renderJob : aRenderJobs)
                {
                    //for(auto& waitParentInfo : renderJob.maWaitParentInfo)
                    for(uint32_t iWaitParentInfo = 0; iWaitParentInfo < static_cast<uint32_t>(renderJob.maWaitParentInfo.size()); iWaitParentInfo++)
                    {
                        auto& waitParentInfo = renderJob.maWaitParentInfo[iWaitParentInfo];
                        auto const& origWaitParentInfo = renderJob.maOrigWaitParentInfo[iWaitParentInfo];

                        if(origWaitParentInfo.mParentCommandQueueType == static_cast<RenderDriver::Common::CCommandQueue::Type>(iQueue) &&
                           origWaitParentInfo.miStartOnFenceValue >= iRenderJobIndex)
                        {
                            waitParentInfo.miStartOnFenceValue = origWaitParentInfo.miStartOnFenceValue + (iNumRenderJobs - 1);
                            break;
                        }
                    }
                }
            }

            // add wait parent info for new passes
            WTFASSERT(iQueue != UINT32_MAX && iRenderJobIndex != UINT32_MAX, "didn\'t find render job \"%s\"", renderJobName.c_str());
            uint32_t iCurrFenceValue = origRenderJobFenceInfo.miRenderJobIndex;
            for(uint32_t iInsertIndex = 0; iInsertIndex < iNumRenderJobs; iInsertIndex++)
            {
                RenderJobFenceInfo toInsertJobFenceInfo = origRenderJobFenceInfo;

                // start incrementing the fence values on subsequent duplicated render jobs
                if(iInsertIndex > 0)
                {
                    WaitParentInfo waitParentInfo;
                    waitParentInfo.mpParentRenderJob = origRenderJobFenceInfo.mpRenderJob;
                    waitParentInfo.mParentCommandQueueType = static_cast<RenderDriver::Common::CCommandQueue::Type>(iQueue);
                    waitParentInfo.miStartOnFenceValue = iCurrFenceValue;
                    toInsertJobFenceInfo.miRenderJobIndex = iRenderJobIndex + iInsertIndex;
                    toInsertJobFenceInfo.miNumParents += 1;

                    toInsertJobFenceInfo.maWaitParentInfo.push_back(waitParentInfo);
                }
                
                if(aDispatchesPerJob.size() > iInsertIndex)
                {
                    toInsertJobFenceInfo.mDispatches = aDispatchesPerJob[iInsertIndex];
                }
                else
                {
                    toInsertJobFenceInfo.mDispatches = uint3(0, 0, 0);
                }

                // save this for getting all the infos for the render job
                toInsertJobFenceInfo.miOrigRenderJobIndex = iRenderJobIndex;

                maRenderJobsByType[iQueue].insert(maRenderJobsByType[iQueue].begin() + iRenderJobIndex + iInsertIndex, toInsertJobFenceInfo); 
                ++iCurrFenceValue;
            }           

#if 0
            uint32_t iQueueIndex = 0;
            for(auto const& aRenderJobs : maRenderJobsByType)
            {
                DEBUG_PRINTF("Queue %d\n", iQueueIndex);
                uint32_t iRenderJob = 0;
                for(auto const& renderJob : aRenderJobs)
                {
                    DEBUG_PRINTF("\t%d %s\n", iRenderJob, renderJob.mpRenderJob->mName.c_str());
                    for(auto const& waitParentInfo : renderJob.maWaitParentInfo)
                    {
                        DEBUG_PRINTF("\t\tparent: \"%s\" fence value = %d\n",
                            waitParentInfo.mpParentRenderJob->mName.c_str(),
                            waitParentInfo.miStartOnFenceValue);
                    }

                    ++iRenderJob;
                }

                ++iQueueIndex;
            }

            int iDebug = 1;
#endif // #if 0

#if 0
            // use the last job as parent
            for(auto& pDependentJob : apDependentJobs)
            {
                for(auto& waitParentInfo : pDependentJob->maWaitParentInfo)
                {
                    if(waitParentInfo.mpParentRenderJob->mName == renderJobName)
                    {
                        waitParentInfo.mpParentRenderJob = origRenderJobFenceInfo.mpRenderJob;
                        waitParentInfo.miStartOnFenceValue = 
                    }
                }
            }
#endif // #if 0

        }

        /*
        **
        */
        void CRenderer::execRenderJobs()
        {
        }

        /*
        **
        */
        void CRenderer::execGraphicsJob(
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex)
        {
#if 0
            //auto start = std::chrono::high_resolution_clock::now();
            Render::Common::RenderJobInfo const& renderJob = mpSerializer->getRenderJob(iRenderJob);

            if(renderJob.mPassType == Render::Common::PassType::Imgui)
            {
                platformRenderImgui(iTripleBufferIndex);
                return;
            }

            RenderDriver::Common::PipelineInfo const& pipelineInfo = mpSerializer->getPipelineInfo(iRenderJob);
            RenderDriver::Common::CPipelineState& pipelineState = *mpSerializer->getPipelineStateFromHandle(pipelineInfo.mPipelineStateHandle).get();
            RenderDriver::Common::CDescriptorSet& descriptorSet = *mpSerializer->getDescriptorSet(pipelineInfo.mDescriptorHandle).get();

            RenderDriver::Common::CCommandBuffer& commandBuffer = *mpSerializer->getCommandBuffer(renderJob.mCommandBufferHandle).get();

            PLATFORM_OBJECT_HANDLE commandAllocatorHandle = renderJob.maCommandAllocatorHandles[iTripleBufferIndex];
            RenderDriver::Common::CCommandAllocator& commandAllocator = *mpSerializer->getCommandAllocator(commandAllocatorHandle).get();

            PLATFORM_OBJECT_HANDLE descriptorHeapHandle = renderJob.maDescriptorHeapHandles[iTripleBufferIndex];
            RenderDriver::Common::CDescriptorHeap& descriptorHeap = *mpSerializer->getDescriptorHeap(descriptorHeapHandle).get();

            // wait on parent job
            RenderDriver::Common::CFence* apFences[32];
            uint32_t iNumCheckFences = 0;
            for(uint32_t iAttachment = 0; iAttachment < static_cast<uint32_t>(renderJob.maAttachmentInfo.size()); iAttachment++)
            {
                if(renderJob.maAttachmentInfo[iAttachment].mParentJobName != "" && renderJob.maAttachmentInfo[iAttachment].mParentJobName != "This")
                {
                    Render::Common::RenderJobInfo& parentRenderJob = mpSerializer->getRenderJob(renderJob.maAttachmentInfo[iAttachment].mParentJobName);
                    RenderDriver::Common::CFence& parentFence = *mpSerializer->getFence(parentRenderJob.maFenceHandles[0]).get();

                    // check already placed fence
                    bool bAlreadyPlaced = false;
                    for(uint32_t iFenceCheck = 0; iFenceCheck < iNumCheckFences; iFenceCheck++)
                    {
                        if(&parentFence == apFences[iFenceCheck])
                        {
                            bAlreadyPlaced = true;
                            break;
                        }
                    }
                    if(bAlreadyPlaced)
                    {
                        continue;
                    }

                    apFences[iNumCheckFences++] = &parentFence;


                    RenderDriver::Common::CCommandBuffer& parentCommandBuffer = *mpSerializer->getCommandBuffer(parentRenderJob.mCommandBufferHandle);
                    RenderDriver::Common::CCommandAllocator& parentCommandAllocator = *mpSerializer->getCommandAllocator(parentRenderJob.maCommandAllocatorHandles[iTripleBufferIndex]);

                    RenderDriver::Common::CCommandQueue* pCommandQueue = nullptr;
                    if(renderJob.mType == JobType::Compute)
                    {
                        pCommandQueue = mpComputeCommandQueue.get();
                    }
                    else if(renderJob.mType == JobType::Graphics)
                    {
                        pCommandQueue = mpGraphicsCommandQueue.get();
                    }
                    else if(renderJob.mType == JobType::Copy)
                    {
                        pCommandQueue = mpCopyCommandQueue.get();
                    }
                    WTFASSERT(pCommandQueue, "Didn\'t get a valid command queue 0x%lld", reinterpret_cast<uint64_t>(pCommandQueue));

                    // wait
                    // switching from graphics to compute queue
                    for(auto const& parentRenderJob : renderJob.mapParents)
                    {
                        if(parentRenderJob->mType != JobType::Compute)
                        {
                            parentFence.waitGPU(pCommandQueue);
                            break;
                        }
                    }

                    // reset command buffer
                    if(parentCommandBuffer.getState() == RenderDriver::Common::CommandBufferState::Executing)
                    {
                        parentFence.waitCPU(UINT64_MAX);
                        parentCommandAllocator.reset();
                        parentCommandBuffer.reset();

                        platformEndRenderJobDebugEventMark(parentRenderJob.mName);
                    }
                }
            }

            //auto waitElapsed = std::chrono::high_resolution_clock::now() - start;
            //uint64_t iWaitedUS = std::chrono::duration_cast<std::chrono::microseconds>(waitElapsed).count();
            //DEBUG_PRINTF("%s waited %d us\n",
            //    renderJob.mName.c_str(),
            //    iWaitedUS);

            // pipeline state and descriptor set
            platformSetPipelineState(pipelineState, commandBuffer);
            platformSetGraphicsDescriptorSet(descriptorSet, commandBuffer, pipelineState);
            if(descriptorHeap.getNativeDescriptorHeap() != nullptr)
            {
                platformSetDescriptorHeap(descriptorHeap, commandBuffer);
            }

            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = mpSerializer->getShaderResourceInfo(iRenderJob);

            // TODO: cache gpu addresss
            // set resource views
            platformSetResourceViews(
                aShaderResources,
                commandBuffer,
                descriptorHeap,
                iTripleBufferIndex,
                Render::Common::JobType::Graphics);


            // viewport and scissor
            platformSetViewportAndScissorRect(
                uint32_t(float(mDesc.miScreenWidth) * renderJob.mViewportScale.x),
                uint32_t(float(mDesc.miScreenHeight) * renderJob.mViewportScale.y),
                mDesc.mfViewportMaxDepth,
                commandBuffer);

            float afClearColor[] = { 0.0f, 0.0f, 0.3f, 0.0 };

            uint32_t iNumBarrierInfo = 0;
            RenderDriver::Common::Utils::TransitionBarrierInfo aBarrierInfo[32];

            // resource barrier and clear swap chain
            if(renderJob.mPassType == PassType::SwapChain)
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
                clearDesc.miFrameIndex = iTripleBufferIndex;
                clearDesc.mpCommandBuffer = &commandBuffer;

                mpSwapChain->clear(clearDesc);
            }

            // get the appropriate command list from gpu memory based on the render job
            // TODO: cache the memory address to avoid lookup
            std::unique_ptr<Render::Common::CDescriptorBufferManager>& pDescriptorBufferManager = Render::Common::CDescriptorBufferManager::instance();
            RenderDriver::Common::CBuffer* pIndirectDrawMeshList = static_cast<RenderDriver::Common::CBuffer*>(pDescriptorBufferManager->getReadWriteBuffer());
            uint32_t iDrawCountBufferOffset = 0;            // TODO: clean this shit up
            std::string commandListName = "";
            if(renderJob.mPassType == Render::Common::PassType::DrawMeshes)
            {
                commandListName = INDIRECT_OUTPUT_DRAW_COMMAND_LIST_BUFFER_NAME;
            }
            else if(renderJob.mPassType == Render::Common::PassType::FullTriangle ||
                renderJob.mPassType == Render::Common::PassType::SwapChain)
            {
                commandListName = renderJob.mName + ".indirect-command-buffer";
                pIndirectDrawMeshList = static_cast<RenderDriver::Common::CBuffer*>(pDescriptorBufferManager->getReadOnlyBuffer());
                iDrawCountBufferOffset = sizeof(uint32_t);
            }

            uint32_t iDrawListAddress = UINT32_MAX;
            DESCRIPTOR_MEMORY_HANDLE drawListHandle;
            pDescriptorBufferManager->getHandleAndAddress(
                iDrawListAddress,
                drawListHandle,
                commandListName);
            assert(iDrawListAddress != UINT32_MAX);

            // barriers
            if(renderJob.mPassType == Render::Common::PassType::DrawMeshes)
            {
                // add to barrier list
                aBarrierInfo[iNumBarrierInfo].mpBuffer = pIndirectDrawMeshList;
                aBarrierInfo[iNumBarrierInfo].mBefore = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;
                aBarrierInfo[iNumBarrierInfo].mAfter = RenderDriver::Common::ResourceStateFlagBits::IndirectArgument;
                ++iNumBarrierInfo;
            }

            platformSetVertexAndIndexBuffers(commandBuffer);

            
            {
                std::vector<RenderDriver::Common::CDescriptorHeap*> apRenderTargetDescriptorHeaps;
                std::vector<RenderDriver::Common::CDescriptorHeap*> apDepthStencilDescriptorHeaps;

                assert(renderJob.maOutputRenderTargetAttachments.size() == renderJob.maiOutputAttachmentMapping.size());

                // render targets to clear and barriers to transition
                uint32_t iNumRenderTargetAttachments = static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size());
                std::vector<bool> abClear(iNumRenderTargetAttachments);
                for(uint32_t i = 0; i < iNumRenderTargetAttachments; i++)
                {
                    abClear[i] = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mbClear;
                    renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mbClear;

                    PLATFORM_OBJECT_HANDLE renderTargetHandle = renderJob.maOutputRenderTargetAttachments[i];
                    RenderDriver::Common::CRenderTarget& renderTarget = *mpSerializer->getRenderTarget(renderTargetHandle).get();

                    PLATFORM_OBJECT_HANDLE colorRenderTargetHeapHandle = renderJob.maOutputRenderTargetColorHeaps[i];
                    RenderDriver::Common::CDescriptorHeap& renderTargetDescriptorHeap = *mpSerializer->getDescriptorHeap(colorRenderTargetHeapHandle).get();
                    apRenderTargetDescriptorHeaps.push_back(&renderTargetDescriptorHeap);

                    // add to barrier list
                    WTFASSERT(iNumBarrierInfo < sizeof(aBarrierInfo) / sizeof(*aBarrierInfo), "Number of barriers out of bounds %d", iNumBarrierInfo);
                    aBarrierInfo[iNumBarrierInfo].mpImage = renderTarget.getImage().get();
                    aBarrierInfo[iNumBarrierInfo].mBefore = RenderDriver::Common::ResourceStateFlagBits::Present;
                    aBarrierInfo[iNumBarrierInfo].mAfter = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;
                    ++iNumBarrierInfo;
                }

                if(iNumRenderTargetAttachments > 0)
                {
                    PLATFORM_OBJECT_HANDLE depthStencilRenderTargetHeapHandle = renderJob.maOutputRenderTargetDepthHeaps[0];
                    RenderDriver::Common::CDescriptorHeap& depthStencilDescriptorHeap = *mpSerializer->getDescriptorHeap(depthStencilRenderTargetHeapHandle).get();
                    apDepthStencilDescriptorHeaps.push_back(&depthStencilDescriptorHeap);
                }

                platformTransitionBarriers(
                    aBarrierInfo,
                    commandBuffer,
                    iNumBarrierInfo,
                    false);

                if(iNumRenderTargetAttachments > 0)
                {
                    platformSetRenderTargetAndClear(
                        commandBuffer,
                        apRenderTargetDescriptorHeaps,
                        apDepthStencilDescriptorHeaps,
                        iNumRenderTargetAttachments,
                        afClearColor,
                        abClear);
                }
            }

            // draw mesh
            platformExecuteIndirectDrawMeshInstances(
                commandBuffer,
                iDrawListAddress,
                iDrawCountBufferOffset,
                renderJob.mPassType,
                pIndirectDrawMeshList,
                renderJob.mName);

            // present state for swap chain
            if(renderJob.mPassType == PassType::SwapChain)
            {
                mpSwapChain->transitionRenderTarget(
                    commandBuffer,
                    RenderDriver::Common::ResourceStateFlagBits::RenderTarget,
                    RenderDriver::Common::ResourceStateFlagBits::Present);
            }

            // transition all the render targets resource barriers
            platformTransitionBarriers(
                aBarrierInfo,
                commandBuffer,
                iNumBarrierInfo,
                true);

            // execute commands
            commandBuffer.close();
            if(renderJob.mType == Render::Common::JobType::Compute)
            {
                mpComputeCommandQueue->execCommandBuffer(commandBuffer, *mpDevice);
            }
            else if(renderJob.mType == Render::Common::JobType::Graphics)
            {
                mpGraphicsCommandQueue->execCommandBuffer(commandBuffer, *mpDevice);
            }

            // place job fence
            RenderDriver::Common::CFence& fence = *mpSerializer->getFence(renderJob.maFenceHandles[0]).get();
            RenderDriver::Common::PlaceFenceDescriptor fenceDesc;
            fenceDesc.miFenceValue = miFenceValue++;
            fenceDesc.mType = RenderDriver::Common::FenceType::CPU;
            if(renderJob.mType == Render::Common::JobType::Compute)
            {
                fenceDesc.mpCommandQueue = mpComputeCommandQueue.get();
            }
            else if(renderJob.mType == Render::Common::JobType::Graphics)
            {
                fenceDesc.mpCommandQueue = mpGraphicsCommandQueue.get();
            }

            fence.place(static_cast<RenderDriver::Common::PlaceFenceDescriptor const&>(fenceDesc));


            //auto elapsed = std::chrono::high_resolution_clock::now() - start;
            //uint64_t iElapsedUS = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
            //DEBUG_PRINTF("%s elapsed: %d us\n", renderJob.mName.c_str(), iElapsedUS);
#endif // #if 0
        }

        /*
        **
        */
        void CRenderer::execComputeJob(
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex)
        {
#if 0
            RenderDriver::Common::PipelineInfo const& pipelineInfo = mpSerializer->getPipelineInfo(iRenderJob);
            RenderDriver::Common::CPipelineState& pipelineState = *mpSerializer->getPipelineStateFromHandle(pipelineInfo.mPipelineStateHandle).get();
            RenderDriver::Common::CDescriptorSet& descriptorSet = *mpSerializer->getDescriptorSet(pipelineInfo.mDescriptorHandle).get();

            Render::Common::RenderJobInfo const& renderJob = mpSerializer->getRenderJob(iRenderJob);
            RenderDriver::Common::CCommandBuffer& commandBuffer = *mpSerializer->getCommandBuffer(renderJob.mCommandBufferHandle).get();
            RenderDriver::Common::CCommandAllocator& commandAllocator = *mpSerializer->getCommandAllocator(renderJob.maCommandAllocatorHandles[iTripleBufferIndex]).get();

            RenderDriver::Common::CDescriptorHeap& descriptorHeap = *mpSerializer->getDescriptorHeap(renderJob.maDescriptorHeapHandles[iTripleBufferIndex]).get();

            // wait on parent job
            for(uint32_t iAttachment = 0; iAttachment < static_cast<uint32_t>(renderJob.maAttachmentInfo.size()); iAttachment++)
            {
                if(renderJob.maAttachmentInfo[iAttachment].mParentJobName != "" && renderJob.maAttachmentInfo[iAttachment].mParentJobName != "This")
                {
                    Render::Common::RenderJobInfo& parentRenderJob = mpSerializer->getRenderJob(renderJob.maAttachmentInfo[iAttachment].mParentJobName);
                    RenderDriver::Common::CFence& parentFence = *mpSerializer->getFence(parentRenderJob.maFenceHandles[0]).get();
                    RenderDriver::Common::CCommandBuffer& parentCommandBuffer = *mpSerializer->getCommandBuffer(parentRenderJob.mCommandBufferHandle);
                    RenderDriver::Common::CCommandAllocator& parentCommandAllocator = *mpSerializer->getCommandAllocator(parentRenderJob.maCommandAllocatorHandles[iTripleBufferIndex]);

                    RenderDriver::Common::CCommandQueue* pCommandQueue = nullptr;
                    if(renderJob.mType == JobType::Compute)
                    {
                        pCommandQueue = mpComputeCommandQueue.get();
                    }
                    else if(renderJob.mType == JobType::Graphics)
                    {
                        pCommandQueue = mpGraphicsCommandQueue.get();
                    }
                    else if(renderJob.mType == JobType::Copy)
                    {
                        pCommandQueue = mpCopyCommandQueue.get();
                    }
                    WTFASSERT(pCommandQueue, "Didn\'t get a valid command queue 0x%lld", reinterpret_cast<uint64_t>(pCommandQueue));

                    // switching from graphics to compute queue
                    for(auto const& parentRenderJob : renderJob.mapParents)
                    {
                        if(parentRenderJob->mType != JobType::Compute)
                        {
                            parentFence.waitGPU(pCommandQueue);
                            break;
                        }
                    }

                    if(parentCommandBuffer.getState() == RenderDriver::Common::CommandBufferState::Executing)
                    {
                        parentFence.waitCPU(UINT64_MAX);

                        parentCommandAllocator.reset();
                        parentCommandBuffer.reset();

                        platformEndRenderJobDebugEventMark(parentRenderJob.mName);
                    }
                }
            }

            // set pipeline and all the shader resource bindings
            platformSetPipelineState(pipelineState, commandBuffer);
            platformSetComputeDescriptorSet(descriptorSet, commandBuffer);
            platformSetDescriptorHeap(descriptorHeap, commandBuffer);

            // set resource views
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = mpSerializer->getShaderResourceInfo(iRenderJob);
            platformSetResourceViews(
                aShaderResources,
                commandBuffer,
                descriptorHeap,
                iTripleBufferIndex,
                Render::Common::JobType::Compute);

            //if(renderJob.mName.find("Irradiance") != std::string::npos)
            //{
            //    // NON-INDIRECT COMMAND BUFFER VERSION
            //    std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = mpSerializer->getShaderResourceInfo(iRenderJob);
            //
            //    // set resource views
            //    platformSetResourceViews(
            //        aShaderResources,
            //        commandBuffer,
            //        descriptorHeap,
            //        iTripleBufferIndex,
            //        Render::Common::JobType::Compute);
            //    
            //    commandBuffer.dispatch(1, 1, 1);
            //    commandBuffer.close();
            //}
            //else
            // execute indirect command list
            {
                std::unique_ptr<Render::Common::CDescriptorBufferManager>& pDescriptorManager = Render::Common::CDescriptorBufferManager::instance();
                RenderDriver::Common::CBuffer* pIndirectComputeList = pDescriptorManager->getReadOnlyBuffer();

                // compute command buffer list
                std::unique_ptr<Render::Common::CIndirectDrawCommandManager>& pIndirectDrawCommandManager = Render::Common::CIndirectDrawCommandManager::instance();
                uint32_t iRenderJobIndirectCommandAddress = pIndirectDrawCommandManager->getRenderJobCommandBufferAddress(renderJob.mName);
                if(iRenderJobIndirectCommandAddress == UINT32_MAX)
                {
                    pIndirectDrawCommandManager->createIndirectCommandBuffer(renderJob.mName);
                    iRenderJobIndirectCommandAddress = pIndirectDrawCommandManager->getRenderJobCommandBufferAddress(renderJob.mName);
                }
                assert(iRenderJobIndirectCommandAddress != UINT32_MAX);

                // indirect compute command
                platformExecuteIndirectCompute(
                    commandBuffer,
                    iRenderJobIndirectCommandAddress,
                    pIndirectComputeList,
                    renderJob.mName);
            }

            // execute
            mpComputeCommandQueue->execCommandBuffer(commandBuffer, *mpDevice);

            //if(renderJob.mapChildren.size() > 0)
            {
                // place job fence
                RenderDriver::Common::CFence& fence = *mpSerializer->getFence(renderJob.maFenceHandles[0]).get();
                uint64_t iFenceValue = fence.getFenceValue();
                RenderDriver::Common::PlaceFenceDescriptor fenceDesc;
                fenceDesc.miFenceValue = iFenceValue + 1;
                fenceDesc.mType = RenderDriver::Common::FenceType::CPU;
                if(renderJob.mType == Render::Common::JobType::Compute)
                {
                    fenceDesc.mpCommandQueue = mpComputeCommandQueue.get();
                }
                else if(renderJob.mType == Render::Common::JobType::Graphics)
                {
                    fenceDesc.mpCommandQueue = mpGraphicsCommandQueue.get();
                }

                fence.place(static_cast<RenderDriver::Common::PlaceFenceDescriptor const&>(fenceDesc));
            }
#endif // #if 0
        }

        /*
        **
        */
        void CRenderer::execCopyJob(
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex)
        {
        }

        /*
        **
        */
        void CRenderer::updateRenderJobData(
            Render::Common::RenderJobInfo const& renderJob)
        {
            WTFASSERT(0, "Implement me");
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

#if 0
            // exec command buffer
            mpUploadCommandBuffer->close();
            mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);

            // wait until done
            RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
            placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
            placeFenceDesc.miFenceValue = ++miCopyCommandFenceValue;
            placeFenceDesc.mType = RenderDriver::Common::FenceType::CPU;
            mpUploadFence->place(placeFenceDesc);
            mpUploadFence->waitCPU(UINT64_MAX);
#endif // #if 0

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
            Render::Common::RenderJobInfo& renderJob = mpSerializer->getRenderJob(renderJobName);
            auto const& iter = std::find_if(
                renderJob.maShaderResourceInfo.begin(),
                renderJob.maShaderResourceInfo.end(),
                [shaderResourceName](SerializeUtils::Common::ShaderResourceInfo const& shaderResourceInfo)
                {
                    return shaderResourceInfo.mName == shaderResourceName;
                }
            );

            if(iter != renderJob.maShaderResourceInfo.end())
            {
                DEBUG_PRINTF("\nupdate shader resource\n");
                DEBUG_PRINTF("\tjob: %s\n", renderJobName.c_str());
                DEBUG_PRINTF("\tshader resource: %s\n", shaderResourceName.c_str());
                DEBUG_PRINTF("\tdata size: %d\n", iDataSize);

                for(uint32_t iTripleBuffer = 0; iTripleBuffer < 3; iTripleBuffer++)
                {
                    RenderDriver::Common::CBuffer& buffer = *mpSerializer->getBuffer(iter->maHandles[iTripleBuffer]).get();
                    platformUploadResourceData(buffer, pData, iDataSize, 0);
                }
            }
            else
            {
                DEBUG_PRINTF("Can not find render job %s with resource %s\n", renderJobName.c_str(), shaderResourceName.c_str());
            }
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
        void CRenderer::copyCPUToBufferImmediate(
            RenderDriver::Common::CBuffer& buffer,
            void* pRawSrcData,
            uint64_t iDataSize,
            uint64_t iDestDataOffset)
        {
            platformUploadResourceDataImmediate(
                buffer,
                pRawSrcData,
                iDataSize,
                iDestDataOffset);
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
        void CRenderer::initializeTotalVertexAndIndexBuffer(Render::Common::InitializeVertexAndIndexBufferDescriptor const& desc)
        {
            platformCreateTotalVertexAndIndexBuffer(desc);
            platformCreateVertexIndexBufferViews(desc.miVertexDataSize, desc.miIndexDataSize);
        }

        /*
        **
        */
        void CRenderer::createOctahedronTexture(
            std::vector<float>& afData,
            uint32_t iImageSize,
            float fInc,
            bool bConvertRange)
        {
            for(float fTheta = 0.0f; fTheta <= 3.14159f; fTheta += fInc)
            {
                for(float fPhi = 0.0f; fPhi <= 2.0f * 3.14159f; fPhi += fInc)
                {
                    float3 normal;
                    normal.x = sinf(fPhi) * cosf(fTheta);
                    normal.y = sinf(fPhi) * sinf(fTheta);
                    normal.z = cosf(fPhi);

                    normal = normalize(normal);

                    float2 uv = float2(0.0f, 0.0f);
                    {
                        float fDP = dot(vec3(1.0f, 1.0f, 1.0f), vec3(fabsf(normal.x), fabsf(normal.y), fabsf(normal.z)));
                        vec3 ret = normal / fDP;

                        float fSignX = (ret.x < 0.0f) ? -1.0f : 1.0f;
                        float fSignZ = (ret.z < 0.0f) ? -1.0f : 1.0f;

                        vec2 rev;
                        rev.x = fabsf(ret.z) - 1.0f;
                        rev.y = fabsf(ret.x) - 1.0f;

                        vec2 neg;
                        neg.x = (ret.x < 0.0f) ? rev.x : -rev.x;
                        neg.y = (ret.z < 0.0f) ? rev.y : -rev.y;

                        uv.x = ret.x;
                        uv.y = ret.z;

                        if(ret.y < 0.0f)
                        {
                            uv = neg;
                        }

                        uv.x = uv.x * 0.5f + 0.5f;
                        uv.y = uv.y * 0.5f + 0.5f;
                    }

                    uint32_t iX = static_cast<uint32_t>(uv.x * iImageSize);
                    uint32_t iY = static_cast<uint32_t>(uv.y * iImageSize);

                    iX = (iX >= iImageSize) ? iImageSize - 1 : iX;
                    iY = (iY >= iImageSize) ? iImageSize - 1 : iY;

                    uint32_t iIndex = (iY * iImageSize + iX) * 3;

                    if(bConvertRange)
                    {
                        afData[iIndex] = normal.x * 0.5f + 0.5f;
                        afData[iIndex + 1] = normal.y * 0.5f + 0.5f;
                        afData[iIndex + 2] = normal.z * 0.5f + 0.5f;
                    }
                    else
                    {
                        afData[iIndex] = normal.x;
                        afData[iIndex + 1] = normal.y;
                        afData[iIndex + 2] = normal.z;
                    }

                }   // for phi
            }   // for theta
        }

        /*
        **
        */
        void CRenderer::loadBlueNoiseImageData()
        {
            WTFASSERT(0, "Implement me");

        }   // blue noise

        /*
        **
        */
        void CRenderer::setResourceBuffer(
            std::string const& shaderResourceName,
            RenderDriver::Common::CBuffer* pBuffer)
        {
            auto& aRenderJobs = mpSerializer->getRenderJobs();
            for(auto& renderJob : aRenderJobs)
            {
                for(auto& shaderResource : renderJob.maShaderResourceInfo)
                {
                    if(shaderResource.mName == shaderResourceName)
                    {
                        shaderResource.mExternalResource.mpBuffer = pBuffer;
                    }
                }
            }
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
        RenderDriver::Common::CCommandBuffer* CRenderer::filloutGraphicsJobCommandBuffer(
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex)
        {
            Render::Common::RenderJobInfo const& renderJob = mpSerializer->getRenderJob(iRenderJob);

            WTFASSERT(
                renderJob.mPassType == Render::Common::PassType::Imgui ||
                renderJob.mPassType == Render::Common::PassType::DrawMeshes ||
                renderJob.mPassType == Render::Common::PassType::FullTriangle ||
                renderJob.mPassType == Render::Common::PassType::DrawMeshClusters ||
                renderJob.mPassType == Render::Common::PassType::SwapChain,
                "Invalid pass type (%d) for graphics render job",
                renderJob.mPassType);

            if(renderJob.mPassType == Render::Common::PassType::Imgui)
            {
                //platformRenderImgui(iTripleBufferIndex);
                return nullptr;
            }

            //auto start = std::chrono::high_resolution_clock::now();

            //platformBeginRenderJobDebugEventMark(renderJob.mName);

            RenderDriver::Common::PipelineInfo const& pipelineInfo = mpSerializer->getPipelineInfo(iRenderJob);
            RenderDriver::Common::CPipelineState& pipelineState = *mpSerializer->getPipelineStateFromHandle(pipelineInfo.mPipelineStateHandle).get();
            RenderDriver::Common::CDescriptorSet& descriptorSet = *mpSerializer->getDescriptorSet(pipelineInfo.mDescriptorHandle).get();

            //RenderDriver::Common::CCommandBuffer& commandBuffer = *mpSerializer->getCommandBuffer(renderJob.mCommandBufferHandle).get();
            //PLATFORM_OBJECT_HANDLE commandAllocatorHandle = renderJob.maCommandAllocatorHandles[iTripleBufferIndex];
            //RenderDriver::Common::CCommandAllocator& commandAllocator = *mpSerializer->getCommandAllocator(commandAllocatorHandle).get();
            
            RenderDriver::Common::CCommandBuffer& commandBuffer = *(mapQueueCommandBuffers[static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Graphics)].get());
            RenderDriver::Common::CCommandAllocator& commandAllocator = *(mapQueueCommandAllocators[static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Graphics)].get());

            PLATFORM_OBJECT_HANDLE descriptorHeapHandle = renderJob.maDescriptorHeapHandles[iTripleBufferIndex];

            // resource barrier and clear swap chain
            if(renderJob.mPassType == PassType::SwapChain)
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

            // pipeline state and descriptor set
            platformSetGraphicsDescriptorSet(descriptorSet, commandBuffer, pipelineState);
            if(descriptorHeapHandle)
            {
                RenderDriver::Common::CDescriptorHeap& descriptorHeap = *mpSerializer->getDescriptorHeap(descriptorHeapHandle).get();
                platformSetDescriptorHeap(descriptorHeap, commandBuffer);

                std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = mpSerializer->getShaderResourceInfo(iRenderJob);

                // TODO: cache gpu addresss
                // set resource views
                platformSetResourceViews(
                    aShaderResources,
                    commandBuffer,
                    descriptorHeap,
                    iTripleBufferIndex,
                    Render::Common::JobType::Graphics);
            }

            // viewport and scissor
            platformSetViewportAndScissorRect(
                uint32_t(float(mDesc.miScreenWidth) * renderJob.mViewportScale.x),
                uint32_t(float(mDesc.miScreenHeight) * renderJob.mViewportScale.y),
                mDesc.mfViewportMaxDepth,
                commandBuffer);

            float afClearColor[] = { 0.0f, 0.0f, 0.3f, 0.0f };

            uint32_t iNumBarrierInfo = 0;
            RenderDriver::Common::Utils::TransitionBarrierInfo aBarrierInfo[32];

            std::string commandListName = "";
            if(renderJob.mPassType == Render::Common::PassType::DrawMeshes)
            {
                WTFASSERT(0, "Implement me");
            }
            else if(renderJob.mPassType == Render::Common::PassType::FullTriangle ||
                renderJob.mPassType == Render::Common::PassType::SwapChain)
            {
                WTFASSERT(0, "Implement me");
            }
            else if(renderJob.mPassType == Render::Common::PassType::DrawMeshClusters)
            {
                WTFASSERT(0, "Implement me");
            }

            if(renderJob.mPassType != PassType::DrawMeshes &&
                renderJob.mPassType != PassType::FullTriangle &&
                renderJob.mPassType != PassType::DrawMeshClusters)
            {
                platformSetVertexAndIndexBuffers(commandBuffer);
            }

            {
                std::vector<RenderDriver::Common::CDescriptorHeap*> apRenderTargetDescriptorHeaps;
                std::vector<RenderDriver::Common::CDescriptorHeap*> apDepthStencilDescriptorHeaps;

                assert(renderJob.maOutputRenderTargetAttachments.size() == renderJob.maiOutputAttachmentMapping.size());

                // render targets to clear and barriers to transition
                uint32_t iNumRenderTargetAttachments = static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size());
                std::vector<bool> abClear(iNumRenderTargetAttachments);
                std::vector<std::vector<float>> aafClearColors(iNumRenderTargetAttachments);
                for(uint32_t i = 0; i < iNumRenderTargetAttachments; i++)
                {
                    aafClearColors[i].resize(4);
                    aafClearColors[i][0] = 0.0f; aafClearColors[i][1] = 0.0f; aafClearColors[i][2] = 0.3f; aafClearColors[i][3] = 0.0f;

                    if(renderJob.maiOutputAttachmentMapping.size() > 0 && 
                        renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureIn &&
                        renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureOut && 
                        renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureInOut)
                    {
                        continue;
                    }

                    if(i < renderJob.maiOutputAttachmentMapping.size())
                    {
                        uint32_t iOutputAttachmentIndex = renderJob.maiOutputAttachmentMapping[i];
                        abClear[i] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mbClear;
                        
                        // clear color for the attachments
                        if(renderJob.maAttachmentInfo[iOutputAttachmentIndex].mbHasClearColor)
                        {
                            aafClearColors[i][0] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mafClearColor[0];
                            aafClearColors[i][1] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mafClearColor[1];
                            aafClearColors[i][2] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mafClearColor[2];
                            aafClearColors[i][3] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mafClearColor[3];
                        }
                    }

                    PLATFORM_OBJECT_HANDLE renderTargetHandle = renderJob.maOutputRenderTargetAttachments[i];
                    RenderDriver::Common::CRenderTarget& renderTarget = *mpSerializer->getRenderTarget(renderTargetHandle).get();
                    
                    // heap for setting render target and clear
                    PLATFORM_OBJECT_HANDLE colorRenderTargetHeapHandle = renderJob.maOutputRenderTargetColorHeaps[i];
                    RenderDriver::Common::CDescriptorHeap& renderTargetDescriptorHeap = *mpSerializer->getDescriptorHeap(colorRenderTargetHeapHandle).get();
                    apRenderTargetDescriptorHeaps.push_back(&renderTargetDescriptorHeap);

                    // add to barrier list
                    WTFASSERT(iNumBarrierInfo < sizeof(aBarrierInfo) / sizeof(*aBarrierInfo), "Number of barriers out of bounds %d", iNumBarrierInfo);
                    aBarrierInfo[iNumBarrierInfo].mpImage = renderTarget.getImage().get();
                    aBarrierInfo[iNumBarrierInfo].mBefore = RenderDriver::Common::ResourceStateFlagBits::Present;
                    aBarrierInfo[iNumBarrierInfo].mAfter = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;
                    ++iNumBarrierInfo;
                }

                // transition to pixel shader resource for attachment in swap chain pass
                if(renderJob.mPassType == PassType::SwapChain)
                {
                    uint32_t iNumRenderTargetAttachments = static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size());
                    for(uint32_t i = 0; i < iNumRenderTargetAttachments; i++)
                    {
                        if(renderJob.maiOutputAttachmentMapping.size() > 0 &&
                            renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureIn &&
                            renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureOut)
                        {
                            continue;
                        }

                        PLATFORM_OBJECT_HANDLE renderTargetHandle = renderJob.maInputRenderTargetAttachments[i];
                        RenderDriver::Common::CRenderTarget* pRenderTarget = mpSerializer->getRenderTarget(renderTargetHandle).get();

                        // add to barrier list
                        if(pRenderTarget)
                        {
                            WTFASSERT(iNumBarrierInfo < sizeof(aBarrierInfo) / sizeof(*aBarrierInfo), "Number of barriers out of bounds %d", iNumBarrierInfo);
                            aBarrierInfo[iNumBarrierInfo].mpImage = pRenderTarget->getImage().get();
                            aBarrierInfo[iNumBarrierInfo].mBefore = RenderDriver::Common::ResourceStateFlagBits::Present;
                            aBarrierInfo[iNumBarrierInfo].mAfter = RenderDriver::Common::ResourceStateFlagBits::PixelShaderResource;
                            ++iNumBarrierInfo;
                        }
                    }
                }

                if(iNumRenderTargetAttachments > 0)
                {
                    PLATFORM_OBJECT_HANDLE depthStencilRenderTargetHeapHandle = renderJob.maOutputRenderTargetDepthHeaps[0];
                    if(depthStencilRenderTargetHeapHandle > 0)
                    {
                        RenderDriver::Common::CDescriptorHeap& depthStencilDescriptorHeap = *mpSerializer->getDescriptorHeap(depthStencilRenderTargetHeapHandle).get();
                        apDepthStencilDescriptorHeaps.push_back(&depthStencilDescriptorHeap);
                    }
                }

                platformTransitionBarriers(
                    aBarrierInfo,
                    commandBuffer,
                    iNumBarrierInfo,
                    false);

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
            }

            commandBuffer.setPipelineState(pipelineState, *mpDevice);

            // draw mesh, indirect draw meshes don't work for triangle passes for some reason
            
            RenderDriver::Common::Utils::TransitionBarrierInfo passBarrierInfo;
            if(renderJob.mPassType == Render::Common::PassType::DrawMeshes ||
                renderJob.mPassType == Render::Common::PassType::FullTriangle ||
                renderJob.mPassType == Render::Common::PassType::SwapChain)
            {
                WTFASSERT(0, "Implement me");
            }
            else if(renderJob.mPassType == Render::Common::PassType::DrawMeshClusters)
            {
                CMeshClusterManager::instance()->execIndirectDrawCommands(
                    commandBuffer,
                    passBarrierInfo,
                    renderJob);
            }

            for(uint32_t iBarrier = 0; iBarrier < iNumBarrierInfo; iBarrier++)
            {
                auto& barrierInfo = aBarrierInfo[iBarrier];

                if(barrierInfo.mAfter == RenderDriver::Common::ResourceStateFlagBits::RenderTarget)
                {
                    barrierInfo.mBefore = barrierInfo.mAfter;
                    barrierInfo.mAfter = RenderDriver::Common::ResourceStateFlagBits::PixelShaderResource;
                }
                else
                {
                    auto before = barrierInfo.mBefore;
                    barrierInfo.mBefore = barrierInfo.mAfter;
                    barrierInfo.mAfter = before;
                }
            }

            if(passBarrierInfo.mpBuffer != nullptr || passBarrierInfo.mpImage != nullptr)
            {
                aBarrierInfo[iNumBarrierInfo] = passBarrierInfo;
                ++iNumBarrierInfo;
            }

            // present state for swap chain
            if(renderJob.mPassType == PassType::SwapChain)
            {
                uint32_t iCurrBackBufferIndex = mpSwapChain->getCurrentBackBufferIndex();
                aBarrierInfo[iNumBarrierInfo].mBefore = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;
                aBarrierInfo[iNumBarrierInfo].mAfter = RenderDriver::Common::ResourceStateFlagBits::Present;
                aBarrierInfo[iNumBarrierInfo].mpImage = mpSwapChain->getColorRenderTarget(iCurrBackBufferIndex);
                ++iNumBarrierInfo;

            }

            // transition all the render targets resource barriers
            platformTransitionBarriers(
                aBarrierInfo,
                commandBuffer,
                iNumBarrierInfo,
                false);

            return &commandBuffer;
        }

        /*
        **
        */
        void CRenderer::filloutGraphicsJobCommandBuffer2(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer,
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex)
        {
            Render::Common::RenderJobInfo const& renderJob = mpSerializer->getRenderJob(iRenderJob);

            platformBeginDebugMarker2(
                renderJob.mName.c_str(),
                pCommandBuffer);

            WTFASSERT(
                renderJob.mPassType == Render::Common::PassType::Imgui ||
                renderJob.mPassType == Render::Common::PassType::DrawMeshes ||
                renderJob.mPassType == Render::Common::PassType::FullTriangle ||
                renderJob.mPassType == Render::Common::PassType::DrawMeshClusters ||
                renderJob.mPassType == Render::Common::PassType::SwapChain,
                "Invalid pass type (%d) for graphics render job",
                renderJob.mPassType);

            if(renderJob.mPassType == Render::Common::PassType::Imgui)
            {
                return;
            }

            RenderDriver::Common::PipelineInfo const& pipelineInfo = mpSerializer->getPipelineInfo(iRenderJob);
            RenderDriver::Common::CPipelineState& pipelineState = *mpSerializer->getPipelineStateFromHandle(pipelineInfo.mPipelineStateHandle).get();
            RenderDriver::Common::CDescriptorSet& descriptorSet = *mpSerializer->getDescriptorSet(pipelineInfo.mDescriptorHandle).get();

            RenderDriver::Common::CCommandBuffer& commandBuffer = *pCommandBuffer;

            PLATFORM_OBJECT_HANDLE descriptorHeapHandle = renderJob.maDescriptorHeapHandles[iTripleBufferIndex];

            // resource barrier and clear swap chain
            if(renderJob.mPassType == PassType::SwapChain)
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

            uint32_t iNumRenderTargetAttachments = static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size());

            // barriers
            uint32_t iNumBarrierInfo = 0;
            RenderDriver::Common::Utils::TransitionBarrierInfo aBarrierInfo[32];
            for(uint32_t i = 0; i < iNumRenderTargetAttachments; i++)
            {
                PLATFORM_OBJECT_HANDLE renderTargetHandle = renderJob.maOutputRenderTargetAttachments[i];
                RenderDriver::Common::CRenderTarget* pRenderTarget = mpSerializer->getRenderTarget(renderTargetHandle).get();
                if(pRenderTarget)
                {
                    // add to barrier list
                    WTFASSERT(iNumBarrierInfo < sizeof(aBarrierInfo) / sizeof(*aBarrierInfo), "Number of barriers out of bounds %d", iNumBarrierInfo);
                    aBarrierInfo[iNumBarrierInfo].mpImage = pRenderTarget->getImage().get();
                    aBarrierInfo[iNumBarrierInfo].mBefore = RenderDriver::Common::ResourceStateFlagBits::Present;
                    aBarrierInfo[iNumBarrierInfo].mAfter = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;
                    ++iNumBarrierInfo;
                }
            }

            // transition to pixel shader resource for attachment in swap chain pass
            if(renderJob.mPassType == PassType::SwapChain)
            {
                uint32_t iNumRenderTargetAttachments = static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size());
                for(uint32_t i = 0; i < iNumRenderTargetAttachments; i++)
                {
                    if(renderJob.maiOutputAttachmentMapping.size() > 0 &&
                        renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureIn &&
                        renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureOut)
                    {
                        continue;
                    }

                    PLATFORM_OBJECT_HANDLE renderTargetHandle = renderJob.maInputRenderTargetAttachments[i];
                    RenderDriver::Common::CRenderTarget* pRenderTarget = mpSerializer->getRenderTarget(renderTargetHandle).get();

                    // add to barrier list
                    if(pRenderTarget)
                    {
                        WTFASSERT(iNumBarrierInfo < sizeof(aBarrierInfo) / sizeof(*aBarrierInfo), "Number of barriers out of bounds %d", iNumBarrierInfo);
                        aBarrierInfo[iNumBarrierInfo].mpImage = pRenderTarget->getImage().get();
                        aBarrierInfo[iNumBarrierInfo].mBefore = RenderDriver::Common::ResourceStateFlagBits::Present;
                        aBarrierInfo[iNumBarrierInfo].mAfter = RenderDriver::Common::ResourceStateFlagBits::PixelShaderResource;
                        ++iNumBarrierInfo;
                    }
                }
            }

            platformTransitionBarriers(
                aBarrierInfo,
                commandBuffer,
                iNumBarrierInfo,
                false,
                (void*)&renderJob);

            // begin render pass
            RenderPassDescriptor renderPassDesc = {};
            renderPassDesc.mpCommandBuffer = &commandBuffer;
            renderPassDesc.mpPipelineState = &pipelineState;
            renderPassDesc.miOffsetX = renderPassDesc.miOffsetY = 0;
            renderPassDesc.miOutputWidth = uint32_t(float(mDesc.miScreenWidth) * renderJob.mViewportScale.x);
            renderPassDesc.miOutputHeight = uint32_t(float(mDesc.miScreenHeight) * renderJob.mViewportScale.y);
            renderPassDesc.mpRenderJobInfo = &renderJob;
            renderPassDesc.miSwapChainFrameBufferindex = mpSwapChain->getCurrentBackBufferIndex();
            platformBeginRenderPass(renderPassDesc);

            // pipeline state and descriptor set
            platformSetGraphicsDescriptorSet(descriptorSet, commandBuffer, pipelineState);
            if(descriptorHeapHandle)
            {
                RenderDriver::Common::CDescriptorHeap& descriptorHeap = *mpSerializer->getDescriptorHeap(descriptorHeapHandle).get();
                platformSetDescriptorHeap(descriptorHeap, commandBuffer);

                std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = mpSerializer->getShaderResourceInfo(iRenderJob);

                // TODO: cache gpu addresss
                // set resource views
                platformSetResourceViews(
                    aShaderResources,
                    commandBuffer,
                    descriptorHeap,
                    iTripleBufferIndex,
                    Render::Common::JobType::Graphics);
            }

            // viewport and scissor
            platformSetViewportAndScissorRect(
                uint32_t(float(mDesc.miScreenWidth) * renderJob.mViewportScale.x),
                uint32_t(float(mDesc.miScreenHeight) * renderJob.mViewportScale.y),
                mDesc.mfViewportMaxDepth,
                commandBuffer);

            float afClearColor[] = { 0.0f, 0.0f, 0.3f, 0.0f };

            
            if(renderJob.mPassType == Render::Common::PassType::DrawMeshes)
            {
                WTFASSERT(0, "Implement me");
            }
            else if(renderJob.mPassType == Render::Common::PassType::FullTriangle ||
                renderJob.mPassType == Render::Common::PassType::SwapChain)
            {
                WTFASSERT(0, "Implement me");
            }
            else if(renderJob.mPassType == Render::Common::PassType::DrawMeshClusters)
            {
                WTFASSERT(0, "Implement me");
            }

            if(renderJob.mPassType != PassType::DrawMeshes &&
                renderJob.mPassType != PassType::FullTriangle &&
                renderJob.mPassType != PassType::DrawMeshClusters)
            {
                platformSetVertexAndIndexBuffers(commandBuffer);
            }

            // root constants are set at the end of the descriptor set
            if(renderJob.miNumRootConstants)
            {
                platformSetGraphicsRoot32BitConstants(
                    commandBuffer,
                    renderJob.mafRootConstants,
                    renderJob.miNumRootConstants,
                    static_cast<uint32_t>(renderJob.maShaderResourceInfo.size()),
                    0);
            }

            {
                std::vector<RenderDriver::Common::CDescriptorHeap*> apRenderTargetDescriptorHeaps;
                std::vector<RenderDriver::Common::CDescriptorHeap*> apDepthStencilDescriptorHeaps;

                assert(renderJob.maOutputRenderTargetAttachments.size() == renderJob.maiOutputAttachmentMapping.size());

                // render targets to clear and barriers to transition
                std::vector<bool> abClear(iNumRenderTargetAttachments);
                std::vector<std::vector<float>> aafClearColors(iNumRenderTargetAttachments);
                for(uint32_t i = 0; i < iNumRenderTargetAttachments; i++)
                {
                    aafClearColors[i].resize(4);
                    aafClearColors[i][0] = 0.0f; aafClearColors[i][1] = 0.0f; aafClearColors[i][2] = 0.3f; aafClearColors[i][3] = 0.0f;

                    if(renderJob.maiOutputAttachmentMapping.size() > 0 &&
                        renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureIn &&
                        renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureOut &&
                        renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[i]].mType != AttachmentType::TextureInOut)
                    {
                        continue;
                    }

                    if(i < renderJob.maiOutputAttachmentMapping.size())
                    {
                        uint32_t iOutputAttachmentIndex = renderJob.maiOutputAttachmentMapping[i];
                        abClear[i] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mbClear;

                        // clear color for the attachments
                        if(renderJob.maAttachmentInfo[iOutputAttachmentIndex].mbHasClearColor)
                        {
                            aafClearColors[i][0] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mafClearColor[0];
                            aafClearColors[i][1] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mafClearColor[1];
                            aafClearColors[i][2] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mafClearColor[2];
                            aafClearColors[i][3] = renderJob.maAttachmentInfo[iOutputAttachmentIndex].mafClearColor[3];
                        }
                    }

                    PLATFORM_OBJECT_HANDLE renderTargetHandle = renderJob.maOutputRenderTargetAttachments[i];
                    RenderDriver::Common::CRenderTarget& renderTarget = *mpSerializer->getRenderTarget(renderTargetHandle).get();

                    // heap for setting render target and clear
                    PLATFORM_OBJECT_HANDLE colorRenderTargetHeapHandle = renderJob.maOutputRenderTargetColorHeaps[i];
                    RenderDriver::Common::CDescriptorHeap& renderTargetDescriptorHeap = *mpSerializer->getDescriptorHeap(colorRenderTargetHeapHandle).get();
                    apRenderTargetDescriptorHeaps.push_back(&renderTargetDescriptorHeap);
                }

                if(iNumRenderTargetAttachments > 0)
                {
                    PLATFORM_OBJECT_HANDLE depthStencilRenderTargetHeapHandle = renderJob.maOutputRenderTargetDepthHeaps[0];
                    if(depthStencilRenderTargetHeapHandle > 0)
                    {
                        RenderDriver::Common::CDescriptorHeap& depthStencilDescriptorHeap = *mpSerializer->getDescriptorHeap(depthStencilRenderTargetHeapHandle).get();
                        apDepthStencilDescriptorHeaps.push_back(&depthStencilDescriptorHeap);
                    }
                }

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
            }

            commandBuffer.setPipelineState(pipelineState, *mpDevice);

            // draw mesh, indirect draw meshes don't work for triangle passes for some reason

            RenderDriver::Common::Utils::TransitionBarrierInfo passBarrierInfo;
            if(renderJob.mpfnDrawOverride != nullptr)
            {
                DrawOverrideInfo drawOverrideInfo;
                drawOverrideInfo.mpUserData = renderJob.mpDrawFuncUserData;
                drawOverrideInfo.mpRenderJob = (void*)&renderJob;

                renderJob.mpfnDrawOverride(commandBuffer, &drawOverrideInfo);
            }
            else if(renderJob.mPassType == Render::Common::PassType::DrawMeshes ||
                renderJob.mPassType == Render::Common::PassType::FullTriangle ||
                renderJob.mPassType == Render::Common::PassType::SwapChain)
            {
                WTFASSERT(0, "Implement me");
            }
            else if(renderJob.mPassType == Render::Common::PassType::DrawMeshClusters)
            {
                CMeshClusterManager::instance()->execIndirectDrawCommands(
                    commandBuffer,
                    passBarrierInfo,
                    renderJob);
            }
            
            platformEndRenderPass(renderPassDesc);

            for(uint32_t iBarrier = 0; iBarrier < iNumBarrierInfo; iBarrier++)
            {
                auto& barrierInfo = aBarrierInfo[iBarrier];

                if(barrierInfo.mAfter == RenderDriver::Common::ResourceStateFlagBits::RenderTarget)
                {
                    barrierInfo.mBefore = barrierInfo.mAfter;
                    barrierInfo.mAfter = RenderDriver::Common::ResourceStateFlagBits::PixelShaderResource;
                }
                else
                {
                    auto before = barrierInfo.mBefore;
                    barrierInfo.mBefore = barrierInfo.mAfter;
                    barrierInfo.mAfter = before;
                }
            }

            if(passBarrierInfo.mpBuffer != nullptr || passBarrierInfo.mpImage != nullptr)
            {
                aBarrierInfo[iNumBarrierInfo] = passBarrierInfo;
                ++iNumBarrierInfo;
            }

            // present state for swap chain
            if(renderJob.mPassType == PassType::SwapChain)
            {
                uint32_t iCurrBackBufferIndex = mpSwapChain->getCurrentBackBufferIndex();
                aBarrierInfo[iNumBarrierInfo].mBefore = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;
                aBarrierInfo[iNumBarrierInfo].mAfter = RenderDriver::Common::ResourceStateFlagBits::Present;
                aBarrierInfo[iNumBarrierInfo].mpImage = mpSwapChain->getColorRenderTarget(iCurrBackBufferIndex);
                ++iNumBarrierInfo;

            }

            // transition all the render targets resource barriers
            platformTransitionBarriers(
                aBarrierInfo,
                commandBuffer,
                iNumBarrierInfo,
                false,
                (void *)&renderJob);

            platformEndDebugMarker2(&commandBuffer);
        }

        /*
        **
        */
        RenderDriver::Common::CCommandBuffer* CRenderer::filloutComputeJobCommandBuffer(
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex,
            uint3 const& dispatches)
        {
            Render::Common::RenderJobInfo const& renderJob = mpSerializer->getRenderJob(iRenderJob);
            
            RenderDriver::Common::CCommandBuffer& commandBuffer = *mapQueueCommandBuffers[static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Compute)].get();
            RenderDriver::Common::CCommandAllocator& commandAllocator = *mapQueueCommandAllocators[static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Compute)].get();

            uint32_t iNumDispatchX = renderJob.miNumDispatchX;
            uint32_t iNumDispatchY = renderJob.miNumDispatchY;
            uint32_t iNumDispatchZ = renderJob.miNumDispatchZ;
            if(dispatches.x > 0 && dispatches.y > 0 && dispatches.z > 0)
            {
                iNumDispatchX = dispatches.x;
                iNumDispatchY = dispatches.y;
                iNumDispatchZ = dispatches.z;
            }
            
            if(iNumDispatchX <= 0 && iNumDispatchY <= 0 && iNumDispatchZ <= 0)
            {
                commandBuffer.close();
                return nullptr;
            }

            //platformBeginRenderJobDebugEventMark(renderJob.mName);

            RenderDriver::Common::PipelineInfo const& pipelineInfo = mpSerializer->getPipelineInfo(iRenderJob);
            RenderDriver::Common::CPipelineState& pipelineState = *mpSerializer->getPipelineStateFromHandle(pipelineInfo.mPipelineStateHandle).get();
            RenderDriver::Common::CDescriptorSet& descriptorSet = *mpSerializer->getDescriptorSet(pipelineInfo.mDescriptorHandle).get();

            RenderDriver::Common::CDescriptorHeap& descriptorHeap = *mpSerializer->getDescriptorHeap(renderJob.maDescriptorHeapHandles[iTripleBufferIndex]).get();

            // set pipeline and all the shader resource bindings
            platformSetComputeDescriptorSet(descriptorSet, commandBuffer, pipelineState);
            platformSetDescriptorHeap(descriptorHeap, commandBuffer);

            // set resource views
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = mpSerializer->getShaderResourceInfo(iRenderJob);
            platformSetResourceViews(
                aShaderResources,
                commandBuffer,
                descriptorHeap,
                iTripleBufferIndex,
                Render::Common::JobType::Compute);

            uint32_t iIndex = 0;
            std::vector<RenderDriver::Common::Utils::TransitionBarrierInfo> aBarriers(aShaderResources.size());
            for(auto const& shaderResource : aShaderResources)
            {
                if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT)
                {
                    aBarriers[iIndex].mpBuffer = shaderResource.mExternalResource.mpBuffer;
                    aBarriers[iIndex].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iIndex].mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;

                    WTFASSERT(aBarriers[iIndex].mpBuffer, "no buffer");
                }
                else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT)
                {
                    aBarriers[iIndex].mpImage = shaderResource.mExternalResource.mpImage;
                    aBarriers[iIndex].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iIndex].mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;

                    WTFASSERT(aBarriers[iIndex].mpImage, "no image");
                }
                else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN)
                {
                    if(shaderResource.mExternalResource.mpBuffer == nullptr)
                    {
                        continue;
                    }

                    aBarriers[iIndex].mpBuffer = shaderResource.mExternalResource.mpBuffer;
                    aBarriers[iIndex].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iIndex].mAfter = RenderDriver::Common::ResourceStateFlagBits::NonPixelShaderResource;

                    WTFASSERT(aBarriers[iIndex].mpBuffer, "no buffer");
                }
                else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
                {
                    aBarriers[iIndex].mpImage = shaderResource.mExternalResource.mpImage;
                    aBarriers[iIndex].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iIndex].mAfter = RenderDriver::Common::ResourceStateFlagBits::NonPixelShaderResource;

                    WTFASSERT(aBarriers[iIndex].mpImage, "no image");
                }

                ++iIndex;
            }

            platformTransitionBarriers(
                aBarriers.data(),
                commandBuffer,
                iIndex,
                false,
                (void *)&renderJob);

            commandBuffer.setPipelineState(pipelineState, *mpDevice);

            commandBuffer.dispatch(iNumDispatchX, iNumDispatchY, iNumDispatchZ);

            platformTransitionBarriers(
                aBarriers.data(),
                commandBuffer,
                iIndex,
                true,
                (void *)&renderJob);

            //commandBuffer.close();

            return &commandBuffer;
        }

        /*
        **
        */
        void CRenderer::filloutComputeJobCommandBuffer2(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer,
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex,
            uint3 const& dispatches)
        {
            Render::Common::RenderJobInfo const& renderJob = mpSerializer->getRenderJob(iRenderJob);
            RenderDriver::Common::CCommandBuffer& commandBuffer = *pCommandBuffer;
            
            platformBeginDebugMarker2(
                renderJob.mName.c_str(),
                pCommandBuffer);

            uint32_t iNumDispatchX = renderJob.miNumDispatchX;
            uint32_t iNumDispatchY = renderJob.miNumDispatchY;
            uint32_t iNumDispatchZ = renderJob.miNumDispatchZ;
            if(dispatches.x > 0 && dispatches.y > 0 && dispatches.z > 0)
            {
                iNumDispatchX = dispatches.x;
                iNumDispatchY = dispatches.y;
                iNumDispatchZ = dispatches.z;
            }

            if(iNumDispatchX <= 0 && iNumDispatchY <= 0 && iNumDispatchZ <= 0)
            {
                commandBuffer.close();
                return;
            }

            RenderDriver::Common::PipelineInfo const& pipelineInfo = mpSerializer->getPipelineInfo(iRenderJob);
            RenderDriver::Common::CPipelineState& pipelineState = *mpSerializer->getPipelineStateFromHandle(pipelineInfo.mPipelineStateHandle).get();
            RenderDriver::Common::CDescriptorSet& descriptorSet = *mpSerializer->getDescriptorSet(pipelineInfo.mDescriptorHandle).get();

            RenderDriver::Common::CDescriptorHeap& descriptorHeap = *mpSerializer->getDescriptorHeap(renderJob.maDescriptorHeapHandles[iTripleBufferIndex]).get();

            // set pipeline and all the shader resource bindings
            platformSetComputeDescriptorSet(descriptorSet, commandBuffer, pipelineState);
            platformSetDescriptorHeap(descriptorHeap, commandBuffer);

            // set resource views
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = mpSerializer->getShaderResourceInfo(iRenderJob);
            platformSetResourceViews(
                aShaderResources,
                commandBuffer,
                descriptorHeap,
                iTripleBufferIndex,
                Render::Common::JobType::Compute);

            // root constants are set at the end of the descriptor set
            if(renderJob.miNumRootConstants)
            {
                auto const& pipelineInfo = mpSerializer->getPipelineInfo(iRenderJob);
                RenderDriver::Common::CPipelineState* pPipelineState = mpSerializer->getPipelineStateFromHandle(pipelineInfo.mPipelineStateHandle).get();
                platformSetComputeRoot32BitConstants(
                    commandBuffer,
                    *pPipelineState,
                    renderJob.mafRootConstants,
                    renderJob.miNumRootConstants,
                    static_cast<uint32_t>(renderJob.maShaderResourceInfo.size()),
                    0);
            }
            
            uint32_t iIndex = 0;
            uint32_t iNumTransitions = 0;
            std::vector<RenderDriver::Common::Utils::TransitionBarrierInfo> aBarriers(aShaderResources.size());
            for(auto const& shaderResource : aShaderResources)
            {
                if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT)
                {
                    aBarriers[iNumTransitions].mpBuffer = shaderResource.mExternalResource.mpBuffer;
                    aBarriers[iNumTransitions].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumTransitions].mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;

                    WTFASSERT(aBarriers[iNumTransitions].mpBuffer, "no buffer");

                    ++iNumTransitions;
                }
                else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT)
                {
                    aBarriers[iNumTransitions].mpImage = shaderResource.mExternalResource.mpImage;
                    aBarriers[iNumTransitions].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumTransitions].mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;

                    WTFASSERT(aBarriers[iNumTransitions].mpImage, "no image");

                    ++iNumTransitions;
                }
                else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN)
                {
                    if(shaderResource.mExternalResource.mpBuffer == nullptr)
                    {
                        continue;
                    }
                
                    aBarriers[iIndex].mpBuffer = shaderResource.mExternalResource.mpBuffer;
                    aBarriers[iIndex].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iIndex].mAfter = RenderDriver::Common::ResourceStateFlagBits::NonPixelShaderResource;
                
                    WTFASSERT(aBarriers[iIndex].mpBuffer, "no buffer");
                }
                else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
                {
                    if(shaderResource.mExternalResource.mpImage == nullptr)
                    {
                        continue;
                    }
                
                    aBarriers[iIndex].mpImage = shaderResource.mExternalResource.mpImage;
                    aBarriers[iIndex].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iIndex].mAfter = RenderDriver::Common::ResourceStateFlagBits::PixelShaderResource;
                
                    WTFASSERT(aBarriers[iIndex].mpImage, "no image");
                }
                else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT)
                {
                    if(aBarriers[iNumTransitions].mpBuffer == nullptr)
                    {
                        continue;
                    }

                    aBarriers[iNumTransitions].mpBuffer = shaderResource.mExternalResource.mpBuffer;
                    aBarriers[iNumTransitions].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumTransitions].mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;

                    WTFASSERT(aBarriers[iNumTransitions].mpBuffer, "no buffer");

                    ++iNumTransitions;
                }
                else if(shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT)
                {
                    if(aBarriers[iNumTransitions].mpImage == nullptr)
                    {
                        continue;
                    }

                    aBarriers[iNumTransitions].mpImage = shaderResource.mExternalResource.mpImage;
                    aBarriers[iNumTransitions].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[iNumTransitions].mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;

                    WTFASSERT(aBarriers[iNumTransitions].mpImage, "no buffer");

                    ++iNumTransitions;
                }
                //else
                //{
                //    WTFASSERT(0, "not handle case %d\n", shaderResource.mType);
                //}

                ++iIndex;
            }

            aBarriers.resize(iIndex);

            //if(mRenderDriverType == Render::Common::RenderDriverType::Vulkan)
            //{
            //    platformTransitionImageLayouts(
            //        renderJob,
            //        commandBuffer);
            //}
            
            commandBuffer.setPipelineState(pipelineState, *mpDevice);
            commandBuffer.dispatch(iNumDispatchX, iNumDispatchY, iNumDispatchZ);

            platformEndDebugMarker2(
                pCommandBuffer);

            //platformTransitionBarriers(
            //    aBarriers.data(),
            //    commandBuffer,
            //    iIndex,
            //    true);
        }

        /*
        **
        */
        void CRenderer::filloutCopyJobCommandBuffer(
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex)
        {
            Render::Common::RenderJobInfo const& renderJob = mpSerializer->getRenderJob(iRenderJob);

            //platformBeginRenderJobDebugEventMark(renderJob.mName);

            //RenderDriver::Common::CCommandBuffer& commandBuffer = *mpSerializer->getCommandBuffer(renderJob.mCommandBufferHandle).get();
            //RenderDriver::Common::CCommandAllocator& commandAllocator = *mpSerializer->getCommandAllocator(renderJob.maCommandAllocatorHandles[iTripleBufferIndex]).get();
            
            RenderDriver::Common::CCommandBuffer& commandBuffer = *mapQueueCommandBuffers[static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Copy)].get();
            RenderDriver::Common::CCommandAllocator& commandAllocator = *mapQueueCommandAllocators[static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Copy)].get();

            // number of copy source and copy destionation does not match up 
            assert(renderJob.maInputRenderTargetAttachments.size() == renderJob.maOutputRenderTargetAttachments.size());
            for(uint32_t iAttachment = 0; iAttachment < static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size()); iAttachment++)
            {
                WTFASSERT(
                    renderJob.maiOutputAttachmentMapping.size() == renderJob.maOutputRenderTargetAttachments.size(),
                    "Invalid input mapping %d to %d", renderJob.maiOutputAttachmentMapping.size(),
                    renderJob.maOutputRenderTargetAttachments.size());

                //auto& outputAttachmentInfo = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iAttachment]];
                auto const* pOutputAttachmentInfo = &renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iAttachment]];

                WTFASSERT(
                    pOutputAttachmentInfo->mType == AttachmentType::TextureOut || pOutputAttachmentInfo->mType == AttachmentType::BufferOut,
                    "Not output attachment type %d",
                    (uint32_t)pOutputAttachmentInfo->mType);

                WTFASSERT(
                    renderJob.maiInputAttachmentMapping.size() == renderJob.maInputRenderTargetAttachments.size(),
                    "Invalid input mapping %d to %d",
                    renderJob.maiInputAttachmentMapping.size(),
                    renderJob.maInputRenderTargetAttachments.size());

                auto const& inputAttachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iAttachment]];

                WTFASSERT(
                    inputAttachmentInfo.mType == AttachmentType::TextureIn || inputAttachmentInfo.mType == AttachmentType::BufferIn,
                    "Not input attachment type %d",
                    (uint32_t)inputAttachmentInfo.mType);

                // find the matching output attachment for the input
                uint32_t iOutputAttachment = iAttachment;
                for(uint32_t iCheckOutputAttachment = 0; iCheckOutputAttachment < static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size()); iCheckOutputAttachment++)
                {
                    auto const& checkOutputAttachment = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iCheckOutputAttachment]];
                    if(checkOutputAttachment.mParentName != "This" &&
                        checkOutputAttachment.mParentName == inputAttachmentInfo.mName &&
                        checkOutputAttachment.mParentJobName == inputAttachmentInfo.mParentJobName)
                    {
                        pOutputAttachmentInfo = &checkOutputAttachment;
                        iOutputAttachment = iCheckOutputAttachment;
                        break;
                    }
                }

                // copy
                if(inputAttachmentInfo.mType == AttachmentType::TextureIn && pOutputAttachmentInfo->mType == AttachmentType::BufferOut)
                {
                    // copy from texture to buffer (total read-only buffer)

                    WTFASSERT(0, "Implement me");

                }
                else if(inputAttachmentInfo.mType == AttachmentType::TextureIn && pOutputAttachmentInfo->mType == AttachmentType::TextureOut)
                {
                    // copy from texture to texture

                    auto const& pSrcRenderTarget = mpSerializer->getRenderTarget(renderJob.maInputRenderTargetAttachments[iAttachment]);
                    //auto const& pDestRenderTarget = mpSerializer->getRenderTarget(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]);
                    auto const& pDestRenderTargetImage = mpSerializer->getImage(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]);

                    //platformCopyImage(
                    //    *(pDestRenderTarget->getImage().get()), 
                    //    *(pSrcRenderTarget->getImage().get()), 
                    //    pSrcRenderTarget->getDescriptor().mbComputeShaderWritable);

                    RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[2];
                    aBarriers[0].mpImage = pSrcRenderTarget->getImage().get();
                    aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::CopySource;

                    aBarriers[1].mpImage = pDestRenderTargetImage.get();
                    aBarriers[1].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[1].mAfter = RenderDriver::Common::ResourceStateFlagBits::CopyDestination;

                    platformTransitionBarriers(
                        aBarriers,
                        commandBuffer, //*mpUploadCommandBuffer,
                        2,
                        false);

                    platformCopyImage2(
                        *(pDestRenderTargetImage.get()),
                        *(pSrcRenderTarget->getImage().get()),
                        commandBuffer,
                        pSrcRenderTarget->getDescriptor().mbComputeShaderWritable);

                    platformTransitionBarriers(
                        aBarriers,
                        commandBuffer, //*mpUploadCommandBuffer,
                        2,
                        true);
                }
                else if(inputAttachmentInfo.mType == AttachmentType::BufferIn && pOutputAttachmentInfo->mType == AttachmentType::TextureOut)
                {
                    auto& pSrcBuffer = mpSerializer->getBuffer(renderJob.maInputRenderTargetAttachments[iAttachment]);
                    auto& pDestRenderTarget = mpSerializer->getRenderTarget(renderJob.maOutputRenderTargetAttachments[iAttachment]);

                    platformCopyBufferToImage2(
                        *pDestRenderTarget->getImage(),
                        *pSrcBuffer,
                        commandBuffer,
                        0);
                }
                else if(inputAttachmentInfo.mType == AttachmentType::BufferIn && pOutputAttachmentInfo->mType == AttachmentType::BufferOut)
                {
                    auto& pSrcBuffer = mpSerializer->getBuffer(renderJob.maInputRenderTargetAttachments[iAttachment]);
                    auto& pDestBuffer = mpSerializer->getBuffer(renderJob.maOutputRenderTargetAttachments[iAttachment]);

                    platformCopyBufferToBuffer3(
                        pDestBuffer.get(),
                        pSrcBuffer.get(),
                        commandBuffer,
                        commandAllocator,
                        0,
                        0,
                        pSrcBuffer->getDescriptor().miSize,
                        false);
                }
            }

        }

        /*
        **
        */
        void CRenderer::filloutCopyJobCommandBuffer2(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer,
            RenderDriver::Common::CCommandAllocator* pCommandAllocator,
            uint32_t iRenderJob,
            uint32_t iTripleBufferIndex)
        {
            Render::Common::RenderJobInfo const& renderJob = mpSerializer->getRenderJob(iRenderJob);

            platformBeginDebugMarker2(
                renderJob.mName.c_str(),
                pCommandBuffer);

            RenderDriver::Common::CCommandBuffer& commandBuffer = *pCommandBuffer;
            RenderDriver::Common::CCommandAllocator& commandAllocator = *pCommandAllocator;

            // number of copy source and copy destionation does not match up 
            assert(renderJob.maInputRenderTargetAttachments.size() == renderJob.maOutputRenderTargetAttachments.size());
            for(uint32_t iAttachment = 0; iAttachment < static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size()); iAttachment++)
            {
                WTFASSERT(
                    renderJob.maiOutputAttachmentMapping.size() == renderJob.maOutputRenderTargetAttachments.size(),
                    "Invalid input mapping %d to %d", renderJob.maiOutputAttachmentMapping.size(),
                    renderJob.maOutputRenderTargetAttachments.size());

                auto const* pOutputAttachmentInfo = &renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iAttachment]];

                WTFASSERT(
                    pOutputAttachmentInfo->mType == AttachmentType::TextureOut || pOutputAttachmentInfo->mType == AttachmentType::BufferOut,
                    "Not output attachment type %d",
                    (uint32_t)pOutputAttachmentInfo->mType);

                WTFASSERT(
                    renderJob.maiInputAttachmentMapping.size() == renderJob.maInputRenderTargetAttachments.size(),
                    "Invalid input mapping %d to %d",
                    renderJob.maiInputAttachmentMapping.size(),
                    renderJob.maInputRenderTargetAttachments.size());

                auto const& inputAttachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iAttachment]];

                WTFASSERT(
                    inputAttachmentInfo.mType == AttachmentType::TextureIn || inputAttachmentInfo.mType == AttachmentType::BufferIn,
                    "Not input attachment type %d",
                    (uint32_t)inputAttachmentInfo.mType);

                // find the matching output attachment for the input
                uint32_t iOutputAttachment = iAttachment;
                for(uint32_t iCheckOutputAttachment = 0; iCheckOutputAttachment < static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size()); iCheckOutputAttachment++)
                {
                    auto const& checkOutputAttachment = renderJob.maAttachmentInfo[renderJob.maiOutputAttachmentMapping[iCheckOutputAttachment]];
                    if(checkOutputAttachment.mParentName != "This" &&
                        checkOutputAttachment.mParentName == inputAttachmentInfo.mName &&
                        checkOutputAttachment.mParentJobName == inputAttachmentInfo.mParentJobName)
                    {
                        pOutputAttachmentInfo = &checkOutputAttachment;
                        iOutputAttachment = iCheckOutputAttachment;
                        break;
                    }
                }

                // copy
                if(inputAttachmentInfo.mType == AttachmentType::TextureIn && pOutputAttachmentInfo->mType == AttachmentType::BufferOut)
                {
                    // copy from texture to buffer (total read-only buffer)

                    auto const& pSrcRenderTarget = mpSerializer->getRenderTarget(renderJob.maInputRenderTargetAttachments[iAttachment]);

                    WTFASSERT(0, "Implement me");

                }
                else if(inputAttachmentInfo.mType == AttachmentType::TextureIn && pOutputAttachmentInfo->mType == AttachmentType::TextureOut)
                {
                    // copy from texture to texture

                    auto const& pSrcRenderTarget = mpSerializer->getRenderTarget(renderJob.maInputRenderTargetAttachments[iAttachment]);
                    auto const& pDestRenderTargetImage = mpSerializer->getImage(renderJob.maOutputRenderTargetAttachments[iOutputAttachment]);

                    RenderDriver::Common::Utils::TransitionBarrierInfo aBarriers[2];
                    aBarriers[0].mpImage = pSrcRenderTarget->getImage().get();
                    aBarriers[0].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[0].mAfter = RenderDriver::Common::ResourceStateFlagBits::CopySource;
                    
                    aBarriers[1].mpImage = pDestRenderTargetImage.get();
                    aBarriers[1].mBefore = RenderDriver::Common::ResourceStateFlagBits::Common;
                    aBarriers[1].mAfter = RenderDriver::Common::ResourceStateFlagBits::CopyDestination;

                    platformTransitionBarriers(
                        aBarriers,
                        commandBuffer, //*mpUploadCommandBuffer,
                        2,
                        false,
                        (void*)&renderJob);

                    platformCopyImage2(
                        *(pDestRenderTargetImage.get()),
                        *(pSrcRenderTarget->getImage().get()),
                        commandBuffer,
                        pSrcRenderTarget->getDescriptor().mbComputeShaderWritable);

                    //platformTransitionBarriers(
                    //    aBarriers,
                    //    commandBuffer, //*mpUploadCommandBuffer,
                    //    2,
                    //    true);
                }
                else if(inputAttachmentInfo.mType == AttachmentType::BufferIn && pOutputAttachmentInfo->mType == AttachmentType::TextureOut)
                {
                    auto& pSrcBuffer = mpSerializer->getBuffer(renderJob.maInputRenderTargetAttachments[iAttachment]);
                    auto& pDestRenderTarget = mpSerializer->getRenderTarget(renderJob.maOutputRenderTargetAttachments[iAttachment]);

                    platformCopyBufferToImage2(
                        *pDestRenderTarget->getImage(),
                        *pSrcBuffer,
                        commandBuffer,
                        0);
                }
                else if(inputAttachmentInfo.mType == AttachmentType::BufferIn && pOutputAttachmentInfo->mType == AttachmentType::BufferOut)
                {
                    auto& pSrcBuffer = mpSerializer->getBuffer(renderJob.maInputRenderTargetAttachments[iAttachment]);
                    auto& pDestBuffer = mpSerializer->getBuffer(renderJob.maOutputRenderTargetAttachments[iAttachment]);

                    platformCopyBufferToBuffer3(
                        pDestBuffer.get(),
                        pSrcBuffer.get(),
                        commandBuffer,
                        commandAllocator,
                        0,
                        0,
                        pSrcBuffer->getDescriptor().miSize,
                        false);
                }
            }

            platformEndDebugMarker2(pCommandBuffer);

        }

        /*
        **
        */
        void CRenderer::execRenderJobs2()
        {
            waitForCopyJobs();

            platformBeginDebugMarker("Execute Render Jobs 2");

            // exec command buffer
            if(mbDataUpdated)
            {
                mpUploadCommandBuffer->close();
                mpCopyCommandQueue->execCommandBuffer(*mpUploadCommandBuffer, *mpDevice);
                mbDataUpdated = false;

                // wait until done
                RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
                placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
                placeFenceDesc.miFenceValue = miCopyCommandFenceValue++;
                placeFenceDesc.mType = RenderDriver::Common::FenceType::CPU;
                mpUploadFence->place(placeFenceDesc);
                mpUploadFence->waitCPU(UINT64_MAX);

                clearUploadBuffers();
                mpUploadCommandAllocator->reset();
                mpUploadCommandBuffer->reset();
            }

            auto start = std::chrono::high_resolution_clock::now();

            if(mPreRenderCopyJob.maUploadBuffers.size() > 0)
            {
                platformBeginDebugMarker("Pre Cache Copy Render Jobs");
                mpCopyCommandQueue->execCommandBuffer(
                    *mPreRenderCopyJob.mCommandBuffer.get(),
                    *mpDevice);
                // wait until done
                RenderDriver::Common::PlaceFenceDescriptor placeFenceDesc = {};
                placeFenceDesc.mpCommandQueue = mpCopyCommandQueue.get();
                placeFenceDesc.miFenceValue = ++miCopyCommandFenceValue;
                mpUploadFence->place(placeFenceDesc);
                mpUploadFence->waitCPU(UINT64_MAX);
                platformEndDebugMarker();
            }

            auto const& aRenderJobs = mpSerializer->getRenderJobs();
            uint64_t iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

            std::vector<std::vector<RenderJobFenceInfo>>* papRenderJobsByType = &maRenderJobsByType;

            uint32_t iCurrRenderJob = 0;
            uint32_t iQueueCompleted = 0;
            int32_t aiQueueRenderJobIndex[RenderDriver::Common::CCommandQueue::NumTypes] = { 0, 0, 0, 0 };

            uint32_t iCurrSwapChainBackBufferIndex = mpSwapChain->getCurrentBackBufferIndex();

            uint64_t aiFenceValues[RenderDriver::Common::CCommandQueue::NumTypes] = { 0, 0, 0, 0 };

            std::vector<RenderDriver::Common::CCommandBuffer*> apLastExecCommandBuffersForQueue(RenderDriver::Common::CCommandQueue::NumTypes);
            std::vector<RenderDriver::Common::CCommandAllocator*> apLastExecCommandAllocatorsForQueue(RenderDriver::Common::CCommandQueue::NumTypes);
            for(uint32_t iQueue = 0; iQueue < RenderDriver::Common::CCommandQueue::NumTypes; iQueue++)
            {
                apLastExecCommandBuffersForQueue[iQueue] = nullptr;
                apLastExecCommandAllocatorsForQueue[iQueue] = nullptr;
            }

            std::map<std::string, uint64_t> aSpinTimes;
            std::map<std::string, uint64_t> aWaitTimes;
            std::map<std::string, uint64_t> aQueueWaitTimes;

            RenderDriver::Common::CCommandQueue* pGraphicsCommandQueue = mpGraphicsCommandQueue.get();
            RenderDriver::Common::CCommandQueue* pComputeCommandQueue = mpComputeCommandQueue.get();
            RenderDriver::Common::CCommandQueue* pCopyCommandQueue = mpCopyCommandQueue.get();
            RenderDriver::Common::CCommandQueue* pGPUCopyCommandQueue = mpGPUCopyCommandQueue.get();

            static uint64_t saiTimeElapsed[RenderDriver::Common::CCommandQueue::NumTypes];
            static uint64_t saiSameQueueWaitTimes[RenderDriver::Common::CCommandQueue::NumTypes];
            memset(saiTimeElapsed, 0, sizeof(uint64_t) * RenderDriver::Common::CCommandQueue::NumTypes);
            memset(saiSameQueueWaitTimes, 0, sizeof(uint64_t) * RenderDriver::Common::CCommandQueue::NumTypes);

            struct CommandQueueWorkData
            {
                std::vector<std::vector<RenderJobFenceInfo>>*           mpapRenderJobsByType;
                uint32_t*                                               miQueueCompleted;
                uint64_t*                                               maiFenceValues;
                uint64_t*                                               maiLastFenceValues;
                int32_t*                                                maiQueueRenderJobIndex;
                RenderDriver::Common::CCommandQueue*                    mpGraphicsCommandQueue;
                RenderDriver::Common::CCommandQueue*                    mpComputeCommandQueue;
                RenderDriver::Common::CCommandQueue*                    mpCopyCommandQueue;
                RenderDriver::Common::CCommandQueue*                    mpGPUCopyCommandQueue;
                std::vector<RenderDriver::Common::CCommandBuffer*>*     mpapLastExecCommandBuffersForQueue;
                std::vector<RenderDriver::Common::CCommandAllocator*>*  mpapLastExecCommandAllocatorsForQueue;
                uint32_t                                                miQueueType;
                uint32_t                                                miCurrSwapChainBackBufferIndex;
                Render::Common::CRenderer*                              mpRenderer;
                uint64_t                                                miStartCopyQueueFenceValue;
            };

            CommandQueueWorkData aWorkData[32];
            
            // reset the command queue fences to start from 0
            for(uint32_t iQueueType = 0; iQueueType < RenderDriver::Common::CCommandQueue::NumTypes; iQueueType++)
            {
                if(iQueueType == static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Graphics))
                {
                    mapCommandQueueFences[iQueueType]->reset(mpGraphicsCommandQueue.get());
                }
                else if(iQueueType == static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Compute))
                {
                    mapCommandQueueFences[iQueueType]->reset(mpComputeCommandQueue.get());
                }
                else if(iQueueType == static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Copy))
                {
                    mapCommandQueueFences[iQueueType]->reset(mpCopyCommandQueue.get());
                }
                else if(iQueueType == static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::CopyGPU))
                {
                    mapCommandQueueFences[iQueueType]->reset(mpGPUCopyCommandQueue.get());
                }

                aiQueueRenderJobIndex[iQueueType] = 0;
            }

            // counter for the number of queues done with all the commands
            static std::atomic<uint32_t> siNumQueueDone;
            siNumQueueDone = 0;

            // count the job types to wait for
            uint32_t iNumQueuesToWait = 0;
            for(uint32_t iQueueType = 0; iQueueType < RenderDriver::Common::CCommandQueue::NumTypes; iQueueType++)
            {
                if(maRenderJobsByType[iQueueType].size() > 0 && iQueueType != RenderDriver::Common::CCommandQueue::Copy)
                {
                    ++iNumQueuesToWait;
                }
            }

            // apply any updates in the copy fence value
            maiRenderJobFenceValues[RenderDriver::Common::CCommandQueue::Type::Copy] = miCopyCommandFenceValue;

            uint64_t aiLastFenceValues[RenderDriver::Common::CCommandQueue::Type::NumTypes];
            memcpy(aiLastFenceValues, maiRenderJobFenceValues, sizeof(uint64_t)* RenderDriver::Common::CCommandQueue::Type::NumTypes);

            for(uint32_t iQueueType = 0; iQueueType < RenderDriver::Common::CCommandQueue::NumTypes; iQueueType++)
            {
                aWorkData[iQueueType].mpapRenderJobsByType = &maRenderJobsByType;
                aWorkData[iQueueType].miQueueCompleted = &iQueueCompleted;
                aWorkData[iQueueType].maiFenceValues = aiFenceValues;
                aWorkData[iQueueType].maiQueueRenderJobIndex = aiQueueRenderJobIndex;
                aWorkData[iQueueType].mpGraphicsCommandQueue = pGraphicsCommandQueue;
                aWorkData[iQueueType].mpComputeCommandQueue = pComputeCommandQueue;
                aWorkData[iQueueType].mpCopyCommandQueue = pCopyCommandQueue;
                aWorkData[iQueueType].mpGPUCopyCommandQueue = pGPUCopyCommandQueue;
                aWorkData[iQueueType].mpapLastExecCommandBuffersForQueue = &apLastExecCommandBuffersForQueue;
                aWorkData[iQueueType].mpapLastExecCommandAllocatorsForQueue = &apLastExecCommandAllocatorsForQueue;
                aWorkData[iQueueType].miQueueType = iQueueType;
                aWorkData[iQueueType].miCurrSwapChainBackBufferIndex = iCurrSwapChainBackBufferIndex;
                aWorkData[iQueueType].mpRenderer = this;
                aWorkData[iQueueType].miStartCopyQueueFenceValue = miCopyCommandFenceValue;
                aWorkData[iQueueType].maiLastFenceValues = aiLastFenceValues;

                JobManager::instance()->addToWorkerQueue(
                    [](void* pJobData, uint32_t iWorkerThread)
                    {
                        CommandQueueWorkData* pWorkData = reinterpret_cast<CommandQueueWorkData*>(pJobData);
                        auto papRenderJobsByType = pWorkData->mpapRenderJobsByType;
                        auto iQueueCompleted = pWorkData->miQueueCompleted;
                        auto aiFenceValues = pWorkData->maiFenceValues;
                        auto aiLastFenceValues = pWorkData->maiLastFenceValues;
                        auto aiQueueRenderJobIndex = pWorkData->maiQueueRenderJobIndex;
                        auto pGraphicsCommandQueue = pWorkData->mpGraphicsCommandQueue;
                        auto pComputeCommandQueue = pWorkData->mpComputeCommandQueue;
                        auto pCopyCommandQueue = pWorkData->mpCopyCommandQueue;
                        auto pGPUCopyCommandQueue = pWorkData->mpGPUCopyCommandQueue;
                        auto apLastExecCommandBuffersForQueue = pWorkData->mpapLastExecCommandBuffersForQueue;
                        auto apLastExecCommandAllocatorsForQueue = pWorkData->mpapLastExecCommandAllocatorsForQueue;
                        auto iQueueType = pWorkData->miQueueType;
                        auto iCurrSwapChainBackBufferIndex = pWorkData->miCurrSwapChainBackBufferIndex;
                        auto pRenderer = pWorkData->mpRenderer;
                        auto iStartCopyQueueFenceValue = pWorkData->miStartCopyQueueFenceValue;

                        RenderJobInfo* pRenderJob = nullptr;
                        std::vector<std::vector<RenderJobFenceInfo>>& aRenderJobByType = *papRenderJobsByType;

                        // command queue
                        RenderDriver::Common::CCommandQueue* pCommandQueue = pGraphicsCommandQueue;
                        if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Compute)
                        {
                            pCommandQueue = pComputeCommandQueue;
                        }
                        else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Copy)
                        {
                            pCommandQueue = pCopyCommandQueue;
                        }
                        else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::CopyGPU)
                        {
                            pCommandQueue = pGPUCopyCommandQueue;
                        }

                        bool bHasImgui = false;
                        uint32_t iNumJobs = static_cast<uint32_t>(aRenderJobByType[iQueueType].size());

                        for(uint32_t iJob = 0; iJob < iNumJobs; iJob++)
                        {
                            auto renderJobStartTime = std::chrono::high_resolution_clock::now();

                            auto& renderJobByType = aRenderJobByType[iQueueType][iJob];
                            pRenderJob = renderJobByType.mpRenderJob;

                            uint32_t iRenderJobIndex = aRenderJobByType[iQueueType][iJob].miRenderJobIndex;

                            // command buffer for this job 
                            RenderDriver::Common::CCommandBuffer* pCommandBuffer = nullptr;
                            if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Graphics)
                            {
                                WTFASSERT(iJob < pRenderer->mapQueueGraphicsCommandBuffers.size(), "vector out of bounds %d (%d)",
                                    iJob, 
                                    pRenderer->mapQueueGraphicsCommandBuffers.size());

                                pCommandBuffer = pRenderer->mapQueueGraphicsCommandBuffers[iJob].get();
                            }
                            else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Compute)
                            {
                                WTFASSERT(iJob < pRenderer->mapQueueComputeCommandBuffers.size(), "vector out of bounds %d (%d)",
                                    iJob,
                                    pRenderer->mapQueueComputeCommandBuffers.size());

                                pCommandBuffer = pRenderer->mapQueueComputeCommandBuffers[iJob].get();
                            }
                            else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Copy)
                            {
                                WTFASSERT(iJob < pRenderer->mapQueueCopyCommandBuffers.size(), "vector out of bounds %d (%d)",
                                    iJob,
                                    pRenderer->mapQueueCopyCommandBuffers.size());

                                pCommandBuffer = pRenderer->mapQueueCopyCommandBuffers[iJob].get();
                            }
                            else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::CopyGPU)
                            {
                                WTFASSERT(iJob < pRenderer->mapQueueGPUCopyCommandBuffers.size(), "vector out of bounds %d (%d)",
                                    iJob,
                                    pRenderer->mapQueueGPUCopyCommandBuffers.size());

                                pCommandBuffer = pRenderer->mapQueueGPUCopyCommandBuffers[iJob].get();
                            }
                            WTFASSERT(pCommandBuffer, "not valid command buffer");

                            char szRenderJobName[128];
                            sprintf(szRenderJobName, "%s %d", pRenderJob->mName.c_str(), pRenderer->getFrameIndex());

                            //pRenderer->platformBeginDebugMarker(
                            //    szRenderJobName,
                            //    pCommandQueue
                            //);

                            pRenderer->platformBeginDebugMarker("Fill Out Command Buffer");

                            RenderDriver::Common::CFence* pFence = pRenderer->mapCommandQueueFences[iQueueType].get();

                            // wait on parent job
                            for(uint32_t iWaitParent = 0; iWaitParent < aRenderJobByType[iQueueType][iJob].miNumParents; iWaitParent++)
                            {
                                // same type of queue
                                WaitParentInfo& waitParentInfo = aRenderJobByType[iQueueType][iJob].maWaitParentInfo[iWaitParent];
                                if(waitParentInfo.mParentCommandQueueType == iQueueType)
                                {
                                    continue;
                                }

                                WTFASSERT(waitParentInfo.mParentCommandQueueType != iQueueType, "Not the same queue type %d != %d",
                                    waitParentInfo.mParentCommandQueueType,
                                    iQueueType);

                                // wait on parent's fence on the queue
                                RenderDriver::Common::CFence* pParentFence = pRenderer->mapCommandQueueFences[static_cast<uint32_t>(waitParentInfo.mParentCommandQueueType)].get();
                                if(iQueueType != waitParentInfo.mParentCommandQueueType)
                                {
                                    pParentFence->waitGPU(
                                        pCommandQueue, 
                                        pFence,
                                        waitParentInfo.miStartOnFenceValue + aiLastFenceValues[waitParentInfo.mParentCommandQueueType]);
                                }
                            }

                            // fill out the commands first, don't execute just yet
                            if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Graphics)
                            {
                                if(pRenderJob->mPassType == Render::Common::PassType::Imgui)
                                {
                                    // imgui will fillout its own command buffer using the graphics queue command buffer
                                    pCommandBuffer = pRenderer->mapQueueGraphicsCommandBuffers[iJob].get();
                                    pRenderer->platformRenderImgui2(
                                        iCurrSwapChainBackBufferIndex,
                                        *pCommandBuffer);

                                    bHasImgui = true;
                                }
                                else
                                {
                                    Render::Common::RenderJobInfo const& renderJob = pRenderer->mpSerializer->getRenderJob(iRenderJobIndex);
                                    pRenderer->filloutGraphicsJobCommandBuffer2(
                                        pCommandBuffer,
                                        iRenderJobIndex, 
                                        iCurrSwapChainBackBufferIndex);
                                }

                                WTFASSERT(pCommandBuffer->getID().find("Graphics") != std::string::npos, "not the correct command buffer \"%s\"",
                                    pCommandBuffer->getID().c_str());
                            }
                            else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Compute)
                            {
                                pRenderer->filloutComputeJobCommandBuffer2(
                                    pCommandBuffer,
                                    (renderJobByType.miOrigRenderJobIndex != UINT32_MAX) ? renderJobByType.miOrigRenderJobIndex : iRenderJobIndex,
                                    iCurrSwapChainBackBufferIndex,
                                    renderJobByType.mDispatches);

                                WTFASSERT(pCommandBuffer->getID().find("Compute") != std::string::npos, "not the correct command buffer \"%s\"",
                                    pCommandBuffer->getID().c_str());
                            }
                            else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Copy)
                            {
                                pRenderer->filloutCopyJobCommandBuffer2(
                                    pCommandBuffer,
                                    pRenderer->mapQueueCopyCommandAllocators[iJob].get(),
                                    iRenderJobIndex,
                                    iCurrSwapChainBackBufferIndex);

                                WTFASSERT(pCommandBuffer->getID().find("Copy") != std::string::npos, "not the correct command buffer \"%s\"",
                                    pCommandBuffer->getID().c_str());
                            }
                            else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::CopyGPU)
                            {
                                pRenderer->filloutCopyJobCommandBuffer2(
                                    pCommandBuffer,
                                    pRenderer->mapQueueGPUCopyCommandAllocators[iJob].get(),
                                    iRenderJobIndex,
                                    iCurrSwapChainBackBufferIndex);

                                WTFASSERT(pCommandBuffer->getID().find("GPU Copy") != std::string::npos, "not the correct command buffer \"%s\"",
                                    pCommandBuffer->getID().c_str());
                            }

                            //pRenderer->platformEndDebugMarker2(
                            //    pCommandBuffer
                            //);
                            

                            pRenderer->platformEndDebugMarker();        // Fill Out Command Buffer

                            pRenderer->platformBeginDebugMarker(
                                "Execute Command Buffer"
                            );

                            auto startExecute = std::chrono::high_resolution_clock::now();

                            // execute after waiting for parent job on the same queue 
                            if(pCommandBuffer)
                            {
                                pCommandBuffer->close();

                                pRenderer->platformPreFenceSignal(
                                    pRenderer->mapCommandQueueFences[iQueueType].get(),
                                    pCommandQueue,
                                    iQueueType,
                                    pRenderer->maiRenderJobFenceValues[iQueueType]);

                                if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Graphics)
                                {
                                    pGraphicsCommandQueue->execCommandBuffer(
                                        *pCommandBuffer,
                                        *pRenderer->mpDevice);
                                }
                                else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Compute)
                                {
                                    pComputeCommandQueue->execCommandBuffer(
                                        *pCommandBuffer, 
                                        *pRenderer->mpDevice);
                                }
                                else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Copy)
                                {
                                    pCopyCommandQueue->execCommandBuffer(
                                        *pCommandBuffer,
                                        *pRenderer->mpDevice);
                                }
                                else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::CopyGPU)
                                {
                                    pGPUCopyCommandQueue->execCommandBuffer(
                                        *pCommandBuffer,
                                        *pRenderer->mpDevice);
                                }

                                pRenderer->platformPostFenceSignal(
                                    pRenderer->mapCommandQueueFences[iQueueType].get(),
                                    pCommandQueue,
                                    iQueueType,
                                    pRenderer->maiRenderJobFenceValues[iQueueType]);
                            }
                            
                            uint64_t iExecuteElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startExecute).count();

                            pRenderer->platformEndDebugMarker();  // Execute Command Buffer

                            uint64_t iRenderJobElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - renderJobStartTime).count();
                            {
                                std::lock_guard<std::mutex> lock(sMutex);
                                pRenderer->maRenderJobExecTimes[pRenderJob->mName].miTimeDuration = iRenderJobElapsed;
                            }

                            //pRenderer->platformEndDebugMarker(
                            //    pCommandQueue
                            //);

                        }   // for job = 0 to num jobs in queue

                        // wait for the last job for the queue to finish
                        if(iNumJobs > 0)
                        {
                            if(iQueueType != RenderDriver::Common::CCommandQueue::Type::Copy)
                            {
                                pRenderer->mapCommandQueueFences[iQueueType]->waitCPU2(
                                    UINT64_MAX,
                                    pCommandQueue,
                                    pRenderer->maiRenderJobFenceValues[iQueueType]);
                                siNumQueueDone.fetch_add(1);
                            }
                        }
                    },
                    &aWorkData[iQueueType],
                        sizeof(CommandQueueWorkData),
                        false,
                        nullptr
                );
            }


            // wait till command queue jobs are done
            for(;;)
            {
                if(siNumQueueDone.load() >= iNumQueuesToWait)
                {
                    break;
                }
            }

static uint32_t siFrames = 0;
iElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

siFrames += 1;
start = std::chrono::high_resolution_clock::now();

            platformBeginDebugMarker("End Render Jobs");

            // callback after finishing all the render jobs
            for(auto const& callback : maFinishRenderJobCallbacks)
            {
                callback.second.mCallbackFunction(callback.first, callback.second.mpCallbackData);
            }

            for(uint32_t iQueueType = 0; iQueueType < RenderDriver::Common::CCommandQueue::NumTypes; iQueueType++)
            {
                if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Copy)
                {
                    continue;
                }

                // reset command allocators and command buffers
                uint32_t iNumJobs = static_cast<uint32_t>(maRenderJobsByType[iQueueType].size());
                for(uint32_t iJob = 0; iJob < iNumJobs; iJob++)
                {
                    if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Graphics)
                    {
                        mapQueueGraphicsCommandAllocators[iJob]->reset();
                        mapQueueGraphicsCommandBuffers[iJob]->reset();
                    }
                    else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Compute)
                    {
                        mapQueueComputeCommandAllocators[iJob]->reset();
                        mapQueueComputeCommandBuffers[iJob]->reset();
                    }
                    else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Copy)
                    {
                        mapQueueCopyCommandAllocators[iJob]->reset();
                        mapQueueCopyCommandBuffers[iJob]->reset();
                    }
                    else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::CopyGPU)
                    {
                        mapQueueGPUCopyCommandAllocators[iJob]->reset();
                        mapQueueGPUCopyCommandBuffers[iJob]->reset();
                    }
                }
            }

iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
              
            // copy queue closes upload command buffer, without any copy jobs, we need to manually close
            bool bCloseAndResetUploadCommandBuffer = (maRenderJobsByType[RenderDriver::Common::CCommandQueue::Type::Copy].size() <= 0);
            platformBeginDebugMarker("Reset Upload Buffers");
            platformResetUploadBuffers(bCloseAndResetUploadCommandBuffer);
            platformEndDebugMarker();

            // update copy fence value
            miCopyCommandFenceValue = maiRenderJobFenceValues[RenderDriver::Common::CCommandQueue::Type::Copy];

            miFrameIndex += 1;


            platformEndDebugMarker();

            platformEndDebugMarker();

#if 0
            // global brick
            {
                float const kfBrickSize = 0.1f;

                RenderDriver::Common::CBuffer* pGlobalBrickInfoBuffer = mpSerializer->getBuffer("Global Brick Info Buffer");
                std::vector<char> acBuffer(pGlobalBrickInfoBuffer->getDescriptor().miSize);
                char* pacBuffer = acBuffer.data();
                uint32_t iOffset = 0;
                uint32_t const kiChunkSize = 10 * 1024 * 1024;
                uint32_t iNumDivisions = uint32_t(ceilf(float(pGlobalBrickInfoBuffer->getDescriptor().miSize) / float(kiChunkSize)));
                for(uint32_t i = 0; i < iNumDivisions; i++)
                {
                    platformCopyBufferToCPUMemory(
                        pGlobalBrickInfoBuffer,
                        pacBuffer + iOffset,
                        iOffset,
                        kiChunkSize);
                    iOffset += kiChunkSize;
                }

                RenderDriver::Common::CBuffer* pGlobalBrickGeneralInfoBuffer = mpSerializer->getBuffer("Global Brick General Info Buffer");
                std::vector<char> acGlobalBrickGeneralInfoBuffer(pGlobalBrickGeneralInfoBuffer->getDescriptor().miSize);
                char* pacGlobalBrickGeneralInfoBuffer = acGlobalBrickGeneralInfoBuffer.data();
                platformCopyBufferToCPUMemory(
                    pGlobalBrickGeneralInfoBuffer,
                    pacGlobalBrickGeneralInfoBuffer,
                    0,
                    acGlobalBrickGeneralInfoBuffer.size());

                Test::HLSL::ByteAddress address = { 0 };
                float3 minGlobalWorldPosition = Test::HLSL::loadFloat3(pacGlobalBrickGeneralInfoBuffer, address);
                uint3 numBricks;
                numBricks.x = Test::HLSL::loadUInt(pacGlobalBrickGeneralInfoBuffer, address);
                numBricks.y = Test::HLSL::loadUInt(pacGlobalBrickGeneralInfoBuffer, address);
                numBricks.z = Test::HLSL::loadUInt(pacGlobalBrickGeneralInfoBuffer, address);

                std::vector<float3> aBrickPositions;

                address.miPtr = sizeof(uint32_t);
                uint32_t iNumBricks = numBricks.x * numBricks.y * numBricks.z;
                for(uint32_t iBrick = 0; iBrick < iNumBricks; iBrick++)
                {
                    uint32_t iBrixelAddress = Test::HLSL::loadUInt(pacBuffer, address);
                    float fClosestDistanceToNeighbor = Test::HLSL::loadFloat(pacBuffer, address);
                    if(iBrixelAddress > 0)
                    {
                        int32_t iBrickX = iBrick % numBricks.x;
                        int32_t iBrickZ = iBrick / (numBricks.x * numBricks.y);
                        int32_t iBrickY = (iBrick - iBrickZ * (numBricks.x * numBricks.y)) / numBricks.x;

                        float3 brickPosition = minGlobalWorldPosition + float3(
                            float(iBrickX),
                            float(iBrickY),
                            float(iBrickZ)) * kfBrickSize;
                        aBrickPositions.push_back(brickPosition);
                    }
                }

                Test::HLSL::Voxel::outputDebugVoxelOBJ(
                    "c:\\Users\\Dingwings\\demo-models\\rasterizer\\global-brick-gpu-to-cpu-verification.obj",
                    aBrickPositions,
                    kfBrickSize);
                int iDebug = 1;
            }
#endif // #if 0
            
#if 0
            {
                Test::HLSL::ByteAddress address = { sizeof(uint32_t) };

                std::vector<char> acMeshBrickBuffer;
                copyToCPUBufferFromGPU(
                    acMeshBrickBuffer,
                    "Mesh Bricks");
                char* pacMeshBrickBuffer = acMeshBrickBuffer.data();

                std::vector<char> acMeshBrickMapBuffer;
                copyToCPUBufferFromGPU(
                    acMeshBrickMapBuffer,
                    "Mesh Brick Map");
                char* pacMeshBrickMapBuffer = acMeshBrickMapBuffer.data();

                struct MeshBrickMap
                {
                    uint32_t        miStartAddress;
                    uint32_t        miNumBricks;
                };
                std::vector<MeshBrickMap> aBrickMap;

                address.miPtr = 0;
                uint32_t iNumMeshes = Test::HLSL::loadUInt(pacMeshBrickMapBuffer, address);
                uint32_t iNumTotalBricks = 0;
                for(uint32_t i = 0; i < iNumMeshes; i++)
                {
                    MeshBrickMap brickMap;
                    brickMap.miStartAddress = Test::HLSL::loadUInt(pacMeshBrickMapBuffer, address);
                    brickMap.miNumBricks = Test::HLSL::loadUInt(pacMeshBrickMapBuffer, address);
                    iNumTotalBricks += brickMap.miNumBricks;
                    aBrickMap.push_back(brickMap);
                }

                struct MeshBrickInfo
                {
                    uint32_t        miBrixelAddress;
                    float3          mPosition;
                    float           mfShortestDistanceToOther;
                };
                std::vector<MeshBrickInfo> aBricks;

                address.miPtr = sizeof(uint32_t);
                for(uint32_t i = 0; i < iNumTotalBricks; i++)
                {
                    MeshBrickInfo brick;
                    brick.miBrixelAddress = Test::HLSL::loadUIntRW(pacMeshBrickBuffer, address);
                    brick.mPosition = Test::HLSL::loadFloat3RW(pacMeshBrickBuffer, address);
                    brick.mfShortestDistanceToOther = Test::HLSL::loadFloatRW(pacMeshBrickBuffer, address);
                    if(brick.mfShortestDistanceToOther >= 0.1f)
                    {
                        int iDebug = 1;
                    }

                    aBricks.push_back(brick);
                }

                int iDebug = 1;
            }
#endif // #if 0

#if 0
            {
                RenderDriver::Common::CBuffer* pValidBrickInfoBuffer = mpSerializer->getBuffer("Valid Brick Info Buffer");
                std::vector<char> acBuffer(pValidBrickInfoBuffer->getDescriptor().miSize);
                char* pacBuffer = acBuffer.data();
                uint32_t iOffset = 0;
                uint32_t const kiChunkSize = 10 * 1024 * 1024;
                uint32_t iNumDivisions = uint32_t(ceilf(float(pValidBrickInfoBuffer->getDescriptor().miSize) / float(kiChunkSize)));
                for(uint32_t i = 0; i < iNumDivisions; i++)
                {
                    platformCopyBufferToCPUMemory(
                        pValidBrickInfoBuffer,
                        pacBuffer + iOffset,
                        iOffset,
                        kiChunkSize);
                    iOffset += kiChunkSize;
                }

                struct ValidBrickInfo
                {
                    float3          mPosition;
                    uint32_t        miBrickIndex;
                    uint32_t        miMesh;
                };
                
                std::vector<ValidBrickInfo> aValidBricks;
                Test::HLSL::ByteAddress address = { 0 };
                uint32_t iNumValidBricks = Test::HLSL::loadUInt(pacBuffer, address);
                for(uint32_t i = 0; i < iNumValidBricks; i++)
                {
                    ValidBrickInfo validBrickInfo;
                    validBrickInfo.mPosition = Test::HLSL::loadFloat3(pacBuffer, address);
                    validBrickInfo.miBrickIndex = Test::HLSL::loadUInt(pacBuffer, address);
                    validBrickInfo.miMesh = Test::HLSL::loadUInt(pacBuffer, address);
                    aValidBricks.push_back(validBrickInfo);
                }

                int iDebug = 1;

            }
#endif // #if 0

#if 0
            if(miFrameIndex >= 10)
            {
                RenderDriver::Common::CBuffer* pBrixelBuffer = nullptr;
                RenderDriver::Common::CBuffer* pMeshBBoxBuffer = nullptr;
                RenderDriver::Common::CBuffer* pBrickBuffer = nullptr;
                RenderDriver::Common::CBuffer* pBrickMapBuffer = nullptr;
                RenderDriver::Common::CBuffer* pMeshBrixelIndirectDiffuseBuffer = nullptr;
                for(auto const& renderJob : mpSerializer->getRenderJobs())
                {
                    for(auto const& shaderResource : renderJob.maShaderResourceInfo)
                    {
                        if(shaderResource.mName == "pgaMeshBrixel")
                        {
                            pBrixelBuffer = shaderResource.mExternalResource.mpBuffer;
                        }

                        if(shaderResource.mName == "pgaMeshBBox")
                        {
                            pMeshBBoxBuffer = shaderResource.mExternalResource.mpBuffer;
                        }

                        if(shaderResource.mName == "pgaMeshBrick")
                        {
                            pBrickBuffer = shaderResource.mExternalResource.mpBuffer;
                        }

                        if(shaderResource.mName == "pgaMeshBrickMap")
                        {
                            pBrickMapBuffer = shaderResource.mExternalResource.mpBuffer;
                        }

                        if(shaderResource.mName == "pgaMeshBrixelIndirectDiffuseInfo")
                        {
                            pMeshBrixelIndirectDiffuseBuffer = shaderResource.mExternalResource.mpBuffer;
                        }
                    }

                    if(pBrixelBuffer && pMeshBBoxBuffer && pBrickBuffer && pBrickMapBuffer && pMeshBrixelIndirectDiffuseBuffer)
                    {
                        break;
                    }
                }

                std::vector<char> aBrixelBuffer(pBrixelBuffer->getDescriptor().miSize);
                char* pacBrixelBuffer = aBrixelBuffer.data();
                uint32_t iOffset = 0;
                uint32_t const kiChunkSize = 10 * 1024 * 1024;
                uint32_t iNumDivisions = uint32_t(ceilf(float(pBrixelBuffer->getDescriptor().miSize) / float(kiChunkSize)));
                for(uint32_t i = 0; i < iNumDivisions; i++)
                {
                    platformCopyBufferToCPUMemory(
                        pBrixelBuffer,
                        aBrixelBuffer.data() + iOffset,
                        iOffset,
                        kiChunkSize);
                    iOffset += kiChunkSize;
                }

                std::vector<char> aBrickBuffer(pBrickBuffer->getDescriptor().miSize);
                char* pacBrickBuffer = aBrickBuffer.data();
                iOffset = 0;
                iNumDivisions = uint32_t(ceilf(float(pBrickBuffer->getDescriptor().miSize) / float(kiChunkSize)));
                for(uint32_t i = 0; i < iNumDivisions; i++)
                {
                    platformCopyBufferToCPUMemory(
                        pBrickBuffer,
                        pacBrickBuffer + iOffset,
                        iOffset,
                        kiChunkSize);
                    iOffset += kiChunkSize;
                }

                std::vector<char> aBrixelIndirectDiffuseBuffer(pMeshBrixelIndirectDiffuseBuffer->getDescriptor().miSize);
                char* pacBrixelIndirectDiffuseBuffer = aBrixelIndirectDiffuseBuffer.data();
                iNumDivisions = uint32_t(ceilf(float(pMeshBrixelIndirectDiffuseBuffer->getDescriptor().miSize) / float(kiChunkSize)));
                iOffset = 0;
                for(uint32_t i = 0; i < iNumDivisions; i++)
                {
                    platformCopyBufferToCPUMemory(
                        pMeshBrixelIndirectDiffuseBuffer,
                        pacBrixelIndirectDiffuseBuffer + iOffset,
                        iOffset,
                        kiChunkSize);
                    iOffset += kiChunkSize;
                }

                float const kfBrickSize = 0.1f;
                float const kfBrixelSize = kfBrickSize / 8.0f;
                std::vector<float3> aBrixelPositions;
                std::vector<float3> aBrickPositions;

                std::vector<char> aMeshBBox(1 << 16);
                platformCopyBufferToCPUMemory(
                    pMeshBBoxBuffer,
                    aMeshBBox.data(),
                    0,
                    1 << 16);

                std::vector<char> aBrickMap(1 << 16);
                platformCopyBufferToCPUMemory(
                    pBrickMapBuffer,
                    aBrickMap.data(),
                    0,
                    1 << 16);

                uint32_t aiBrickOffsets[16];
                uint32_t iNumTotalBricks = 0;
                Test::HLSL::ByteAddress address = { 0 };
                uint32_t iNumBrickMaps = Test::HLSL::loadUInt(aBrickMap.data(), address);
                for(uint32_t iBrickMap = 0; iBrickMap < iNumBrickMaps; iBrickMap++)
                {
                    uint32_t iAddress = Test::HLSL::loadUInt(aBrickMap.data(), address);
                    uint32_t iNumBricks = Test::HLSL::loadUInt(aBrickMap.data(), address);

                    iNumTotalBricks += iNumBricks;
                    aiBrickOffsets[iBrickMap] = iNumTotalBricks;
                }

                Test::HLSL::ByteAddress brickAddress = { sizeof(uint32_t) };
                for(uint32_t iBrick = 0; iBrick < iNumTotalBricks; iBrick++)
                {
                    uint32_t iBrixelAddress = Test::HLSL::loadUInt(pacBrickBuffer, brickAddress);
                    float3 brickPosition = Test::HLSL::loadFloat3(pacBrickBuffer, brickAddress);
                    float fSize = Test::HLSL::loadFloat(pacBrickBuffer, brickAddress);
                    if(iBrixelAddress > 0)
                    {
                        aBrickPositions.push_back(brickPosition);

                        uint32_t iBrixelArrayIndex = 0;
                        for(float fZ = 0.0f; fZ < 8.0f; fZ += 1.0f)
                        {
                            for(float fY = 0.0f; fY < 8.0f; fY += 1.0f)
                            {
                                for(float fX = 0.0f; fX < 8.0f; fX += 1.0f)
                                {
                                    Test::HLSL::ByteAddress brixelAddress = { iBrixelAddress + 512 * sizeof(uint2) + iBrixelArrayIndex * sizeof(float) };
                                    float fDistance = Test::HLSL::loadFloat(pacBrixelBuffer, brixelAddress);
                                    if(fDistance != FLT_MAX)
                                    {
                                        float3 brixelPosition = brickPosition + float3(fX, fY, fZ) * kfBrixelSize;
                                        aBrixelPositions.push_back(brixelPosition);
                                    }
                                    
                                    iBrixelArrayIndex += 1;
                                }
                            }
                        }
                        
                    }
                }

                Test::HLSL::Voxel::outputDebugVoxelOBJ(
                    "c:\\Users\\Dingwings\\demo-models\\rasterizer\\brick-gpu-to-cpu-verification.obj",
                    aBrickPositions,
                    kfBrickSize);

                Test::HLSL::Voxel::outputDebugVoxelOBJ(
                    "c:\\Users\\Dingwings\\demo-models\\rasterizer\\brixel-gpu-to-cpu-verification.obj",
                    aBrixelPositions,
                    kfBrixelSize);
                int iDebug = 1;
            }
#endif // #if 0

            //{
            //    auto start = std::chrono::high_resolution_clock::now();
            //    uint64_t iElapsedUS = 0;
            //    while(iElapsedUS < 8000)
            //    {
            //        iElapsedUS = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
            //    }
            //}

            platformEndDebugMarker();



        }

        /*
        **
        */
        void CRenderer::waitForCopyJobs()
        {
            mapCommandQueueFences[RenderDriver::Common::CCommandQueue::Type::Copy]->waitCPU2(
                UINT64_MAX,
                mpCopyCommandQueue.get(),
                maiRenderJobFenceValues[RenderDriver::Common::CCommandQueue::Type::Copy]);

            // reset command allocators and command buffers
            uint32_t iNumJobs = static_cast<uint32_t>(maRenderJobsByType[RenderDriver::Common::CCommandQueue::Type::Copy].size());
            for(uint32_t iJob = 0; iJob < iNumJobs; iJob++)
            {
                if(mapQueueCopyCommandBuffers[iJob]->getState() != RenderDriver::Common::CommandBufferState::Initialized)
                {
                    mapQueueCopyCommandAllocators[iJob]->reset();
                    mapQueueCopyCommandBuffers[iJob]->reset();
                }
            }
        }

        /*
        **
        */
        void CRenderer::testRenderGraph(uint32_t iCurrSwapChainBackBufferIndex)
        {
            RenderDriver::Common::CCommandQueue* pGraphicsCommandQueue = mpGraphicsCommandQueue.get();
            RenderDriver::Common::CCommandQueue* pComputeCommandQueue = mpComputeCommandQueue.get();
            RenderDriver::Common::CCommandQueue* pCopyCommandQueue = mpCopyCommandQueue.get();

            std::vector<std::vector<RenderJobFenceInfo>>* papRenderJobsByType = &maRenderJobsByType;

            std::atomic<uint32_t> iJobCounter = 0;
            std::atomic<uint32_t> iWaitJobCounter = 0;
            RenderJobInfo* apJobsExecuted[128];
            RenderJobInfo* apWaitingJobs[128];

            uint32_t aiFenceValues[RenderDriver::Common::CCommandQueue::NumTypes] = { 0, 0, 0 };

            struct ExecInfo
            {
                std::string                                                         mRenderJobName;
                std::chrono::time_point<std::chrono::high_resolution_clock>         mStartExecTime;
                std::chrono::time_point<std::chrono::high_resolution_clock>         mEndExecTime;

                std::chrono::time_point<std::chrono::high_resolution_clock>         mStartWaitTime;
                std::chrono::time_point<std::chrono::high_resolution_clock>         mEndWaitTime;
            };

            ExecInfo aExecInfo[128];

            std::chrono::high_resolution_clock::time_point jobsStartTime = std::chrono::high_resolution_clock::now();

            uint64_t aiWaitTimes[3] = { 0, 0, 0 };

            uint32_t iCurrRenderJob = 0;
            uint32_t iQueueCompleted = 0;
            uint32_t aiQueueRenderJobIndex[RenderDriver::Common::CCommandQueue::NumTypes] = { 0, 0, 0 };
            std::vector<std::unique_ptr<std::thread>> apThreads(RenderDriver::Common::CCommandQueue::NumTypes);
            for(uint32_t iQueueType = 0; iQueueType < RenderDriver::Common::CCommandQueue::NumTypes; iQueueType++)
            {
                apThreads[iQueueType] = std::make_unique<std::thread>(
                    [&iCurrRenderJob,
                    &aiQueueRenderJobIndex,
                    &iJobCounter,
                    &iWaitJobCounter,
                    &iQueueCompleted,
                    &apJobsExecuted,
                    &apWaitingJobs,
                    &aiFenceValues,
                    &aExecInfo,
                    &aiWaitTimes,
                    iQueueType,
                    pGraphicsCommandQueue,
                    pComputeCommandQueue,
                    pCopyCommandQueue,
                    iCurrSwapChainBackBufferIndex,
                    papRenderJobsByType,
                    this]()
                    {
                        uint32_t iCurrJobIndex = 0;
                        RenderDriver::Common::CCommandQueue* pCommandQueue = pGraphicsCommandQueue;
                        RenderJobInfo* pRenderJob = nullptr;

                        std::vector<std::vector<RenderJobFenceInfo>>& aRenderJobByType = *papRenderJobsByType;

                        uint32_t iNumJobs = static_cast<uint32_t>(aRenderJobByType[iQueueType].size());
                        for(uint32_t iJob = 0; iJob < iNumJobs; iJob++)
                        {
                            std::chrono::time_point<std::chrono::high_resolution_clock> startTime = std::chrono::high_resolution_clock::now();

                            iCurrJobIndex = aiQueueRenderJobIndex[iQueueType];

                            if(iCurrJobIndex >= aRenderJobByType[iQueueType].size())
                            {
                                ++iQueueCompleted;
                            }

                            pRenderJob = aRenderJobByType[iQueueType][iCurrJobIndex].mpRenderJob;

                            std::chrono::time_point<std::chrono::high_resolution_clock> startWaitTime = std::chrono::high_resolution_clock::now();
                            for(uint32_t iWaitParent = 0; iWaitParent < aRenderJobByType[iQueueType][iCurrJobIndex].miNumParents; iWaitParent++)
                            {
                                WaitParentInfo& waitParentInfo = aRenderJobByType[iQueueType][iCurrJobIndex].maWaitParentInfo[iWaitParent];
                                uint32_t iWaitJob = iWaitJobCounter.fetch_add(1);

                                startWaitTime = std::chrono::high_resolution_clock::now();
                                while(aiFenceValues[waitParentInfo.mParentCommandQueueType] < waitParentInfo.miStartOnFenceValue)
                                {
                                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                                }
                                uint64_t iDuration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startWaitTime).count();
                                aiWaitTimes[iQueueType] += iDuration;
                            }

                            std::chrono::time_point<std::chrono::high_resolution_clock> endWaitTime = std::chrono::high_resolution_clock::now();

                            if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Graphics)
                            {
                                filloutGraphicsJobCommandBuffer(iCurrRenderJob, iCurrSwapChainBackBufferIndex);
                            }
                            else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Compute)
                            {
                                filloutComputeJobCommandBuffer(
                                    iCurrRenderJob, 
                                    iCurrSwapChainBackBufferIndex, 
                                    uint3(0, 0, 0));
                            }
                            else if(iQueueType == RenderDriver::Common::CCommandQueue::Type::Copy)
                            {
                                filloutCopyJobCommandBuffer(iCurrRenderJob, iCurrSwapChainBackBufferIndex);
                            }

                            std::chrono::time_point<std::chrono::high_resolution_clock> endTime = std::chrono::high_resolution_clock::now();

                            uint32_t iExecJob = iJobCounter.fetch_add(1);
                            apJobsExecuted[iExecJob] = pRenderJob;

                            ExecInfo execInfo = {};
                            execInfo.mRenderJobName = pRenderJob->mName;
                            execInfo.mStartExecTime = startTime;
                            execInfo.mEndExecTime = endTime;
                            execInfo.mStartWaitTime = startWaitTime;
                            execInfo.mEndWaitTime = endWaitTime;
                            aExecInfo[iExecJob] = execInfo;

                            aiQueueRenderJobIndex[iQueueType] += 1;
                            aiFenceValues[iQueueType] += 1;
                        }
                    });
            }

            for(uint32_t iThread = 0; iThread < RenderDriver::Common::CCommandQueue::NumTypes; iThread++)
            {
                apThreads[iThread]->join();
            }

            auto const& aRenderJobs = mpSerializer->getRenderJobs();
            for(uint32_t i = 0; i < iJobCounter; i++)
            {
                auto const& execInfo = aExecInfo[i];
                auto iter = std::find_if(
                    aRenderJobs.begin(),
                    aRenderJobs.end(),
                    [execInfo](RenderJobInfo const& renderJobInfo)
                    {
                        return (execInfo.mRenderJobName == renderJobInfo.mName);
                    });

                auto* pCommandBuffer = mpSerializer->getCommandBuffer(iter->mCommandBufferHandle).get();
                RenderDriver::Common::CommandBufferType type = pCommandBuffer->getType();
                std::string typeStr = "Graphics";
                if(type == RenderDriver::Common::CommandBufferType::Compute)
                {
                    typeStr = "Compute";
                }
                else if(type == RenderDriver::Common::CommandBufferType::Copy)
                {
                    typeStr = "Copy";
                }

                //uint64_t iStartMS = std::chrono::duration_cast<std::chrono::microseconds>(aExecInfo[i].mStartExecTime - jobsStartTime).count();
                //uint64_t iEndMS = std::chrono::duration_cast<std::chrono::microseconds>(aExecInfo[i].mEndExecTime - jobsStartTime).count();
                //uint64_t iWaitedMS = std::chrono::duration_cast<std::chrono::microseconds>(aExecInfo[i].mEndWaitTime - aExecInfo[i].mStartWaitTime).count();
                //DEBUG_PRINTF("%s (%s) %*sstart: %lld\t\t\t\tend: %lld\t\t\t\twaited: %lld\t\t\t\tintialized: %lld\n",
                //    aExecInfo[i].mRenderJobName.c_str(),
                //    typeStr.c_str(),
                //    80 - (aExecInfo[i].mRenderJobName.length() + typeStr.length()),
                //    "",
                //    iStartMS + iWaitedMS,
                //    iEndMS,
                //    iWaitedMS,
                //    iStartMS);
            }

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
        void CRenderer::setRenderJobDispatch(
            std::string const& renderJobName,
            uint32_t iX,
            uint32_t iY,
            uint32_t iZ)
        {
            auto& aRenderJobs = mpSerializer->getRenderJobs();
            auto iter = std::find_if(
                aRenderJobs.begin(),
                aRenderJobs.end(),
                [renderJobName](RenderJobInfo const& renderJob)
                {
                    return renderJob.mName == renderJobName;
                }
            );

            if(iter == aRenderJobs.end())
            {
                DEBUG_PRINTF("!!! NO RENDER JOB \"%s\"\n", renderJobName.c_str());
                return;
            }

            iter->miNumDispatchX = iX;
            iter->miNumDispatchY = iY;
            iter->miNumDispatchZ = iZ;
        }

        /*
        **
        */
        void CRenderer::copyToCPUBufferFromGPU(
            std::vector<char>& acBuffer,
            std::string const& bufferName)
        {
            uint32_t const kiChunkSize = 10 * 1024 * 1024;

            RenderDriver::Common::CBuffer* pGPUBuffer = mpSerializer->getBuffer(bufferName);
            acBuffer.resize(pGPUBuffer->getDescriptor().miSize);
            char* pacBuffer = acBuffer.data();
            uint32_t iOffset = 0;
            uint32_t iBufferSize = static_cast<uint32_t>(pGPUBuffer->getDescriptor().miSize);
            uint32_t iChunkSize = static_cast<uint32_t>((iBufferSize < kiChunkSize) ? iBufferSize : kiChunkSize);
            uint32_t iNumDivisions = uint32_t(ceilf(float(iBufferSize) / float(iChunkSize)));
            for(uint32_t i = 0; i < iNumDivisions; i++)
            {
                platformCopyBufferToCPUMemory(
                    pGPUBuffer,
                    pacBuffer + iOffset,
                    iOffset,
                    iChunkSize);
                iOffset += iChunkSize;
            }
        }

        /*
        **
        */
        void CRenderer::allocateCopyCommandBuffers(
            std::vector<std::shared_ptr<RenderDriver::Common::CCommandBuffer>>& aCopyCommandBuffers,
            std::vector<std::shared_ptr<RenderDriver::Common::CCommandAllocator>>& aCopyCommandAllocators,
            uint32_t iNumCopyCommandBuffers)
        {
            platformAllocateCopyCommandBuffers(
                aCopyCommandBuffers, 
                aCopyCommandAllocators,
                iNumCopyCommandBuffers);

            for(uint32_t i = 0; i < iNumCopyCommandBuffers; i++)
            {
                std::stringstream ss;
                ss << "SDF Command Buffer " << i;
                aCopyCommandBuffers[i]->setID(ss.str());

                std::stringstream ss2;
                ss2 << "SDF Command Allocator " << i;
                aCopyCommandAllocators[i]->setID(ss2.str());
            }
        }

        /*
        **
        */
        void CRenderer::allocateUploadBuffer(
            std::shared_ptr<RenderDriver::Common::CBuffer>& buffer,
            uint32_t iSize)
        {
            platformAllocateUploadBuffer(buffer, iSize);
        }

        /*
        **
        */
        void CRenderer::uploadResourceDataWithCommandBufferAndUploadBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandAllocator& commandAllocator,
            RenderDriver::Common::CBuffer& uploadBuffer,
            RenderDriver::Common::CBuffer& buffer,
            void* pRawSrcData,
            uint64_t iDataSize,
            uint64_t iDestDataOffset)
        {
            platformUploadResourceDataWithCommandBufferAndUploadBuffer(
                commandBuffer,
                commandAllocator,
                uploadBuffer,
                buffer,
                pRawSrcData,
                iDataSize,
                iDestDataOffset);
        }

        /*
        **
        */
        void CRenderer::encodeCopyCommandBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CCommandAllocator& commandAllocator,
            RenderDriver::Common::CBuffer& uploadBuffer,
            RenderDriver::Common::CBuffer& buffer,
            void* pRawSrcData,
            uint64_t iDataSize,
            uint64_t iDestDataOffset)
        {
            platformEncodeUploadBufferCommand(
                commandBuffer,
                commandAllocator,
                uploadBuffer,
                buffer,
                pRawSrcData,
                iDataSize,
                iDestDataOffset);
        }

        /*
        **
        */
        void CRenderer::executeUploadCommandBuffers(
            RenderDriver::Common::CCommandBuffer* const* apCommandBuffers,
            uint32_t iNumCommandBuffers)
        {
            platformExecuteCopyCommandBuffers(apCommandBuffers, iNumCommandBuffers);
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