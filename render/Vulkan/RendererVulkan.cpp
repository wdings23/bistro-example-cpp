#include <render/Vulkan/RendererVulkan.h>
#include <render/Vulkan/RenderJobVulkan.h>

#include <render-driver/Vulkan/CommandQueueVulkan.h>
#include <render-driver/Vulkan/PipelineStateVulkan.h>
#include <render-driver/Vulkan/CommandAllocatorVulkan.h>
#include <render-driver/Vulkan/CommandBufferVulkan.h>
#include <render-driver/Vulkan/FenceVulkan.h>
#include <render-driver/Vulkan/FrameBufferVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>
#include <render-driver/Vulkan/DescriptorHeapVulkan.h>
#include <render-driver/Vulkan/AccelerationStructureVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>

#include <imgui/backends/imgui_impl_vulkan.h>

#include "LogPrint.h"
#include "wtfassert.h"

#include <sstream>

#include <vulkan/vulkan_win32.h>

#if defined(_DEBUG)
#define RENDERDOC_CAPTURE 1
#endif // DEBUG



namespace Render
{
    namespace Vulkan
    {
        /*
        **
        */
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) 
        {
            DEBUG_PRINTF("validation layer: %s\n", pCallbackData->pMessage);
            return VK_FALSE;
        }

        /*
        **
        */
        void CRenderer::platformSetup(Render::Common::RendererDescriptor const& desc)
        {
            bool bRenderDoc = true;

            Render::Vulkan::RendererDescriptor const& descVulkan = static_cast<Render::Vulkan::RendererDescriptor const&>(desc);

            VkApplicationInfo appInfo = {};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "RenderWithMe";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "RenderWithMe";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_2;

            // instance extensions, NOT device extensions
            uint32_t iNumExtensions = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &iNumExtensions, nullptr);
            std::vector<VkExtensionProperties> extensions(iNumExtensions);
            vkEnumerateInstanceExtensionProperties(nullptr, &iNumExtensions, extensions.data());
            WTFASSERT(iNumExtensions <= 64, "Need larger extension buffer: %d", iNumExtensions);
            char aszExtensionNames[64][256];
            memset(aszExtensionNames, 0, sizeof(char) * 64 * 256);
            uint32_t iNumEnabledExtensions = 0;
            for(uint32_t i = 0; i < iNumExtensions; i++)
            {
                if(!strcmp(extensions[i].extensionName, "VK_KHR_portability_enumeration"))
                {
                    if(!bRenderDoc)
                    {
                        strcpy(aszExtensionNames[i], extensions[i].extensionName);
                        ++iNumEnabledExtensions;
                    }
                }
                else
                {
                    strcpy(aszExtensionNames[i], extensions[i].extensionName);
                    ++iNumEnabledExtensions;
                }
            }

            std::vector<char const*> aExtensionNames;
            uint32_t iNumValidExtensions = 0;
            for(uint32_t i = 0; i < iNumEnabledExtensions; i++)
            {
                if(aszExtensionNames[i][0] != 0)
                {
                    aExtensionNames.push_back((char const*)&aszExtensionNames[i]);
                    DEBUG_PRINTF("Vulkan Extension: %s\n",
                        aExtensionNames[iNumValidExtensions]);
                }
            }

            // layers
            uint32_t iNumLayers = 0;
            vkEnumerateInstanceLayerProperties(&iNumLayers, nullptr);
            std::vector<VkLayerProperties> aAvailableLayers(iNumLayers);
            vkEnumerateInstanceLayerProperties(&iNumLayers, aAvailableLayers.data());
            WTFASSERT(iNumLayers <= 64, "Need larger extension buffer: %d", iNumLayers);
            char aszLayerNames[64][256];
            for(uint32_t i = 0; i < iNumLayers; i++)
            {
                strcpy(aszLayerNames[i], aAvailableLayers[i].layerName);
            }
            
            std::vector<std::string> aEnableLayerKeywords;
            //aEnableLayerKeywords.push_back("validation");
#if defined(_DEBUG)
            aEnableLayerKeywords.push_back("api_dump");
#endif // _DEBUG


            DEBUG_PRINTF("\n");
            uint32_t iNumEnabledLayers = 0;
            std::vector<char const*> aLayerNames(3);
            for(uint32_t i = 0; i < iNumLayers; i++)
            {
                bool bAdd = false;
                for(auto const& layerKeyword : aEnableLayerKeywords)
                {
                    if(strstr(aszLayerNames[i], layerKeyword.c_str()))
                    {
                        bAdd = true;
                        break;
                    }
                }

                if(bAdd)
                {
                    aLayerNames[iNumEnabledLayers++] = aszLayerNames[i];
                }

                DEBUG_PRINTF("Vulkan Layer: %s\ndescription: %s\n",
                    aAvailableLayers[i].layerName,
                    aAvailableLayers[i].description);
            }

            // validation features
            VkValidationFeatureDisableEXT disables[] =
            {
                VK_VALIDATION_FEATURE_DISABLE_CORE_CHECKS_EXT
            };
            VkValidationFeaturesEXT disableFeatures = {};
            disableFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            disableFeatures.disabledValidationFeatureCount = sizeof(disables) / sizeof(*disables);
            disableFeatures.pDisabledValidationFeatures = disables;

            // debug message
            VkDebugUtilsMessengerCreateInfoEXT debugUtilCreateInfo{};
            debugUtilCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugUtilCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugUtilCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugUtilCreateInfo.pfnUserCallback = debugCallback;
            debugUtilCreateInfo.pUserData = nullptr;
            debugUtilCreateInfo.pNext = nullptr; // &disableFeatures;

            // create instance
            VkInstanceCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;
            createInfo.enabledExtensionCount = (uint32_t)aExtensionNames.size();
            createInfo.ppEnabledExtensionNames = aExtensionNames.data();
            createInfo.enabledLayerCount = iNumEnabledLayers;
            createInfo.ppEnabledLayerNames = aLayerNames.data();
            createInfo.flags = (bRenderDoc) ? 0 : VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
            createInfo.pNext = &debugUtilCreateInfo;
            VkResult ret = vkCreateInstance(&createInfo, nullptr, &mInstance);
            WTFASSERT(ret == VK_SUCCESS, "%s error creating vulkan instance", RenderDriver::Vulkan::Utils::getErrorCode(ret));

            mpPlatformInstance = &mInstance;

            RenderDriver::Common::CPhysicalDevice::Descriptor physicalDeviceDesc;
            physicalDeviceDesc.mAdapterType = RenderDriver::Common::CPhysicalDevice::AdapterType::Hardware;
            physicalDeviceDesc.mpAppInstance = &mInstance;
            mpPhysicalDevice = std::make_unique<RenderDriver::Vulkan::CPhysicalDevice>();
            mpPhysicalDevice->create(physicalDeviceDesc);

            // device extensions, !!! shit is important due to dumbasses not warning about needing this extension for swap chain creation !!!
            std::vector<char const*> aDeviceExtensions;
            aDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            aDeviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            aDeviceExtensions.push_back(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
            aDeviceExtensions.push_back(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            aDeviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
#if defined(USE_RAY_TRACING)
            aDeviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            aDeviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            aDeviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
            aDeviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
#endif // USE_RAY_TRACING
            aDeviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
            aDeviceExtensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);


            // device
            RenderDriver::Vulkan::CDevice::CreateDesc createDesc = {};
            createDesc.mpPhyiscalDevice = mpPhysicalDevice.get();
            createDesc.maLayerNames = (char* const*)aLayerNames.data();
            createDesc.miNumLayers = iNumEnabledLayers;
            createDesc.maExtensionNames = (char* const*)aDeviceExtensions.data();
            createDesc.miNumExtensions = static_cast<uint32_t>(aDeviceExtensions.size()); 
            mpDevice = std::make_unique<RenderDriver::Vulkan::CDevice>();
            mpDevice->create(createDesc);
            mpDevice->setID("Vulkan Logical Device");

            mHWND = descVulkan.mHWND;

            // swap chain
            RenderDriver::Vulkan::SwapChainDescriptor swapChainCreateDesc;
            swapChainCreateDesc.mpPhysicalDevice = mpPhysicalDevice.get();
            swapChainCreateDesc.mpDevice = mpDevice.get();
            swapChainCreateDesc.mHWND = descVulkan.mHWND;
            swapChainCreateDesc.mpInstance = &mInstance;
            swapChainCreateDesc.miWidth = desc.miScreenWidth;
            swapChainCreateDesc.miHeight = desc.miScreenHeight;
            mpSwapChain = std::make_unique<RenderDriver::Vulkan::CSwapChain>();
            mpSwapChain->create(swapChainCreateDesc, *mpDevice);

            mpComputeCommandQueue = std::make_unique<RenderDriver::Vulkan::CCommandQueue>();
            mpGraphicsCommandQueue = std::make_unique<RenderDriver::Vulkan::CCommandQueue>();
            mpCopyCommandQueue = std::make_unique<RenderDriver::Vulkan::CCommandQueue>();
            mpGPUCopyCommandQueue = std::make_unique<RenderDriver::Vulkan::CCommandQueue>();

            RenderDriver::Vulkan::CCommandQueue::CreateDesc queueCreateDesc;
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
            

#if defined(RENDERDOC_CAPTURE)
            HMODULE mod = GetModuleHandleA("renderdoc.dll");
            if(mod)
            {
                LoadLibrary(L"./renderdoc.dll");
                HMODULE mod = GetModuleHandleA("renderdoc.dll");

                pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
                int32_t iRet = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void**)&mpRenderDocAPI);
                WTFASSERT(iRet == 1, "Can\'t initialize renderdoc");
            }
#endif // RENDERDOC_CAPTURE

            {
                mpUploadCommandAllocator = std::make_unique<RenderDriver::Vulkan::CCommandAllocator>();
                mpUploadFence = std::make_unique<RenderDriver::Vulkan::CFence>();
                mpUploadCommandBuffer = std::make_unique<RenderDriver::Vulkan::CCommandBuffer>();

                RenderDriver::Common::FenceDescriptor fenceDesc = {};
                mpUploadFence->create(fenceDesc, *mpDevice);
                mpUploadFence->setID("Upload Fence");

                // upload command buffer allocator
                RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc = {};
                commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                mpUploadCommandAllocator->create(commandAllocatorDesc, *mpDevice);
                mpUploadCommandAllocator->setID("Upload Command Allocator");

                // upload command buffer
                RenderDriver::Common::CommandBufferDescriptor commandBufferDesc = {};
                commandBufferDesc.mpCommandAllocator = mpUploadCommandAllocator.get();
                commandBufferDesc.mpPipelineState = nullptr;
                commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
                mpUploadCommandBuffer->create(commandBufferDesc, *mpDevice);
                mpUploadCommandBuffer->setID("Upload Command Buffer");
                mpUploadCommandBuffer->reset();
            }

            //mpSerializer = std::make_unique<Render::Vulkan::Serializer>();
            
            vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(mInstance, "vkSetDebugUtilsObjectNameEXT"));
            vkQueueBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(mInstance, "vkQueueBeginDebugUtilsLabelEXT"));
            vkCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(mInstance, "vkCmdBeginDebugUtilsLabelEXT"));
            vkQueueEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkQueueEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(mInstance, "vkQueueEndDebugUtilsLabelEXT"));
            vkCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(mInstance, "vkCmdEndDebugUtilsLabelEXT"));
            
            

#if defined(USE_RAY_TRACING)
            //vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetInstanceProcAddr(mInstance, "vkCreateRayTracingPipelinesKHR"));
            
            vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(
                vkGetInstanceProcAddr(
                    mInstance,
                    "vkCmdTraceRaysKHR"
                )
            );

            VkPhysicalDevice& nativePhysicalDevice = *((VkPhysicalDevice*)mpPhysicalDevice->getNativeDevice());
            mRayTracingPipelineProperties = {};
            mPhysicalDeviceProperties2 = {};
            mRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
            mPhysicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
            mPhysicalDeviceProperties2.pNext = &mRayTracingPipelineProperties;
            vkGetPhysicalDeviceProperties2(
                nativePhysicalDevice, 
                &mPhysicalDeviceProperties2);

#endif // USE_RAY_TRACING

            // Create sampler to sample from the color attachments
            VkSamplerCreateInfo samplerCreateInfo = {};
            samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerCreateInfo.maxAnisotropy = 1.0f;
            samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
            samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerCreateInfo.mipLodBias = 0.0f;
            samplerCreateInfo.maxAnisotropy = 1.0f;
            samplerCreateInfo.minLod = 0.0f;
            samplerCreateInfo.maxLod = 1.0f;
            samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            VkDevice& nativeDevice = *(static_cast<VkDevice*>(mpDevice->getNativeDevice()));
            ret = vkCreateSampler(
                nativeDevice,
                &samplerCreateInfo, 
                nullptr, 
                &mNativeLinearSampler);
            WTFASSERT(ret == VK_SUCCESS, "Error creating linear sampler: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

            samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
            samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
            ret = vkCreateSampler(
                *(static_cast<VkDevice*>(mpDevice->getNativeDevice())),
                &samplerCreateInfo,
                nullptr,
                &mNativePointSampler);
            WTFASSERT(ret == VK_SUCCESS, "Error creating linear sampler: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

            maSamplers[0] = mNativePointSampler;
            maSamplers[1] = mNativeLinearSampler;

            mRenderDriverType = Render::Common::RenderDriverType::Vulkan;

            mDefaultUniformBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();

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
        }

        /*
        **
        */
        void CRenderer::transitionBarriers(
            std::vector<RenderDriver::Common::Utils::TransitionBarrierInfo> const& aBarriers)
        {
            // create graphics command buffer if needed
            if(mapQueueGraphicsCommandBuffers[0] == nullptr)
            {
                mapQueueGraphicsCommandAllocators[0] = std::make_shared<RenderDriver::Vulkan::CCommandAllocator>();
                mapQueueGraphicsCommandBuffers[0] = std::make_shared<RenderDriver::Vulkan::CCommandBuffer>();

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
                mapQueueComputeCommandAllocators[0] = std::make_shared<RenderDriver::Vulkan::CCommandAllocator>();
                mapQueueComputeCommandBuffers[0] = std::make_shared<RenderDriver::Vulkan::CCommandBuffer>();

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
                std::vector<VkImageMemoryBarrier> aImageMemoryBarriers(iNumBarrierInfo);
                for(uint32_t i = 0; i < iNumBarrierInfo; i++)
                {
                    RenderDriver::Common::Utils::TransitionBarrierInfo const& barrierInfo = aBarrierInfo[i];
                    if(barrierInfo.mpImage &&
                        barrierInfo.mAfter != RenderDriver::Common::ResourceStateFlagBits::CopyDestination &&
                        barrierInfo.mAfter != RenderDriver::Common::ResourceStateFlagBits::CopySource)
                    {
                        VkImage* pNativeImage = static_cast<VkImage*>(barrierInfo.mpImage->getNativeImage());
                        RenderDriver::Vulkan::CImage* pImageVulkan = static_cast<RenderDriver::Vulkan::CImage*>(barrierInfo.mpImage);
                        RenderDriver::Common::ImageLayout oldImageLayout = pImageVulkan->getImageLayout(iQueueType);
                        if(oldImageLayout == RenderDriver::Common::ImageLayout::UNDEFINED)
                        {
                            // memory barrier for image to transfer layout
                            VkImageMemoryBarrier imageMemoryBarrier = {};
                            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                            imageMemoryBarrier.srcAccessMask = (barrierInfo.mBefore == RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess) ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;
                            imageMemoryBarrier.dstAccessMask = (barrierInfo.mAfter == RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess || barrierInfo.mAfter == RenderDriver::Common::ResourceStateFlagBits::RenderTarget) ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;
                            imageMemoryBarrier.oldLayout = SerializeUtils::Vulkan::convert(oldImageLayout);
                            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                            imageMemoryBarrier.image = *pNativeImage;
                            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

                            pImageVulkan->setImageLayout(SerializeUtils::Vulkan::convert(imageMemoryBarrier.newLayout), iQueueType);

                            aImageMemoryBarriers[iNumImageTransitions++] = imageMemoryBarrier;
                        }
                    }
                }

                if(iNumImageTransitions > 0)
                {
                    VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(pCommandBuffer->getNativeCommandList());

                    vkCmdPipelineBarrier(
                        *pNativeCommandBuffer,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        iNumImageTransitions,
                        aImageMemoryBarriers.data());
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
                VkQueue* pNativeQueue = static_cast<VkQueue*>(mpGraphicsCommandQueue->getNativeCommandQueue());
                vkQueueWaitIdle(*pNativeQueue);
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
                VkQueue* pNativeQueue = static_cast<VkQueue*>(mpComputeCommandQueue->getNativeCommandQueue());
                vkQueueWaitIdle(*pNativeQueue);
                mapQueueComputeCommandBuffers[0]->reset();
            }
        }

        /*
        **
        */
        void CRenderer::transitionImageLayouts(
            std::vector<RenderDriver::Vulkan::Utils::ImageLayoutTransition> const& aTransitions)
        {
                // create graphics command buffer if needed
            if(mapQueueGraphicsCommandBuffers[0] == nullptr)
            {
                mapQueueGraphicsCommandAllocators[0] = std::make_shared<RenderDriver::Vulkan::CCommandAllocator>();
                mapQueueGraphicsCommandBuffers[0] = std::make_shared<RenderDriver::Vulkan::CCommandBuffer>();

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
                mapQueueComputeCommandAllocators[0] = std::make_shared<RenderDriver::Vulkan::CCommandAllocator>();
                mapQueueComputeCommandBuffers[0] = std::make_shared<RenderDriver::Vulkan::CCommandBuffer>();

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
            std::vector< VkImageMemoryBarrier> aImageMemoryBarriers(aTransitions.size());
            uint32_t iNumImageTransitions = 0;
            for(auto const& transition : aTransitions)
            {
                RenderDriver::Vulkan::CImage* pImageVulkan = static_cast<RenderDriver::Vulkan::CImage*>(transition.mpImage);
                VkImage* pNativeImage = static_cast<VkImage*>(transition.mpImage->getNativeImage());

                VkImageMemoryBarrier imageMemoryBarrier = {};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.srcAccessMask = (transition.mBefore == RenderDriver::Common::ImageLayout::SHADER_READ_ONLY_OPTIMAL) ? VK_ACCESS_SHADER_READ_BIT : VK_ACCESS_SHADER_WRITE_BIT;
                imageMemoryBarrier.dstAccessMask = (transition.mAfter == RenderDriver::Common::ImageLayout::GENERAL) ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;
                imageMemoryBarrier.oldLayout = SerializeUtils::Vulkan::convert(transition.mBefore);
                imageMemoryBarrier.newLayout = SerializeUtils::Vulkan::convert(transition.mAfter);
                imageMemoryBarrier.image = *pNativeImage;
                imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

                pImageVulkan->setImageLayout(SerializeUtils::Vulkan::convert(imageMemoryBarrier.newLayout), 0);

                aImageMemoryBarriers[iNumImageTransitions++] = imageMemoryBarrier;
            }

            VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(mapQueueGraphicsCommandBuffers[0]->getNativeCommandList());

            vkCmdPipelineBarrier(
                *pNativeCommandBuffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                iNumImageTransitions,
                aImageMemoryBarriers.data());

            mapQueueGraphicsCommandBuffers[0]->close();
            mpGraphicsCommandQueue->execCommandBuffer(
                *mapQueueGraphicsCommandBuffers[0].get(),
                *mpDevice);
            VkQueue* pNativeQueue = static_cast<VkQueue*>(mpGraphicsCommandQueue->getNativeCommandQueue());
            vkQueueWaitIdle(*pNativeQueue);
            mapQueueGraphicsCommandBuffers[0]->reset();
        }

        /*
        **
        */
        void CRenderer::updateImageDescriptors(
            RenderDriver::Common::CDescriptorSet* pDescriptorSet,
            std::vector<VkDescriptorImageInfo> const& aImageInfo,
            std::vector<std::pair<uint32_t, uint32_t>> const& aBindingIndices,
            std::vector<VkDescriptorType> const& aDescriptorTypes)
        {
            RenderDriver::Vulkan::CDescriptorSet* pDescriptorSetVulkan = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(pDescriptorSet);

            std::vector<VkDescriptorSet> const& aNativeDescriptorSets = pDescriptorSetVulkan->getNativeDescriptorSets();

            // update the output texture descriptors
            VkDevice& nativeDevice = *(static_cast<VkDevice*>(mpDevice->getNativeDevice()));
            for(uint32_t i = 0; i < static_cast<uint32_t>(aImageInfo.size()); i++)
            {
                uint32_t iSet = aBindingIndices[i].first;
                uint32_t iBinding = aBindingIndices[i].second;

                VkWriteDescriptorSet descriptorWrite = {};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = aNativeDescriptorSets[iSet];
                descriptorWrite.dstBinding = iBinding;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = aDescriptorTypes[i];
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = &aImageInfo[i];

                vkUpdateDescriptorSets(
                    nativeDevice,
                    1,
                    &descriptorWrite,
                    0,
                    nullptr);
            }
        }

        /*
        **
        */
        void CRenderer::initImgui()
        {
            uint32_t const kiNumTotalAttachments = 2;

            // render pass
            // color attachments
            VkImageLayout finalImageLayout = VK_IMAGE_LAYOUT_GENERAL;
            std::vector<VkAttachmentDescription> aColorAttachments(kiNumTotalAttachments);
            for(uint32_t iRenderTarget = 0; iRenderTarget < 1; iRenderTarget++)
            {
                aColorAttachments[iRenderTarget].format = VK_FORMAT_B8G8R8A8_UNORM;
                aColorAttachments[iRenderTarget].samples = VK_SAMPLE_COUNT_1_BIT;
                aColorAttachments[iRenderTarget].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                aColorAttachments[iRenderTarget].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                aColorAttachments[iRenderTarget].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                aColorAttachments[iRenderTarget].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                aColorAttachments[iRenderTarget].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                aColorAttachments[iRenderTarget].finalLayout = finalImageLayout;
            }

            // depth attachments
            uint32_t iDepthAttachmentIndex = 1;
            {
                aColorAttachments[iDepthAttachmentIndex].format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                aColorAttachments[iDepthAttachmentIndex].samples = VK_SAMPLE_COUNT_1_BIT;
                aColorAttachments[iDepthAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                aColorAttachments[iDepthAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                aColorAttachments[iDepthAttachmentIndex].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                aColorAttachments[iDepthAttachmentIndex].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                aColorAttachments[iDepthAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                aColorAttachments[iDepthAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            std::vector<VkAttachmentReference> aColorAttachmentRef(kiNumTotalAttachments);
            for(uint32_t i = 0; i < kiNumTotalAttachments; i++)
            {
                aColorAttachmentRef[i].attachment = i;
                aColorAttachmentRef[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }

            VkAttachmentReference depthAttachmentRef = {};
            {
                depthAttachmentRef.attachment = iDepthAttachmentIndex;
                depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = aColorAttachmentRef.data();
            subpass.pDepthStencilAttachment = &depthAttachmentRef;

            VkSubpassDependency aSubPassDependencies[kiNumTotalAttachments] = {};
            aSubPassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            aSubPassDependencies[0].dstSubpass = 0;
            aSubPassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            aSubPassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            aSubPassDependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            aSubPassDependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

            aSubPassDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
            aSubPassDependencies[1].dstSubpass = 0;
            aSubPassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            aSubPassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            aSubPassDependencies[1].srcAccessMask = 0;
            aSubPassDependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = kiNumTotalAttachments;
            renderPassInfo.pAttachments = aColorAttachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = sizeof(aSubPassDependencies) / sizeof(*aSubPassDependencies);
            renderPassInfo.pDependencies = aSubPassDependencies;
            VkResult ret = vkCreateRenderPass(
                *(static_cast<VkDevice*>(mpDevice->getNativeDevice())),
                &renderPassInfo,
                nullptr,
                &mImguiRenderPass);
            WTFASSERT(ret == VK_SUCCESS, "Error creating render pass: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = mInstance;
            init_info.PhysicalDevice = *(static_cast<VkPhysicalDevice*>(mpPhysicalDevice->getNativeDevice()));
            init_info.Device = *(static_cast<VkDevice*>(mpDevice->getNativeDevice()));
            init_info.Queue = *(static_cast<VkQueue*>(mpGraphicsCommandQueue->getNativeCommandQueue()));
            
            init_info.MinImageCount = 2;
            init_info.ImageCount = 2;
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 128;
            pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;
            ret = vkCreateDescriptorPool(init_info.Device, &pool_info, nullptr, &mImguiDescriptorPool);
            WTFASSERT(ret == VK_SUCCESS, "Error creating imgui descriptor pool: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

            init_info.DescriptorPool = mImguiDescriptorPool;

            ImGui_ImplVulkan_Init(&init_info, mImguiRenderPass);

            std::unique_ptr<RenderDriver::Vulkan::CCommandAllocator> commandAllocator = std::make_unique<RenderDriver::Vulkan::CCommandAllocator>();
            RenderDriver::Common::CommandAllocatorDescriptor graphicsCommandAllocatorDesc;
            graphicsCommandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
            commandAllocator->create(graphicsCommandAllocatorDesc, *mpDevice);
            commandAllocator->reset();

            std::unique_ptr<RenderDriver::Vulkan::CCommandBuffer> commandBuffer = std::make_unique<RenderDriver::Vulkan::CCommandBuffer>();
            RenderDriver::Common::CommandBufferDescriptor graphicsCommandBufferDesc;
            graphicsCommandBufferDesc.mpCommandAllocator = commandAllocator.get();
            graphicsCommandBufferDesc.mpPipelineState = nullptr;
            graphicsCommandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
            commandBuffer->create(graphicsCommandBufferDesc, *mpDevice);
            commandBuffer->reset();

            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.Fonts->Build();

            VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(commandBuffer->getNativeCommandList());
            ImGui_ImplVulkan_CreateFontsTexture(*pNativeCommandBuffer);
            commandBuffer->close();
            mpGraphicsCommandQueue->execCommandBuffer(*commandBuffer, *mpDevice);
            vkQueueWaitIdle(*(static_cast<VkQueue*>(mpGraphicsCommandQueue->getNativeCommandQueue())));
            commandBuffer->reset();
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

            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::Vulkan::CBuffer>());

            RenderDriver::Vulkan::CBuffer& uploadBuffer = static_cast<RenderDriver::Vulkan::CBuffer&>(*mapUploadBuffers.back());
            RenderDriver::Common::BufferDescriptor uploadBufferDesc = {};

            uploadBufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
            uploadBufferDesc.miSize = iDataSize;
            uploadBuffer.create(uploadBufferDesc, *mpDevice);
            uploadBuffer.setID("UploadBuffer");

            platformCopyCPUToGPUBuffer(
                *mpUploadCommandBuffer,
                &buffer,
                &uploadBuffer,
                pRawSrcData,
                0,
                static_cast<uint32_t>(iDestDataOffset),
                static_cast<uint32_t>(iDataSize));
        }

        /*
        **
        */
        void CRenderer::platformSetComputeDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));
            VkPipelineLayout& nativePipelineLayout = *(static_cast<VkPipelineLayout*>(static_cast<RenderDriver::Vulkan::CPipelineState&>(pipelineState).getNativePipelineLayout()));

            RenderDriver::Vulkan::CDescriptorSet& descriptorSetVulkan = static_cast<RenderDriver::Vulkan::CDescriptorSet&>(descriptorSet);
            std::vector<VkDescriptorSet>& aNativeDescriptorSets = descriptorSetVulkan.getNativeDescriptorSets();

            vkCmdBindDescriptorSets(
                nativeCommandBuffer,
                VK_PIPELINE_BIND_POINT_COMPUTE,
                nativePipelineLayout,
                0,
                static_cast<uint32_t>(aNativeDescriptorSets.size()),
                aNativeDescriptorSets.data(),
                0,
                nullptr);
        }

        /*
        **
        */
        void CRenderer::platformSetRayTraceDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));
            VkPipelineLayout& nativePipelineLayout = *(static_cast<VkPipelineLayout*>(static_cast<RenderDriver::Vulkan::CPipelineState&>(pipelineState).getNativePipelineLayout()));

            RenderDriver::Vulkan::CDescriptorSet& descriptorSetVulkan = static_cast<RenderDriver::Vulkan::CDescriptorSet&>(descriptorSet);
            std::vector<VkDescriptorSet>& aNativeDescriptorSets = descriptorSetVulkan.getNativeDescriptorSets();

            vkCmdBindDescriptorSets(
                nativeCommandBuffer,
                VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                nativePipelineLayout,
                0,
                static_cast<uint32_t>(aNativeDescriptorSets.size()),
                aNativeDescriptorSets.data(),
                0,
                nullptr);
        }

        /*
        **
        */
        void CRenderer::platformSetGraphicsDescriptorSet(
            RenderDriver::Common::CDescriptorSet& descriptorSet,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CPipelineState& pipelineState)
        {
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));
            VkPipelineLayout& nativePipelineLayout = *(static_cast<VkPipelineLayout*>(static_cast<RenderDriver::Vulkan::CPipelineState&>(pipelineState).getNativePipelineLayout()));

            RenderDriver::Vulkan::CDescriptorSet& descriptorSetVulkan = static_cast<RenderDriver::Vulkan::CDescriptorSet&>(descriptorSet);
            std::vector<VkDescriptorSet>& aNativeDescriptorSets = descriptorSetVulkan.getNativeDescriptorSets();

            vkCmdBindDescriptorSets(
                nativeCommandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                nativePipelineLayout,
                0,
                static_cast<uint32_t>(aNativeDescriptorSets.size()),
                aNativeDescriptorSets.data(),
                0,
                nullptr);
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
            RenderDriver::Vulkan::CPipelineState& pipelineStateVulkan = static_cast<RenderDriver::Vulkan::CPipelineState&>(pipelineState);
            VkPipelineLayout* pNativeLayout = static_cast<VkPipelineLayout*>(pipelineStateVulkan.getNativePipelineLayout());

            vkCmdPushConstants(
                *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList())),
                *pNativeLayout,
                VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                iOffsetIn32Bits,
                sizeof(float) * iNumValues,
                paValues);
        }

        /*
        **
        */
        void CRenderer::platformSetDescriptorHeap(
            RenderDriver::Common::CDescriptorHeap& descriptorHeap,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            // no descriptor heap, the descriptor pool is already associated with the descriptor set in the pipeline
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
           // no shader resource views, already been set with descriptor set in the pipeline
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
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));

            VkViewport viewport = {};
            viewport.x = 0.0f;
            viewport.y = static_cast<float>(iHeight);
            viewport.width = static_cast<float>(iWidth);
            viewport.height = static_cast<float>(iHeight) * -1.0f;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = fMaxDepth;
            vkCmdSetViewport(
                nativeCommandBuffer, 
                0, 
                1, 
                &viewport);

            VkRect2D scissor = {};
            scissor.offset = { 0, 0 };
            scissor.extent.width = iWidth;
            scissor.extent.height = iHeight;
            vkCmdSetScissor(
                nativeCommandBuffer, 
                0, 
                1, 
                &scissor);
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
            //WTFASSERT(0, "Implement me");
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
            if(mpReadBackBuffer == nullptr)
            {
                mpReadBackBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
                RenderDriver::Common::BufferDescriptor desc;
                desc.miSize = iDataSize;
                desc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
                desc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferSrc;
                mpReadBackBuffer->create(
                    desc,
                    *mpDevice
                );
            }

            VkCommandBuffer* pNativeUploadCommandBuffer = static_cast<VkCommandBuffer*>(mpUploadCommandBuffer->getNativeCommandList());
            VkBuffer* pNativeBufferSrc = static_cast<VkBuffer*>(pGPUBuffer->getNativeBuffer());
            VkBuffer* pNativeBufferDest = static_cast<VkBuffer*>(mpReadBackBuffer->getNativeBuffer());
            VkDevice* pNativeDevice = static_cast<VkDevice*>(mpDevice->getNativeDevice());

            VkDeviceMemory* pNativeMemorySrc = static_cast<VkDeviceMemory*>(static_cast<RenderDriver::Vulkan::CBuffer*>(mpReadBackBuffer.get())->getNativeDeviceMemory());

            vkEndCommandBuffer(*pNativeUploadCommandBuffer);
            vkResetCommandBuffer(*pNativeUploadCommandBuffer, 0);

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.pNext = nullptr;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;
            vkBeginCommandBuffer(
                *pNativeUploadCommandBuffer,
                &beginInfo);

            VkBufferCopy region;
            region.srcOffset = iSrcOffset;
            region.dstOffset = 0;
            region.size = iDataSize;

            vkCmdCopyBuffer(
                *pNativeUploadCommandBuffer,
                *pNativeBufferSrc,
                *pNativeBufferDest,
                1,
                &region);
            
            vkEndCommandBuffer(*pNativeUploadCommandBuffer);

            VkQueue* pNativeCommandQueue = static_cast<VkQueue*>(mpCopyCommandQueue->getNativeCommandQueue());
            //VkSubmitInfo submitInfo = {};
            //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            //submitInfo.pNext = nullptr;
            //submitInfo.waitSemaphoreCount = 0;
            //submitInfo.pWaitSemaphores = nullptr;
            //submitInfo.pWaitDstStageMask = nullptr;
            //submitInfo.commandBufferCount = 1;
            //submitInfo.pCommandBuffers = pNativeUploadCommandBuffer;
            //submitInfo.signalSemaphoreCount = 0;
            //submitInfo.pSignalSemaphores = nullptr;
            //
            //vkQueueSubmit(
            //    *pNativeCommandQueue,
            //    1,
            //    &submitInfo,
            //    nullptr);

            mpCopyCommandQueue->execCommandBufferSynchronized(
                *mpUploadCommandBuffer, 
                *mpDevice,
                true);

            //vkQueueWaitIdle(*pNativeCommandQueue);

            void* pData = nullptr;
            vkMapMemory(
                *pNativeDevice,
                *pNativeMemorySrc,
                0,
                iDataSize,
                0,
                &pData);
            memcpy(pCPUBuffer, pData, iDataSize);
            vkUnmapMemory(
                *pNativeDevice,
                *pNativeMemorySrc);

            mpUploadCommandBuffer->reset();
        
            mpReadBackBuffer->releaseNativeBuffer();
            mpReadBackBuffer.reset();
            mpReadBackBuffer = nullptr;
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
            // native stuff
            VkCommandBuffer* pNativeUploadCommandBuffer = static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList());
            VkBuffer* pNativeBufferSrc = static_cast<VkBuffer*>(pGPUBuffer->getNativeBuffer());
            
            VkDevice* pNativeDevice = static_cast<VkDevice*>(mpDevice->getNativeDevice());
            VkQueue* pNativeCommandQueue = static_cast<VkQueue*>(mpCopyCommandQueue->getNativeCommandQueue());

            RenderDriver::Vulkan::CBuffer* pVulkanBuffer = (RenderDriver::Vulkan::CBuffer*)pGPUBuffer;
            VkDeviceMemory* pNativeMemorySrc = static_cast<VkDeviceMemory*>(pVulkanBuffer->getNativeDeviceMemory());


#if 0
            // allocate memory for buffer
            std::unique_ptr<RenderDriver::Vulkan::CBuffer> readBackBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            RenderDriver::Common::BufferDescriptor bufferDesc = {};
            bufferDesc.miSize = iDataSize;
            bufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            bufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferDest;
            readBackBuffer->create(bufferDesc, *mpDevice);

            commandBuffer.reset();

            VkBuffer* pNativeBufferDest = static_cast<VkBuffer*>(readBackBuffer->getNativeBuffer());
            VkDeviceMemory* pNativeMemorySrc = static_cast<VkDeviceMemory*>(readBackBuffer->getNativeDeviceMemory());

            // copy 
            VkBufferCopy region;
            region.srcOffset = iSrcOffset;
            region.dstOffset = 0;
            region.size = iDataSize;
            vkCmdCopyBuffer(
                *pNativeUploadCommandBuffer,
                *pNativeBufferSrc,
                *pNativeBufferDest,
                1,
                &region);

            commandBuffer.close();

            commandQueue.execCommandBufferSynchronized(
                commandBuffer,
                *mpDevice
            );
#endif // #if 0

            // memcpy from the buffer
            void* pData = nullptr;
            vkMapMemory(
                *pNativeDevice,
                *pNativeMemorySrc,
                0,
                iDataSize,
                0,
                &pData);
            memcpy(pCPUBuffer, pData, iDataSize);
            vkUnmapMemory(
                *pNativeDevice,
                *pNativeMemorySrc);

#if 0
            commandBuffer.reset();

            // cleanup
            readBackBuffer->releaseNativeBuffer();
            readBackBuffer.reset();
            readBackBuffer = nullptr;
#endif // #if 0
        }

        /*
        **
        */
        void CRenderer::platformCopyBufferToCPUMemory3(
            RenderDriver::Common::CBuffer* pGPUBuffer,
            void* pCPUBuffer,
            uint64_t iSrcOffset,
            uint64_t iDataSize,
            RenderDriver::Common::CBuffer& readBackBuffer,
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
            RenderDriver::Vulkan::CDevice* pDeviceVulkan = static_cast<RenderDriver::Vulkan::CDevice*>(mpDevice.get());

            RenderDriver::Vulkan::CImage& imageVulkan = static_cast<RenderDriver::Vulkan::CImage&>(image);

            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::Vulkan::CBuffer>());
            RenderDriver::Vulkan::CBuffer& uploadBuffer = static_cast<RenderDriver::Vulkan::CBuffer&>(*mapUploadBuffers.back());

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

            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0; //  iTextureArrayIndex;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = iImageWidth;
            bufferCopyRegion.imageExtent.height = iImageHeight;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = 0;
            VkCommandBuffer& nativeUploadCommandBufferVulkan = *(static_cast<VkCommandBuffer*>(mpUploadCommandBuffer->getNativeCommandList()));
            VkBuffer& nativeUploadBufferVulkan = *(static_cast<VkBuffer*>(uploadBuffer.getNativeBuffer()));
            VkImage& nativeImage = *(static_cast<VkImage*>(image.getNativeImage()));
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            RenderDriver::Common::CommandBufferState const& commandBufferState = mpUploadCommandBuffer->getState();
            if(commandBufferState == RenderDriver::Common::CommandBufferState::Closed)
            {
                mpUploadCommandBuffer->reset();
            }
                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = nativeImage;
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                barrier.subresourceRange.baseMipLevel = 0;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = 1;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                vkCmdPipelineBarrier(
                    nativeUploadCommandBufferVulkan,
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrier);

                vkCmdCopyBufferToImage(
                    nativeUploadCommandBufferVulkan,
                    nativeUploadBufferVulkan,
                    nativeImage,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &bufferCopyRegion);

            mpUploadCommandBuffer->close();

            RenderDriver::Vulkan::CCommandQueue* pCopyCommandQueueVulkan = static_cast<RenderDriver::Vulkan::CCommandQueue*>(mpCopyCommandQueue.get());

            VkDevice* pNativeDevice = static_cast<VkDevice*>(mpDevice->getNativeDevice());
            //VkSemaphore* pSemaphore = pCopyCommandQueueVulkan->getNativeSignalSemaphore();
            
            RenderDriver::Vulkan::CFence* pUploadFenceVulkan = static_cast<RenderDriver::Vulkan::CFence*>(mpUploadFence.get());
            VkSemaphore* pSemaphore = pUploadFenceVulkan->getNativeSemaphore();

            ++miCopyCommandFenceValue;
            uint64_t iCopyCommandFenceValue = UINT64_MAX;
            if(*pSemaphore)
            {
                vkGetSemaphoreCounterValue(
                    *pNativeDevice,
                    *pSemaphore,
                    &iCopyCommandFenceValue);
                ++iCopyCommandFenceValue;
                miCopyCommandFenceValue = iCopyCommandFenceValue;
            }
            else
            {
                iCopyCommandFenceValue = miCopyCommandFenceValue;
            }

            uint32_t iNumSignalSemaphores = 0;
            pCopyCommandQueueVulkan->signal(mpUploadFence.get(), iCopyCommandFenceValue);
            VkSemaphore* aQueueSemaphores = pCopyCommandQueueVulkan->getNativeSignalSemaphores(iNumSignalSemaphores);
            mpCopyCommandQueue->execCommandBuffer(
                *mpUploadCommandBuffer, 
                *mpDevice);
            
            uint64_t* aiWaitValues = pCopyCommandQueueVulkan->getSignalValues();

            VkSemaphoreWaitInfo waitInfo;
            waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            waitInfo.pNext = NULL;
            waitInfo.flags = 0;
            waitInfo.semaphoreCount = iNumSignalSemaphores;
            waitInfo.pSemaphores = aQueueSemaphores;
            waitInfo.pValues = aiWaitValues;

            VkResult ret = vkWaitSemaphores(
                *pNativeDevice,
                &waitInfo,
                UINT64_MAX);
            WTFASSERT(ret == VK_SUCCESS, "Error waiting on signaled semaphore: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

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
            uint32_t iImageWidth = srcImage.getDescriptor().miWidth;
            uint32_t iImageHeight = srcImage.getDescriptor().miHeight;

            VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList());

            VkImage* pNativeImageSrc = static_cast<VkImage*>(srcImage.getNativeImage());
            VkImage* pNativeImageDest = static_cast<VkImage*>(destImage.getNativeImage());

            uint32_t iQueueType = static_cast<uint32_t>(RenderDriver::Common::CCommandQueue::Type::Copy);
            RenderDriver::Common::ImageLayout const& oldLayoutSrc = static_cast<RenderDriver::Vulkan::CImage&>(srcImage).getImageLayout(static_cast<uint32_t>(iQueueType));
            VkImageLayout oldLayoutSrcVulkan = SerializeUtils::Vulkan::convert(oldLayoutSrc);

            // memory barrier for image to transfer layout
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.oldLayout = oldLayoutSrcVulkan;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.image = *pNativeImageSrc;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                *pNativeCommandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, 
                nullptr,
                0, 
                nullptr,
                1, 
                &imageMemoryBarrier);

            RenderDriver::Common::ImageLayout const& oldLayoutDest = static_cast<RenderDriver::Vulkan::CImage&>(destImage).getImageLayout(iQueueType);
            VkImageLayout oldLayoutDestVulkan = SerializeUtils::Vulkan::convert(oldLayoutDest);

            // memory barrier for image to transfer dest layout
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.oldLayout = oldLayoutDestVulkan;
            imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.image = *pNativeImageDest;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                *pNativeCommandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

            // copy image
            VkImageCopy copyRegion = {};

            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcOffset = { 0, 0, 0 };

            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.baseArrayLayer = 0;
            copyRegion.dstSubresource.mipLevel = 0;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.dstOffset = { 0, 0, 0 };

            copyRegion.extent.width = static_cast<uint32_t>(iImageWidth);
            copyRegion.extent.height = static_cast<uint32_t>(iImageHeight);
            copyRegion.extent.depth = 1;

            vkCmdCopyImage(
                *pNativeCommandBuffer,
                *pNativeImageSrc,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                *pNativeImageDest,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &copyRegion);

            // memory barrier for image back to general layout
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            imageMemoryBarrier.newLayout = oldLayoutSrcVulkan;
            imageMemoryBarrier.image = *pNativeImageSrc;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                *pNativeCommandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

            static_cast<RenderDriver::Vulkan::CImage&>(srcImage).setImageLayout(
                oldLayoutSrc,
                iQueueType);

            // memory barrier for image back to general layout
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageMemoryBarrier.newLayout = oldLayoutDestVulkan;
            imageMemoryBarrier.image = *pNativeImageDest;
            imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

            vkCmdPipelineBarrier(
                *pNativeCommandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &imageMemoryBarrier);

            static_cast<RenderDriver::Vulkan::CImage&>(destImage).setImageLayout(
                oldLayoutDest,
                iQueueType);
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
        void CRenderer::platformBeginRenderDebuggerCapture(std::string const& captureFilePath)
        {
            size_t charsNeeded = ::MultiByteToWideChar(
                CP_UTF8, 
                0,
                captureFilePath.data(), 
                (int)captureFilePath.size(), 
                NULL, 
                0);

            std::vector<wchar_t> buffer(charsNeeded);
            int charsConverted = ::MultiByteToWideChar(
                CP_UTF8, 
                0,
                captureFilePath.data(), 
                (int)captureFilePath.size(), 
                &buffer[0], 
                static_cast<int32_t>(buffer.size()));
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
            VkInstance& instance = *(VkInstance*)mpPlatformInstance;

            mpRenderDocAPI->SetCaptureFilePathTemplate(filePath.c_str());
            mpRenderDocAPI->StartFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(instance), mHWND);

            miStartCaptureFrame = miFrameIndex;
        }

        /*
        **
        */
        void CRenderer::platformEndRenderDocCapture()
        {
            VkInstance& instance = *(VkInstance*)mpPlatformInstance;
            mpRenderDocAPI->EndFrameCapture(RENDERDOC_DEVICEPOINTER_FROM_VKINSTANCE(instance), mHWND);

            uint32_t iNumCaptures = mpRenderDocAPI->GetNumCaptures();
            DEBUG_PRINTF("captured %d frames\n", iNumCaptures);
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
                mapCommandQueueFences[iFence] = std::make_unique<RenderDriver::Vulkan::CFence>();
                RenderDriver::Common::FenceDescriptor desc;
                mapCommandQueueFences[iFence]->create(desc, *mpDevice);
                mapCommandQueueFences[iFence]->setID(aFenceNames[iFence]);
            }
        }

        /*
        **
        */
        void CRenderer::platformPostSetup(std::map<std::string, std::unique_ptr<RenderDriver::Common::CBuffer>>& aExternalBufferMap)
        {
            for(uint32_t i = 0; i < static_cast<uint32_t>(mapQueueGraphicsCommandAllocators.size()); i++)
            {
                if(mapQueueGraphicsCommandAllocators[i] == nullptr)
                {
                    // graphics
                    mapQueueGraphicsCommandAllocators[i] = std::make_shared<RenderDriver::Vulkan::CCommandAllocator>();
                    mapQueueGraphicsCommandBuffers[i] = std::make_shared<RenderDriver::Vulkan::CCommandBuffer>();

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
                    mapQueueComputeCommandAllocators[i] = std::make_shared<RenderDriver::Vulkan::CCommandAllocator>();
                    mapQueueComputeCommandBuffers[i] = std::make_shared<RenderDriver::Vulkan::CCommandBuffer>();

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
                    mapQueueCopyCommandAllocators[i] = std::make_shared<RenderDriver::Vulkan::CCommandAllocator>();
                    mapQueueCopyCommandBuffers[i] = std::make_shared<RenderDriver::Vulkan::CCommandBuffer>();

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
                    mapQueueGPUCopyCommandAllocators[i] = std::make_shared<RenderDriver::Vulkan::CCommandAllocator>();
                    mapQueueGPUCopyCommandBuffers[i] = std::make_shared<RenderDriver::Vulkan::CCommandBuffer>();

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

#if 0
            for(auto& renderJobKeyValue : maRenderJobs)
            {
                RenderDriver::Common::CCommandQueue* pGraphicsCommandQueue = mpGraphicsCommandQueue.get();
                RenderDriver::Common::CCommandQueue* pComputeCommandQueue = mpComputeCommandQueue.get();

                VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(mapRenderJobCommandBuffers[renderJobKeyValue.first]->getNativeCommandList());
                for(auto& outputAttachmentKeyValue : renderJobKeyValue.second->mapOutputImageAttachments)
                {
                    if(outputAttachmentKeyValue.second == nullptr)
                    {
                        continue;
                    }

                    VkResult ret = vkResetCommandBuffer(*pNativeCommandBuffer, 0);
                    WTFASSERT(ret == VK_SUCCESS, "Error resetting command buffer: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = 0;
                    beginInfo.pInheritanceInfo = nullptr;
                    ret = vkBeginCommandBuffer(*pNativeCommandBuffer, &beginInfo);
                    WTFASSERT(ret == VK_SUCCESS, "Error starting command buffer: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

                    RenderDriver::Vulkan::CImage* pImageVulkan = (RenderDriver::Vulkan::CImage*)outputAttachmentKeyValue.second;
                    VkImage& nativeImage = *((VkImage*)outputAttachmentKeyValue.second->getNativeImage());
                    bool bIsDepthStencil = (pImageVulkan->getFormat() == RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT);

                    VkImageLayout oldLayoutVulkan = VK_IMAGE_LAYOUT_UNDEFINED;
                    VkImageLayout newLayoutVulkan = (bIsDepthStencil) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                    // memory barrier for image to transfer layout
                    VkImageMemoryBarrier imageMemoryBarrier = {};
                    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    imageMemoryBarrier.oldLayout = oldLayoutVulkan;
                    imageMemoryBarrier.newLayout = newLayoutVulkan;
                    imageMemoryBarrier.image = nativeImage;
                    imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                    if(oldLayoutVulkan == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                        newLayoutVulkan == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                        bIsDepthStencil)
                    {
                        imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
                    }

                    pImageVulkan->setImageLayout(SerializeUtils::Vulkan::convert(imageMemoryBarrier.newLayout), 0);

                    vkCmdPipelineBarrier(
                        *pNativeCommandBuffer,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &imageMemoryBarrier);

                    ret = vkEndCommandBuffer(*pNativeCommandBuffer);
                    WTFASSERT(ret == VK_SUCCESS, "Error closing command buffer: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

                    VkQueue& nativeQueue = *((VkQueue*)mpGraphicsCommandQueue->getNativeCommandQueue());
                    // VkSubmitInfo submitInfo = {};
                    // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    // submitInfo.pNext = nullptr;
                    // submitInfo.commandBufferCount = 1;
                    // submitInfo.pCommandBuffers = pNativeCommandBuffer;
                    // submitInfo.waitSemaphoreCount = 0;
                    // submitInfo.pWaitSemaphores = nullptr;
                    // submitInfo.pWaitDstStageMask = nullptr;
                    // submitInfo.signalSemaphoreCount = 0;
                    // submitInfo.pSignalSemaphores = nullptr;
                    // ret = vkQueueSubmit(
                    //    nativeQueue,
                    //    1,
                    //    &submitInfo,
                    //    VK_NULL_HANDLE);
                    //WTFASSERT(ret == VK_SUCCESS, "Error submitting command buffer: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));
                    
                    mpGraphicsCommandQueue->execCommandBufferSynchronized(
                        *mapRenderJobCommandBuffers[renderJobKeyValue.first],
                        *mpDevice
                    );

                    //vkQueueWaitIdle(nativeQueue);
                }
            }
#endif // #if 0
        }

        /*
        **
        */
        void CRenderer::platformSwapChainMoveToNextFrame()
        {
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
            uint32_t iFlag)
        {
            pUploadBuffer->setData(pCPUData, iDataSize);
            pDestBuffer->copy(
                *pUploadBuffer,
                commandBuffer,
                iDestOffset,
                iSrcOffset,
                iDataSize);
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
            //std::unique_ptr<RenderDriver::Vulkan::CBuffer> uploadBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            //RenderDriver::Common::BufferDescriptor bufferDesc = {};
            //bufferDesc.miSize = iDataSize;
            //bufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage::TransferSrc;
            //uploadBuffer->create(
            //    bufferDesc, 
            //    *mpDevice);
            //
            
            WTFASSERT(iDataSize < uploadBuffer.getDescriptor().miSize, "Copy size exceeds %d", uploadBuffer.getDescriptor().miSize);
            
            commandBuffer.reset();
            uploadBuffer.setData(pCPUData, iDataSize);
            
           
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));
            VkBuffer& nativeSrcBuffer = *(static_cast<VkBuffer*>(uploadBuffer.getNativeBuffer()));

            VkBufferCopy region;
            region.dstOffset = iDestOffset;
            region.size = iDataSize;
            region.srcOffset = iSrcOffset;
            vkCmdCopyBuffer(
                nativeCommandBuffer,
                nativeSrcBuffer,
                *((VkBuffer *)pDestBuffer->getNativeBuffer()),
                1,
                &region);

            commandBuffer.close();

            commandQueue.execCommandBufferSynchronized(
                commandBuffer,
                *mpDevice
            );

            //uploadBuffer->releaseNativeBuffer();
            //uploadBuffer.reset();
            //uploadBuffer = nullptr;
        }

        /*
        **
        */
        void CRenderer::platformExecuteCopyCommandBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iFlag)
        {
            commandBuffer.close();
            
            RenderDriver::Vulkan::CCommandQueue* pCopyCommandQueueVulkan =
                static_cast<RenderDriver::Vulkan::CCommandQueue*>(mpCopyCommandQueue.get());
            VkDevice* pNativeDevice = static_cast<VkDevice*>(mpDevice->getNativeDevice());

            RenderDriver::Vulkan::CFence* pUploadFenceVulkan = static_cast<RenderDriver::Vulkan::CFence*>(mpUploadFence.get());
            VkSemaphore* pUploadFenceSemaphore = static_cast<VkSemaphore*>(pUploadFenceVulkan->getNativeSemaphore());

            uint64_t iCopyCommandFenceValue = UINT64_MAX;
            vkGetSemaphoreCounterValue(
                *pNativeDevice,
                *pUploadFenceSemaphore,
                &iCopyCommandFenceValue);

            iCopyCommandFenceValue = (iCopyCommandFenceValue > miCopyCommandFenceValue) ? iCopyCommandFenceValue : miCopyCommandFenceValue;

            ++iCopyCommandFenceValue;
            pCopyCommandQueueVulkan->signal(mpUploadFence.get(), iCopyCommandFenceValue);
            uint32_t iNumSignalSemaphores = 0;
            VkSemaphore* aQueueSemaphores = pCopyCommandQueueVulkan->getNativeSignalSemaphores(iNumSignalSemaphores);
            mpCopyCommandQueue->execCommandBuffer(
                *mpUploadCommandBuffer,
                *mpDevice);
            
            uint64_t* aiWaitValues = pCopyCommandQueueVulkan->getSignalValues();

            if((iFlag & static_cast<uint32_t>(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION)) > 0)
            {
                //vkQueueWaitIdle(*(static_cast<VkQueue*>(mpCopyCommandQueue->getNativeCommandQueue())));

                VkQueue& nativeCommandQueue = *(VkQueue*)pCopyCommandQueueVulkan->getNativeCommandQueue();

                //VkSemaphoreWaitInfo waitInfo;
                //waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
                //waitInfo.pNext = NULL;
                //waitInfo.flags = 0;
                //waitInfo.semaphoreCount = iNumSignalSemaphores;
                //waitInfo.pSemaphores = aQueueSemaphores;
                //waitInfo.pValues = aiWaitValues;
                //
                //VkResult ret = vkWaitSemaphores(
                //    *pNativeDevice,
                //    &waitInfo,
                //    UINT64_MAX);
                //WTFASSERT(ret == VK_SUCCESS, "Error waiting on signaled semaphore: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

                vkQueueWaitIdle(nativeCommandQueue);

                miCopyCommandFenceValue = iCopyCommandFenceValue;
            }
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker(
            std::string const& name,
            RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
            if(pCommandQueue)
            {
                VkDebugUtilsLabelEXT labelInfo;
                labelInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
                labelInfo.pNext = nullptr;
                labelInfo.pLabelName = name.c_str();
                labelInfo.color[0] = 0.0f; labelInfo.color[1] = 1.0f; labelInfo.color[2] = 0.0f; labelInfo.color[3] = 1.0f;
                VkQueue* pNativeQueue = static_cast<VkQueue*>(pCommandQueue->getNativeCommandQueue());
                vkQueueBeginDebugUtilsLabelEXT(*pNativeQueue, &labelInfo);
            }
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker2(
            std::string const& name,
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            VkDebugUtilsLabelEXT markerInfo = {};
            markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            markerInfo.pLabelName = name.c_str();
            markerInfo.color[0] = 0.0f; markerInfo.color[1] = 1.0f; markerInfo.color[2] = 0.0f; markerInfo.color[3] = 1.0f;
            VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(pCommandBuffer->getNativeCommandList());
            vkCmdBeginDebugUtilsLabelEXT(*pNativeCommandBuffer, &markerInfo);
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker(
            RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
            if(pCommandQueue)
            {
                VkQueue* pNativeQueue = static_cast<VkQueue*>(pCommandQueue->getNativeCommandQueue());
                vkQueueEndDebugUtilsLabelEXT(*pNativeQueue);
            }
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker2(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(pCommandBuffer->getNativeCommandList());
            vkCmdEndDebugUtilsLabelEXT(*pNativeCommandBuffer);
        }

        /*
        **
        */
        void CRenderer::platformBeginDebugMarker3(
            std::string const& name,
            float4 const& color,
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            VkDebugUtilsLabelEXT markerInfo = {};
            markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
            markerInfo.pLabelName = name.c_str();
            markerInfo.color[0] = color.x; markerInfo.color[1] = color.y; markerInfo.color[2] = color.z; markerInfo.color[3] = color.w;
            VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(pCommandBuffer->getNativeCommandList());
            vkCmdBeginDebugUtilsLabelEXT(*pNativeCommandBuffer, &markerInfo);
        }

        /*
        **
        */
        void CRenderer::platformEndDebugMarker3(
            RenderDriver::Common::CCommandBuffer* pCommandBuffer)
        {
            VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(pCommandBuffer->getNativeCommandList());
            vkCmdEndDebugUtilsLabelEXT(*pNativeCommandBuffer);
        }

        /*
        **
        */
        void CRenderer::platformInitializeRenderJobs(
            std::vector<std::string> const& aRenderJobNames)
        {
            for(auto const& name : aRenderJobNames)
            {
                maRenderJobs[name] = std::make_unique<Render::Vulkan::CRenderJob>();
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
                maRenderJobCommandBuffers[renderJobName] = std::make_unique<RenderDriver::Vulkan::CCommandBuffer>();
                maRenderJobCommandAllocators[renderJobName] = std::make_unique<RenderDriver::Vulkan::CCommandAllocator>();

                // creation descriptors
                RenderDriver::Common::CommandBufferDescriptor desc;
                RenderDriver::Common::CommandAllocatorDescriptor allocatorDesc;
                desc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                allocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
                if(mapRenderJobs[renderJobName]->mType == Render::Common::JobType::Compute)
                {
                    desc.mType = RenderDriver::Common::CommandBufferType::Compute;
                    allocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
                }
                else if(mapRenderJobs[renderJobName]->mType == Render::Common::JobType::Copy)
                {
                    desc.mType = RenderDriver::Common::CommandBufferType::Copy;
                    allocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
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
            RenderDriver::Vulkan::CPipelineState* pPipelineState = reinterpret_cast<RenderDriver::Vulkan::CPipelineState*>(renderPassDesc.mpPipelineState);
            VkRenderPass& renderPass = *(static_cast<VkRenderPass*>(pPipelineState->getNativeRenderPass()));

            auto const& depthStencilState = renderPassDesc.mpPipelineState->getGraphicsDesc().mDepthStencilState;
            VkFramebuffer* pNativeFrameBuffer = nullptr;
            std::vector<VkClearValue> aClearValues;
            if(renderPassDesc.mpRenderJob->mPassType == Render::Common::PassType::SwapChain)
            {
                aClearValues.resize(2);
                pNativeFrameBuffer = static_cast<RenderDriver::Vulkan::CSwapChain*>(mpSwapChain.get())->getNativeFramebuffer(renderPassDesc.miSwapChainFrameBufferindex);

                VkClearValue clearValue = {};
                memcpy(&clearValue.color, &renderPassDesc.mClearValue, sizeof(float) * 4);
                aClearValues[1].depthStencil.depth = 1.0f;
                aClearValues[1].depthStencil.stencil = 0;
            }
            else
            {
                RenderDriver::Vulkan::CFrameBuffer* pFrameBufferVulkan = static_cast<RenderDriver::Vulkan::CFrameBuffer*>(
                    renderPassDesc.mpRenderJob->mpFrameBuffer
                );
                pNativeFrameBuffer = static_cast<VkFramebuffer*>(pFrameBufferVulkan->getNativeFrameBuffer());

                for(uint32_t i = 0; i < static_cast<uint32_t>(renderPassDesc.mpRenderJob->mapOutputImageAttachments.size()); i++)
                {
                    VkClearValue clearValue;
                    memcpy(&clearValue.color, &renderPassDesc.mClearValue, sizeof(float) * 4);
                    aClearValues.push_back(clearValue);
                }

                RenderDriver::Common::GraphicsPipelineStateDescriptor& graphicsPipelineStateDesc = pPipelineState->getGraphicsDesc();

                if(graphicsPipelineStateDesc.mDepthStencilState.mbDepthEnabled)
                {
                    VkClearValue clearValue;
                    clearValue.depthStencil.depth = 1.0f;
                    clearValue.depthStencil.stencil = 0;
                    aClearValues.push_back(clearValue);
                }
            }


            WTFASSERT(pNativeFrameBuffer, "no frame buffer given for pass: %s", renderPassDesc.mpRenderJob->mName.c_str());

            VkRenderPassBeginInfo renderPassBeginInfo = {};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = renderPass;
            renderPassBeginInfo.renderArea.offset.x = renderPassDesc.miOffsetX;
            renderPassBeginInfo.renderArea.offset.y = renderPassDesc.miOffsetY;
            renderPassBeginInfo.renderArea.extent.width = renderPassDesc.miOutputWidth;
            renderPassBeginInfo.renderArea.extent.height = renderPassDesc.miOutputHeight;
            renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(aClearValues.size());
            renderPassBeginInfo.pClearValues = aClearValues.data();
            renderPassBeginInfo.framebuffer = *pNativeFrameBuffer;

            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(renderPassDesc.mpCommandBuffer->getNativeCommandList()));
            vkCmdBeginRenderPass(
                nativeCommandBuffer,
                &renderPassBeginInfo,
                VK_SUBPASS_CONTENTS_INLINE);
        }

        /*
        **
        */
        void CRenderer::platformEndRenderPass2(
            Render::Common::RenderPassDescriptor2 const& renderPassDesc
        )
        {
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(renderPassDesc.mpCommandBuffer->getNativeCommandList()));
            vkCmdEndRenderPass(nativeCommandBuffer);
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
            maVertexBuffers[name] = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            maIndexBuffers[name] = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            maTrianglePositionBuffers[name] = std::make_unique<RenderDriver::Vulkan::CBuffer>();

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
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));
            VkBuffer& nativeVertexBuffer = *(static_cast<VkBuffer*>(mapVertexBuffers[meshName]->getNativeBuffer()));
            VkBuffer& nativeIndexBuffer = *(static_cast<VkBuffer*>(mapIndexBuffers[meshName]->getNativeBuffer()));

            VkDeviceSize aOffsets[1] = {0};
            vkCmdBindVertexBuffers(
                nativeCommandBuffer,
                0,
                1,
                &nativeVertexBuffer,
                aOffsets);

            vkCmdBindIndexBuffer(
                nativeCommandBuffer,
                nativeIndexBuffer,
                0,
                VK_INDEX_TYPE_UINT32);
        }

        /*
        **
        */
        void CRenderer::platformCreateSwapChainCommandBuffer()
        {
            mSwapChainCommandBuffer = std::make_unique<RenderDriver::Vulkan::CCommandBuffer>();
            mSwapChainCommandAllocator = std::make_unique<RenderDriver::Vulkan::CCommandAllocator>();

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
            std::vector<VkImageMemoryBarrier> aImageMemoryBarriers;
            for(uint32_t i = 0; i < iNumBarriers; i++)
            {
                if(aBarriers[i].mpImage == nullptr)
                {
                    continue;
                }

                RenderDriver::Common::Utils::TransitionBarrierInfo const& barrierInfo = aBarriers[i];
                RenderDriver::Vulkan::CImage* pImageVulkan = (RenderDriver::Vulkan::CImage*)barrierInfo.mpImage;
                VkImage* pNativeImage = static_cast<VkImage*>(barrierInfo.mpImage->getNativeImage());
                bool bIsDepthStencil = (
                    pImageVulkan->getInitialImageLayout(0) == RenderDriver::Common::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                );

                VkImageMemoryBarrier imageMemoryBarrier = {};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.image = *pNativeImage;
                imageMemoryBarrier.subresourceRange = (bIsDepthStencil == true) ? 
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1} : 
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                
                VkImageLayout oldLayoutVulkan = SerializeUtils::Vulkan::convert(barrierInfo.mBefore);
                VkImageLayout newLayoutVulkan = SerializeUtils::Vulkan::convert(barrierInfo.mAfter);

                VkAccessFlags srcAccessMask = (barrierInfo.mbWriteableBefore == true) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                VkAccessFlags dstAccessMask = (barrierInfo.mbWriteableAfter == true) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

                VkPipelineStageFlags srcStageFlags = (barrierInfo.mbWriteableBefore == true) ? 
                    (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT) :
                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                VkPipelineStageFlags dstStageFlags = (barrierInfo.mbWriteableAfter == true) ?
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT :
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

                //if(pImageVulkan->getInitialImageLayout(0) == RenderDriver::Common::ImageLayout::TRANSFER_DST_OPTIMAL)
                //{
                //    oldLayoutVulkan = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                //    srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                //    srcStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
                //}

                if(barrierInfo.mCommandBufferType == RenderDriver::Common::CommandBufferType::Compute)
                {
                    dstStageFlags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    dstAccessMask = (barrierInfo.mbWriteableAfter == true) ? VK_ACCESS_SHADER_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;
                }

                imageMemoryBarrier.srcAccessMask = srcAccessMask;
                imageMemoryBarrier.dstAccessMask = dstAccessMask;
                imageMemoryBarrier.oldLayout = oldLayoutVulkan;
                imageMemoryBarrier.newLayout = newLayoutVulkan;
                
                pImageVulkan->setImageLayout(SerializeUtils::Vulkan::convert(imageMemoryBarrier.newLayout), 0);
                
                //pImageVulkan->setImageTransitionInfo(
                //    newLayoutVulkan,
                //    dstAccessMask,
                //    dstStageFlags
                //);

                VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList());
                vkCmdPipelineBarrier(
                    *pNativeCommandBuffer,
                    srcStageFlags,
                    dstStageFlags,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &imageMemoryBarrier
                );
            }

            if(aImageMemoryBarriers.size() > 0)
            {
                VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList());
                vkCmdPipelineBarrier(
                    *pNativeCommandBuffer,
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    iNumBarriers,
                    aImageMemoryBarriers.data());
            }
        }

        /*
        **
        */
        void CRenderer::platformTransitionOutputAttachments()
        {
            for(auto& renderJobKeyValue : maRenderJobs)
            {
                RenderDriver::Common::CCommandQueue* pGraphicsCommandQueue = mpGraphicsCommandQueue.get();
                RenderDriver::Common::CCommandQueue* pComputeCommandQueue = mpComputeCommandQueue.get();

                VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(mapRenderJobCommandBuffers[renderJobKeyValue.first]->getNativeCommandList());
                for(auto& outputAttachmentKeyValue : renderJobKeyValue.second->mapOutputImageAttachments)
                {
                    if(outputAttachmentKeyValue.second == nullptr)
                    {
                        continue;
                    }

                    VkResult ret = vkResetCommandBuffer(*pNativeCommandBuffer, 0);
                    WTFASSERT(ret == VK_SUCCESS, "Error resetting command buffer: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

                    VkCommandBufferBeginInfo beginInfo{};
                    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                    beginInfo.flags = 0;
                    beginInfo.pInheritanceInfo = nullptr;
                    ret = vkBeginCommandBuffer(*pNativeCommandBuffer, &beginInfo);
                    WTFASSERT(ret == VK_SUCCESS, "Error starting command buffer: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

                    RenderDriver::Vulkan::CImage* pImageVulkan = (RenderDriver::Vulkan::CImage*)outputAttachmentKeyValue.second;
                    VkImage& nativeImage = *((VkImage*)outputAttachmentKeyValue.second->getNativeImage());
                    bool bIsDepthStencil = (pImageVulkan->getFormat() == RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT);

                    VkImageLayout oldLayoutVulkan = VK_IMAGE_LAYOUT_UNDEFINED;
                    VkImageLayout newLayoutVulkan = (bIsDepthStencil) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                    // memory barrier for image to transfer layout
                    VkImageMemoryBarrier imageMemoryBarrier = {};
                    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    imageMemoryBarrier.oldLayout = oldLayoutVulkan;
                    imageMemoryBarrier.newLayout = newLayoutVulkan;
                    imageMemoryBarrier.image = nativeImage;
                    imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                    if(oldLayoutVulkan == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                        newLayoutVulkan == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
                        bIsDepthStencil)
                    {
                        imageMemoryBarrier.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
                    }

                    pImageVulkan->setImageLayout(SerializeUtils::Vulkan::convert(imageMemoryBarrier.newLayout), 0);

                    vkCmdPipelineBarrier(
                        *pNativeCommandBuffer,
                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0,
                        0,
                        nullptr,
                        0,
                        nullptr,
                        1,
                        &imageMemoryBarrier);

                    ret = vkEndCommandBuffer(*pNativeCommandBuffer);
                    WTFASSERT(ret == VK_SUCCESS, "Error closing command buffer: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

                    VkQueue& nativeQueue = *((VkQueue*)mpGraphicsCommandQueue->getNativeCommandQueue());
                    // VkSubmitInfo submitInfo = {};
                    // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    // submitInfo.pNext = nullptr;
                    // submitInfo.commandBufferCount = 1;
                    // submitInfo.pCommandBuffers = pNativeCommandBuffer;
                    // submitInfo.waitSemaphoreCount = 0;
                    // submitInfo.pWaitSemaphores = nullptr;
                    // submitInfo.pWaitDstStageMask = nullptr;
                    // submitInfo.signalSemaphoreCount = 0;
                    // submitInfo.pSignalSemaphores = nullptr;
                    // ret = vkQueueSubmit(
                    //    nativeQueue,
                    //    1,
                    //    &submitInfo,
                    //    VK_NULL_HANDLE);
                    //WTFASSERT(ret == VK_SUCCESS, "Error submitting command buffer: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));
                    mpGraphicsCommandQueue->execCommandBufferSynchronized(
                        *mapRenderJobCommandBuffers[renderJobKeyValue.first],
                        *mpDevice
                    );

                    //vkQueueWaitIdle(nativeQueue);
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
#if defined(USE_RAY_TRACING)
            PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
                vkGetInstanceProcAddr(
                    mInstance,
                    "vkGetBufferDeviceAddressKHR")
                );

            PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(
                vkGetInstanceProcAddr(
                    mInstance,
                    "vkCreateAccelerationStructureKHR")
                );

            PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(
                vkGetInstanceProcAddr(
                    mInstance,
                    "vkGetAccelerationStructureBuildSizesKHR")
                );

            PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(
                vkGetInstanceProcAddr(
                    mInstance,
                    "vkCmdBuildAccelerationStructuresKHR")
                );

            PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(
                vkGetInstanceProcAddr(
                    mInstance,
                    "vkGetAccelerationStructureDeviceAddressKHR")
                );

            RenderDriver::Vulkan::CCommandAllocator commandAllocator;

            RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc;
            commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
            commandAllocator.create(commandAllocatorDesc, *mpDevice);
            RenderDriver::Vulkan::CCommandBuffer commandBuffer;
            RenderDriver::Common::CommandBufferDescriptor commandBufferDesc;
            commandBufferDesc.mpCommandAllocator = &commandAllocator;
            commandBufferDesc.mpPipelineState = nullptr;
            commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
            commandBuffer.create(commandBufferDesc, *mpDevice);

            // We flip the matrix [1][1] = -1.0f to accomodate for the glTF up vector
            VkTransformMatrixKHR transformMatrix =
            {
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f
            };

            mTransformBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            RenderDriver::Common::BufferDescriptor transformBufferDesc;
            transformBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage((uint32_t)(
                uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly) | 
                uint32_t(RenderDriver::Common::BufferUsage::TransferDest))
            );
            transformBufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            transformBufferDesc.miSize = sizeof(VkAccelerationStructureInstanceKHR);
            mTransformBuffer->create(
                transformBufferDesc,
                *mpDevice);

            // this is not working, create a command buffer and upload
            //platformUploadResourceData(
            //    *mTransformBuffer,
            //    &transformMatrix,
            //    sizeof(transformMatrix),
            //    0
            //);
            uint32_t iFlags = uint32_t(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) | uint32_t(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION);
            copyCPUToBuffer(
                mTransformBuffer.get(),
                &transformMatrix,
                0,
                sizeof(transformMatrix),
                iFlags
            );

            VkBufferDeviceAddressInfoKHR bufferAddressInfoDesc = {};
            bufferAddressInfoDesc.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bufferAddressInfoDesc.buffer = *((VkBuffer*)maIndexBuffers["bistro"]->getNativeBuffer());
            VkDeviceAddress triangleIndexBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &bufferAddressInfoDesc
            );

            bufferAddressInfoDesc.buffer = *((VkBuffer*)maTrianglePositionBuffers["bistro"]->getNativeBuffer());
            VkDeviceAddress trianglePositionBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &bufferAddressInfoDesc
            );

            bufferAddressInfoDesc.buffer = *((VkBuffer*)mTransformBuffer->getNativeBuffer());
            VkDeviceAddress transformDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &bufferAddressInfoDesc
            );

            VkDevice& nativeDevice = *((VkDevice*)mpDevice->getNativeDevice());
            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(
                nativeDevice, 
                "vkSetDebugUtilsObjectNameEXT");

            {
                // geometries for all the meshes
                std::vector<VkAccelerationStructureGeometryKHR> aGeometries;
                std::vector<VkAccelerationStructureBuildRangeInfoKHR> aBuildRangeInfo;
                std::vector<uint32_t> aiMaxPrimitiveCounts;
                for(auto const& meshRange : aMeshRanges)
                {
                    VkDeviceOrHostAddressConstKHR trianglePositionAddress;
                    trianglePositionAddress.deviceAddress = trianglePositionBufferDeviceAddress;

                    VkDeviceOrHostAddressConstKHR triangleIndexAddress;
                    triangleIndexAddress.deviceAddress = triangleIndexBufferDeviceAddress;

                    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress;
                    transformBufferDeviceAddress.deviceAddress = transformDeviceAddress;

                    VkAccelerationStructureGeometryKHR geometry = {};
                    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
                    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
                    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
                    geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
                    geometry.geometry.triangles.vertexData = trianglePositionAddress;
                    geometry.geometry.triangles.maxVertex = (uint32_t)aTrianglePositions.size();

                    geometry.geometry.triangles.vertexStride = sizeof(float4);
                    geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
                    geometry.geometry.triangles.indexData = triangleIndexAddress;
                    geometry.geometry.triangles.transformData = transformBufferDeviceAddress;

                    aGeometries.push_back(geometry);
                    aiMaxPrimitiveCounts.push_back((meshRange.second - meshRange.first) / 3);

                    uint32_t iNumTriangles = (meshRange.second - meshRange.first) / 3;
                    VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo = {};
                    buildRangeInfo.firstVertex = 0;
                    buildRangeInfo.primitiveOffset = meshRange.first * sizeof(uint32_t);
                    buildRangeInfo.primitiveCount = iNumTriangles;
                    buildRangeInfo.transformOffset = 0;
                    aBuildRangeInfo.push_back(buildRangeInfo);
                }

                // primitive ranges
                std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos;
                for(auto& rangeInfo : aBuildRangeInfo) {
                    pBuildRangeInfos.push_back(&rangeInfo);
                }

                // Get size info
                VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
                accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
                accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
                accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(aGeometries.size());
                accelerationStructureBuildGeometryInfo.pGeometries = aGeometries.data();

                // acceleration structure build size to allocate buffers (acceleration buffer, build scratch buffer)
                VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
                accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
                vkGetAccelerationStructureBuildSizesKHR(
                    nativeDevice,
                    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                    &accelerationStructureBuildGeometryInfo,
                    aiMaxPrimitiveCounts.data(),
                    &accelerationStructureBuildSizesInfo);

                // bottom acceleration buffer
                mpBottomLevelAccelerationStructureBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
                RenderDriver::Common::BufferDescriptor bottomLASDesc;
                bottomLASDesc.miSize = accelerationStructureBuildSizesInfo.accelerationStructureSize;
                bottomLASDesc.mBufferUsage = (RenderDriver::Common::BufferUsage)(
                    uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                    uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) |
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit) |
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly)
                );
                mpBottomLevelAccelerationStructureBuffer->create(
                    bottomLASDesc, 
                    *mpDevice
                );
                mpBottomLevelAccelerationStructureBuffer->setID("Bottom Level Acceleration Structure Buffer");

                // bottom acceleration structure handle
                VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
                accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
                accelerationStructureCreateInfo.buffer = *((VkBuffer *)mpBottomLevelAccelerationStructureBuffer->getNativeBuffer());
                accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
                accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                vkCreateAccelerationStructureKHR(
                    nativeDevice, 
                    &accelerationStructureCreateInfo, 
                    nullptr, 
                    &mBottomLevelASHandle);

                VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
                objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
                objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mBottomLevelASHandle);
                objectNameInfo.objectType = VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
                objectNameInfo.pObjectName = "Bottom Level Acceleration Structure";
                setDebugUtilsObjectNameEXT(
                    nativeDevice,
                    &objectNameInfo
                );

                // Create a small scratch buffer used during build of the bottom level acceleration structure
                std::unique_ptr<RenderDriver::Vulkan::CBuffer> mScratcuBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
                RenderDriver::Common::BufferDescriptor scratchBufferDesc;
                scratchBufferDesc.miSize = accelerationStructureBuildSizesInfo.buildScratchSize;
                scratchBufferDesc.mBufferUsage = (RenderDriver::Common::BufferUsage)(
                    uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                    uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer) | 
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit) |
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly)
                );
                mScratcuBuffer->create(
                    scratchBufferDesc,
                    *mpDevice
                );
                bufferAddressInfoDesc.buffer = *((VkBuffer*)mScratcuBuffer->getNativeBuffer());
                VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                    *((VkDevice*)mpDevice->getNativeDevice()),
                    &bufferAddressInfoDesc
                );
                
                // Build the acceleration structure on the device via a one-time command buffer submission
                // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
                accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                accelerationStructureBuildGeometryInfo.dstAccelerationStructure = mBottomLevelASHandle;
                accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;
                commandBuffer.reset();
                VkCommandBuffer& nativeCommandBuffer = *((VkCommandBuffer*)commandBuffer.getNativeCommandList());
                vkCmdBuildAccelerationStructuresKHR(
                    nativeCommandBuffer,
                    1,
                    &accelerationStructureBuildGeometryInfo,
                    pBuildRangeInfos.data());
                commandBuffer.close();
                mpGraphicsCommandQueue->execCommandBuffer(
                    commandBuffer,
                    *mpDevice
                );
                vkQueueWaitIdle(
                    *((VkQueue*)mpGraphicsCommandQueue->getNativeCommandQueue())
                );

                bufferAddressInfoDesc.buffer = *((VkBuffer*)mpBottomLevelAccelerationStructureBuffer->getNativeBuffer());
                VkDeviceAddress bottomASDeviceAddress = vkGetBufferDeviceAddressKHR(
                    *((VkDevice*)mpDevice->getNativeDevice()),
                    &bufferAddressInfoDesc
                );

                VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo = {};
                accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
                accelerationDeviceAddressInfo.accelerationStructure = mBottomLevelASHandle;
                mBottomAccelerationDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(
                    nativeDevice, 
                    &accelerationDeviceAddressInfo
                );

                mScratcuBuffer->releaseNativeBuffer();
                mScratcuBuffer.reset();

            }   // bottom level acceleration structure
            
            VkAccelerationStructureInstanceKHR instance = {};
            instance.transform = transformMatrix;
            instance.instanceCustomIndex = 0;
            instance.mask = 0xFF;
            instance.instanceShaderBindingTableRecordOffset = 0;
            instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            instance.accelerationStructureReference = mBottomAccelerationDeviceAddress;

            mpInstanceBuffer =std::make_unique<RenderDriver::Vulkan::CBuffer>();
            RenderDriver::Common::BufferDescriptor instanceBufferDesc;
            instanceBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage(
                uint32_t(
                    uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                    uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureBuildInputReadOnly) |
                    uint32_t(RenderDriver::Common::BufferUsage::TransferDest)
                )
            );
            instanceBufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            instanceBufferDesc.miSize = sizeof(VkAccelerationStructureInstanceKHR);
            mpInstanceBuffer->create(
                instanceBufferDesc, 
                *mpDevice);
            copyCPUToBuffer(
                mpInstanceBuffer.get(),
                &instance,
                0,
                sizeof(VkAccelerationStructureInstanceKHR),
                iFlags
            );

            // get the device address of the instance buffer info
            VkBufferDeviceAddressInfoKHR bufferDeviceAI = {};
            bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            bufferDeviceAI.buffer = *((VkBuffer *)mpInstanceBuffer->getNativeBuffer());
            VkDeviceAddress instanceBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice *)mpDevice->getNativeDevice()), 
                &bufferDeviceAI
            );

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(bufferDeviceAI.buffer);
            objectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
            objectNameInfo.pObjectName = "TLAS Instance Buffer";
            setDebugUtilsObjectNameEXT(
                nativeDevice,
                &objectNameInfo
            );


            // geometry
            VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
            accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
            accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
            accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
            VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress = {};
            instanceDataDeviceAddress.deviceAddress = instanceBufferDeviceAddress;
            accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

            // geometry info
            VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
            accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            accelerationStructureBuildGeometryInfo.geometryCount = 1;
            accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

            // get the buffer size needed to build the acceleration structure
            uint32_t iNumPrimitives = 1;
            VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
            accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
            vkGetAccelerationStructureBuildSizesKHR(
                nativeDevice,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
                &accelerationStructureBuildGeometryInfo,
                &iNumPrimitives,
                &accelerationStructureBuildSizesInfo
            );

            // TLAS buffer
            mTLASBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            RenderDriver::Common::BufferDescriptor tlasDesc;
            tlasDesc.mBufferUsage = RenderDriver::Common::BufferUsage((uint32_t)(
                uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                uint32_t(RenderDriver::Common::BufferUsage::AccelerationStructureStorageBit))
            );
            tlasDesc.miSize = accelerationStructureBuildSizesInfo.accelerationStructureSize;
            tlasDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            mTLASBuffer->create(
                tlasDesc, 
                *mpDevice
            );

            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(*((VkBuffer*)mTLASBuffer->getNativeBuffer()));
            objectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
            objectNameInfo.pObjectName = "TLAS Instance Buffer";
            setDebugUtilsObjectNameEXT(
                nativeDevice,
                &objectNameInfo
            );

            // TLAS Acceleration Structure with the buffer created from above
            VkAccelerationStructureKHR mTLASHandle;
            VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
            accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            accelerationStructureCreateInfo.buffer = *((VkBuffer *)mTLASBuffer->getNativeBuffer());
            accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
            accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            vkCreateAccelerationStructureKHR(
                nativeDevice, 
                &accelerationStructureCreateInfo, 
                nullptr, 
                &mTLASHandle);

            // Create a small scratch buffer used during build of the top level acceleration structure
            std::unique_ptr<RenderDriver::Vulkan::CBuffer> mScratchBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
            RenderDriver::Common::BufferDescriptor scratchBufferDesc;
            scratchBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage((uint32_t)(
                uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                uint32_t(RenderDriver::Common::BufferUsage::StorageBuffer))
            );
            scratchBufferDesc.miSize = accelerationStructureBuildSizesInfo.buildScratchSize;
            scratchBufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
            mScratchBuffer->create(
                scratchBufferDesc,
                *mpDevice
            );
            VkBufferDeviceAddressInfoKHR scratchBufferAddressInfoDesc = {};
            scratchBufferAddressInfoDesc.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
            scratchBufferAddressInfoDesc.buffer = *((VkBuffer*)mScratchBuffer->getNativeBuffer());
            VkDeviceAddress scratchBufferDeviceAddress = vkGetBufferDeviceAddressKHR(
                *((VkDevice*)mpDevice->getNativeDevice()),
                &scratchBufferAddressInfoDesc
            );

            // actual building of acceleration structure with the TLAS handle and scratch buffer created from above
            VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {};
            accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
            accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            accelerationBuildGeometryInfo.dstAccelerationStructure = mTLASHandle;
            accelerationBuildGeometryInfo.geometryCount = 1;
            accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
            accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBufferDeviceAddress;
            VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = {};
            accelerationStructureBuildRangeInfo.primitiveCount = 1;
            accelerationStructureBuildRangeInfo.primitiveOffset = 0;
            accelerationStructureBuildRangeInfo.firstVertex = 0;
            accelerationStructureBuildRangeInfo.transformOffset = 0;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = 
                {&accelerationStructureBuildRangeInfo};
            commandBuffer.reset();
            VkCommandBuffer& nativeCommandBuffer = *((VkCommandBuffer *)commandBuffer.getNativeCommandList());
            vkCmdBuildAccelerationStructuresKHR(
                nativeCommandBuffer,
                1,
                &accelerationBuildGeometryInfo,
                accelerationBuildStructureRangeInfos.data()
            );
            commandBuffer.close();
            mpGraphicsCommandQueue->execCommandBuffer(
                commandBuffer,
                *mpDevice
            );
            vkQueueWaitIdle(
                *((VkQueue*)mpGraphicsCommandQueue->getNativeCommandQueue())
            );

            mScratchBuffer->releaseNativeBuffer();
            mScratchBuffer.reset();

            //mpInstanceBuffer->releaseNativeBuffer();
            //mpInstanceBuffer.reset();

            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mTLASHandle);
            objectNameInfo.objectType = VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
            objectNameInfo.pObjectName = "Top Level Acceleration Structure";
            setDebugUtilsObjectNameEXT(
                nativeDevice,
                &objectNameInfo
            );

            maAccelerationStructures["bistro"] = std::make_unique<RenderDriver::Vulkan::CAccelerationStructure>();
            maAccelerationStructures["bistro"]->setNativeAccelerationStructure(&mTLASHandle);

            mapAccelerationStructures["bistro"] = maAccelerationStructures["bistro"].get(); 
#endif // USE_RAY_TRACING
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
#if defined(USE_RAY_TRACING)
            VkCommandBuffer& nativeCommandBuffer = *((VkCommandBuffer*)commandBuffer.getNativeCommandList());

            std::string rayGenName = pRenderJob->mName + "-shader-binding-table-raygen";
            std::string missName = pRenderJob->mName + "-shader-binding-table-miss";
            std::string closestHitName = pRenderJob->mName + "-shader-binding-table-closest-hit";
            VkStridedDeviceAddressRegionKHR emptySbtEntry = {};
            vkCmdTraceRaysKHR(
                nativeCommandBuffer,
                &maShaderBindingTables[rayGenName].mStridedDeviceAddressRegion,
                &maShaderBindingTables[missName].mStridedDeviceAddressRegion,
                &maShaderBindingTables[closestHitName].mStridedDeviceAddressRegion,
                &emptySbtEntry,
                iScreenWidth,
                iScreenHeight,
                1);
#endif // USE_RAY_TRACING
        }

        /*
        **
        */
        void CRenderer::platformRayTraceShaderSetup(
            Render::Common::CRenderJob* pRenderJob
        )
        {
#if defined(USE_RAY_TRACING)
            PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(
                vkGetInstanceProcAddr(
                    mInstance,
                    "vkGetBufferDeviceAddressKHR")
                );

            PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(
                vkGetInstanceProcAddr(
                    mInstance,
                    "vkGetRayTracingShaderGroupHandlesKHR")
                );

            VkDevice& nativeDevice = *((VkDevice*)mpDevice->getNativeDevice());

            VkPipeline& nativePipelineState = *((VkPipeline*)pRenderJob->mpPipelineState->getNativePipelineState());
            uint32_t iAlignSize = (
                (mRayTracingPipelineProperties.shaderGroupHandleSize + mRayTracingPipelineProperties.shaderGroupHandleAlignment - 1) &
                ~(mRayTracingPipelineProperties.shaderGroupHandleAlignment - 1)
                );
            std::vector<uint8_t> aShaderHandles(iAlignSize * 3);
            VkResult ret = vkGetRayTracingShaderGroupHandlesKHR(
                nativeDevice,
                nativePipelineState,
                0,
                3,
                3 * iAlignSize,
                aShaderHandles.data()
            );
            WTFASSERT(ret == VK_SUCCESS, "Error getting ray tracing shader group handles: %s", RenderDriver::Vulkan::Utils::getErrorCode(ret));

            VkBufferDeviceAddressInfoKHR bufferAddressInfoDesc = {};
            bufferAddressInfoDesc.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;

            // shader binding tables for ray trace pipeline
            std::vector<std::string> aShaderTypes = {
                "raygen",
                "miss",
                "closest-hit"
            };
            for(uint32_t i = 0; i < 3; i++)
            {
                std::string bindingTableName = pRenderJob->mName + "-shader-binding-table-" + aShaderTypes[i];
                maShaderBindingTables[bindingTableName].mBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
                RenderDriver::Common::BufferDescriptor sbtBufferDesc = {};
                sbtBufferDesc.miSize = mRayTracingPipelineProperties.shaderGroupHandleSize;
                sbtBufferDesc.mBufferUsage = RenderDriver::Common::BufferUsage(
                    uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress) |
                    uint32_t(RenderDriver::Common::BufferUsage::ShaderBindingTable)
                );
                sbtBufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
                maShaderBindingTables[bindingTableName].mBuffer->create(
                    sbtBufferDesc,
                    *mpDevice
                );
                bufferAddressInfoDesc.buffer = *((VkBuffer*)maShaderBindingTables[bindingTableName].mBuffer->getNativeBuffer());
                VkDeviceAddress sbtDeviceAddress = vkGetBufferDeviceAddressKHR(
                    nativeDevice,
                    &bufferAddressInfoDesc
                );

                VkDeviceMemory& bufferDeviceMemory = *((VkDeviceMemory*)maShaderBindingTables[bindingTableName].mBuffer->getNativeDeviceMemory());
                vkMapMemory(
                    nativeDevice,
                    bufferDeviceMemory,
                    0,
                    sbtBufferDesc.miSize,
                    0,
                    &maShaderBindingTables[bindingTableName].mpMapped
                );
                memcpy(
                    maShaderBindingTables[bindingTableName].mpMapped,
                    aShaderHandles.data() + iAlignSize * i,
                    iAlignSize
                );

                maShaderBindingTables[bindingTableName].mStridedDeviceAddressRegion.size = iAlignSize;
                maShaderBindingTables[bindingTableName].mStridedDeviceAddressRegion.deviceAddress = sbtDeviceAddress;
                maShaderBindingTables[bindingTableName].mStridedDeviceAddressRegion.stride = iAlignSize;

            }   // for shader type = 0 to 3
#endif // USE_RAY_TRACING

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
            RenderDriver::Vulkan::CDevice* pDeviceVulkan = static_cast<RenderDriver::Vulkan::CDevice*>(mpDevice.get());

            mapUploadBuffers.emplace_back(std::make_unique<RenderDriver::Vulkan::CBuffer>());
            RenderDriver::Vulkan::CBuffer& uploadBuffer = static_cast<RenderDriver::Vulkan::CBuffer&>(*mapUploadBuffers.back());

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
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0; 
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageOffset.x = pageCoord.x * iTexturePageDimension;
            bufferCopyRegion.imageOffset.y = pageCoord.y * iTexturePageDimension;
            bufferCopyRegion.imageOffset.z = 0;
            bufferCopyRegion.imageExtent.width = iTexturePageDimension;
            bufferCopyRegion.imageExtent.height = iTexturePageDimension;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = 0;

            // native buffer and image
            RenderDriver::Vulkan::CImage* pImageVulkan = (RenderDriver::Vulkan::CImage*)pDestImage;
            VkCommandBuffer& nativeUploadCommandBufferVulkan = *(static_cast<VkCommandBuffer*>(mpUploadCommandBuffer->getNativeCommandList()));
            VkBuffer& nativeUploadBufferVulkan = *(static_cast<VkBuffer*>(uploadBuffer.getNativeBuffer()));
            VkImage& nativeImage = *(static_cast<VkImage*>(pDestImage->getNativeImage()));
            
            // prepare command buffer
            RenderDriver::Common::CommandBufferState const& commandBufferState = mpUploadCommandBuffer->getState();
            if(commandBufferState == RenderDriver::Common::CommandBufferState::Closed)
            {
                mpUploadCommandBuffer->reset();
            }

            // previous layout
            //VkImageLayout prevLayout = SerializeUtils::Vulkan::convert(((RenderDriver::Vulkan::CImage*)pDestImage)->getImageLayout(0));
            VkImageLayout prevLayout = pImageVulkan->mImageLayout;

            // transition to dest optimal
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = nativeImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            vkCmdPipelineBarrier(
                nativeUploadCommandBufferVulkan,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

            //pImageVulkan->setImageTransitionInfo(
            //    barrier.newLayout,
            //    barrier.dstAccessMask,
            //    VK_PIPELINE_STAGE_TRANSFER_BIT
            //);

            // do the copy
            vkCmdCopyBufferToImage(
                nativeUploadCommandBufferVulkan,
                nativeUploadBufferVulkan,
                nativeImage,
                VK_IMAGE_LAYOUT_GENERAL, // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion);

            barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
            barrier.newLayout = prevLayout;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = 0;
            vkCmdPipelineBarrier(
                nativeUploadCommandBufferVulkan,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

            //pImageVulkan->setImageTransitionInfo(
            //    barrier.newLayout,
            //    barrier.dstAccessMask,
            //    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
            //);

            mpUploadCommandBuffer->close();

            VkDevice* pNativeDevice = static_cast<VkDevice*>(mpDevice->getNativeDevice());
            mpCopyCommandQueue->execCommandBufferSynchronized(
                *mpUploadCommandBuffer,
                *mpDevice);

            RenderDriver::Vulkan::CCommandQueue* pCopyCommandQueueVulkan = static_cast<RenderDriver::Vulkan::CCommandQueue*>(mpCopyCommandQueue.get());
            VkQueue* pNativeQueue = static_cast<VkQueue*>(pCopyCommandQueueVulkan->getNativeCommandQueue());
            //vkQueueWaitIdle(*pNativeQueue);

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
            RenderDriver::Vulkan::CDevice* pDeviceVulkan = static_cast<RenderDriver::Vulkan::CDevice*>(mpDevice.get());

            // native buffer and image
            VkCommandBuffer& nativeUploadCommandBufferVulkan = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));
            VkBuffer& nativeUploadBufferVulkan = *(static_cast<VkBuffer*>(uploadBuffer.getNativeBuffer()));
            VkImage& nativeImage = *(static_cast<VkImage*>(pDestImage->getNativeImage()));

            // recording
            commandBuffer.reset();

            // set copy region
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageOffset.x = pageCoord.x * iTexturePageDimension;
            bufferCopyRegion.imageOffset.y = pageCoord.y * iTexturePageDimension;
            bufferCopyRegion.imageOffset.z = 0;
            bufferCopyRegion.imageExtent.width = iTexturePageDimension;
            bufferCopyRegion.imageExtent.height = iTexturePageDimension;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = 0;

            // do the copy
            vkCmdCopyBufferToImage(
                nativeUploadCommandBufferVulkan,
                nativeUploadBufferVulkan,
                nativeImage,
                VK_IMAGE_LAYOUT_GENERAL, // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion);

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
            threadCommandAllocator = std::make_unique<RenderDriver::Vulkan::CCommandAllocator>();
            threadCommandBuffer = std::make_unique<RenderDriver::Vulkan::CCommandBuffer>();
        }

        /*
        **
        */
        void CRenderer::platformCreateBuffer(
            std::unique_ptr<RenderDriver::Common::CBuffer>& buffer,
            uint32_t iSize)
        {
            buffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();
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
            commandQueue = std::make_unique<RenderDriver::Vulkan::CCommandQueue>();
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
            struct TransitionInfo
            {
                VkImageMemoryBarrier        mBarrier;
                VkPipelineStageFlags        mSrcStageFlags;
                VkPipelineStageFlags        mDstStageFlags;
            };

            uint32_t iNumAttachments = (uint32_t)pRenderJob->mapInputImageAttachments.size();
            uint32_t iVectorSize = iNumAttachments * sizeof(TransitionInfo);
            TransitionInfo* aInfoData = (TransitionInfo*)acPlatformAttachmentInfo.data();

            VkCommandBuffer& nativeCommandBuffer = *(VkCommandBuffer*)commandBuffer.getNativeCommandList();
            if(!bReverse)
            {
                acPlatformAttachmentInfo.resize(iVectorSize);
                aInfoData = (TransitionInfo*)acPlatformAttachmentInfo.data();
            }
             
            for(uint32_t iAttachment = 0; iAttachment < iNumAttachments; iAttachment++)
            {
                // transition info for the attachment
                auto iter = pRenderJob->mapInputImageAttachments.begin();
                std::advance(iter, iAttachment);
                TransitionInfo& infoData = aInfoData[iAttachment];

                RenderDriver::Vulkan::CImage* pImageVulkan = (RenderDriver::Vulkan::CImage*)iter->second;
                VkImage& nativeImage = *(VkImage*)pImageVulkan->getNativeImage();
                bool bIsDepthStencil = (
                    pImageVulkan->getInitialImageLayout(0) == RenderDriver::Common::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                );

                VkImageMemoryBarrier imageMemoryBarrier = {};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.subresourceRange = (bIsDepthStencil == true) ?
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1} :
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
                imageMemoryBarrier.image = nativeImage;

                // access and layout
                if(bReverse)
                {
                    imageMemoryBarrier.srcAccessMask = infoData.mBarrier.dstAccessMask;
                    imageMemoryBarrier.dstAccessMask = infoData.mBarrier.srcAccessMask;

                    imageMemoryBarrier.oldLayout = infoData.mBarrier.newLayout;
                    imageMemoryBarrier.newLayout = infoData.mBarrier.oldLayout;
                }
                else
                {
                    imageMemoryBarrier.srcAccessMask = pImageVulkan->mImageAccessFlags;
                    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    imageMemoryBarrier.oldLayout = pImageVulkan->mImageLayout;
                    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                }

                // pipeline stage 
                VkPipelineStageFlags srcPipelineStage = pImageVulkan->mPipelineStageFlags;
                VkPipelineStageFlags dstPipelineStage = (pRenderJob->mType == Render::Common::JobType::Compute) ?
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : 
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                if(bReverse)
                {
                    srcPipelineStage = infoData.mDstStageFlags;
                    dstPipelineStage = infoData.mSrcStageFlags;
                }
                else
                {
                    aInfoData[iAttachment].mBarrier = imageMemoryBarrier;
                    aInfoData[iAttachment].mSrcStageFlags = srcPipelineStage;
                    aInfoData[iAttachment].mDstStageFlags = dstPipelineStage;
                }

                vkCmdPipelineBarrier(
                    nativeCommandBuffer,
                    srcPipelineStage,
                    dstPipelineStage,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &imageMemoryBarrier);
            }
        }

        /*
        **
        */
        void CRenderer::platformTransitionOutputAttachmentsRayTrace(
            Render::Common::CRenderJob* pRenderJob,
            RenderDriver::Common::CCommandBuffer& commandBuffer)
        {
            VkCommandBuffer& nativeCommandBuffer = *(VkCommandBuffer*)commandBuffer.getNativeCommandList();
            for(auto& outputAttachmentKeyValue : pRenderJob->mapOutputImageAttachments)
            {
                if(outputAttachmentKeyValue.second == nullptr)
                {
                    continue;
                }

                RenderDriver::Vulkan::CImage* pImageVulkan = (RenderDriver::Vulkan::CImage*)outputAttachmentKeyValue.second;
                VkImage& nativeImage = *(VkImage*)pImageVulkan->getNativeImage();

                VkImageMemoryBarrier imageMemoryBarrier = {};
                imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                imageMemoryBarrier.image = nativeImage;
                imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                imageMemoryBarrier.subresourceRange =
                {
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    0,
                    1,
                    0,
                    1
                };

                imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                VkPipelineStageFlags srcPipelineStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

                imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
                imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                VkPipelineStageFlags dstPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

                vkCmdPipelineBarrier(
                    nativeCommandBuffer,
                    srcPipelineStage,
                    dstPipelineStage,
                    0,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &imageMemoryBarrier
                );

                pImageVulkan->setImageTransitionInfo(
                    imageMemoryBarrier.newLayout,
                    imageMemoryBarrier.dstAccessMask,
                    dstPipelineStage
                );
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
            Render::Common::CRenderJob* pDstRenderJob = mapRenderJobs[dstRenderJobName];

            Render::Common::CRenderJob* pSrcRenderJob = mapRenderJobs[srcRenderJobName];
            RenderDriver::Common::CImage* pSrcImage = pSrcRenderJob->mapOutputImageAttachments[srcAttachmentName];
            RenderDriver::Vulkan::CImage* pSrcImageVulkan = (RenderDriver::Vulkan::CImage*)pSrcImage;
            RenderDriver::Common::CImageView* pSrcImageView = pSrcRenderJob->mapOutputImageAttachmentViews[srcAttachmentName];
            RenderDriver::Vulkan::CImageView* pSrcImageViewVulkan = (RenderDriver::Vulkan::CImageView*)pSrcImageView;

            pDstRenderJob->mapOutputImageAttachments[dstAttachmentName] = pSrcImage;
            pDstRenderJob->mapOutputImageAttachmentViews[dstAttachmentName] = pSrcImageView;

            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageView = pSrcImageViewVulkan->getNativeImageView();
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            imageInfo.sampler = VK_NULL_HANDLE;

            RenderDriver::Vulkan::CDescriptorSet* pDescriptorSetVulkan = (RenderDriver::Vulkan::CDescriptorSet*)pDstRenderJob->mpDescriptorSet;
            std::vector<VkDescriptorSet> const& aDescriptorSets = pDescriptorSetVulkan->getNativeDescriptorSets();

            VkWriteDescriptorSet descriptorWrite = {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = aDescriptorSets[0];
            descriptorWrite.dstBinding = 1;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pImageInfo = &imageInfo;

            VkDevice& nativeDevice = *(VkDevice*)mpDevice->getNativeDevice();
            vkUpdateDescriptorSets(
                nativeDevice,
                1,
                &descriptorWrite,
                0,
                nullptr
            );
        }

    }   // Vulkan

}   // Render
