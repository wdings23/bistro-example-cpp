#include <render-driver/Vulkan/SwapChainVulkan.h>
#include <render-driver/Vulkan/PhysicalDeviceVulkan.h>
#include <render-driver/Vulkan/DeviceVulkan.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include <wtfassert.h>
#include <sstream>

namespace RenderDriver
{
    namespace Vulkan
    {
        PLATFORM_OBJECT_HANDLE CSwapChain::create(
            RenderDriver::Common::SwapChainDescriptor& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Vulkan::SwapChainDescriptor vulkanDesc = static_cast<RenderDriver::Vulkan::SwapChainDescriptor&>(desc);

            VkPhysicalDevice* pNativePhysicalDevice = reinterpret_cast<VkPhysicalDevice*>(vulkanDesc.mpPhysicalDevice->getNativeDevice());
            uint32_t iGraphicsQueueFamilyIndex = static_cast<RenderDriver::Vulkan::CPhysicalDevice*>(vulkanDesc.mpPhysicalDevice)->getGraphicsQueueFamilyIndex();

            VkWin32SurfaceCreateInfoKHR win32CreateInfo = {};
            win32CreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            win32CreateInfo.hwnd = vulkanDesc.mHWND;
            win32CreateInfo.hinstance = GetModuleHandle(nullptr);
            VkResult ret = vkCreateWin32SurfaceKHR(*vulkanDesc.mpInstance, &win32CreateInfo, nullptr, &mSurface);
            WTFASSERT(ret == VK_SUCCESS, "error creating surface: %d", ret);

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(*pNativePhysicalDevice, iGraphicsQueueFamilyIndex, mSurface, &presentSupport);
            WTFASSERT(presentSupport, "Selected phyiscal device does not support presentation: %d", iGraphicsQueueFamilyIndex);

            VkSurfaceCapabilitiesKHR capabilities;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*pNativePhysicalDevice, mSurface, &capabilities);

            uint32_t iNumFormats = 0;
            vkGetPhysicalDeviceSurfaceFormatsKHR(*pNativePhysicalDevice, mSurface, &iNumFormats, nullptr);

            std::vector<VkSurfaceFormatKHR> aFormats(iNumFormats);
            vkGetPhysicalDeviceSurfaceFormatsKHR(*pNativePhysicalDevice, mSurface, &iNumFormats, aFormats.data());

            for(auto const& format : aFormats)
            {
                if(format.format == VK_FORMAT_B8G8R8A8_UNORM)
                {
                    mSurfaceFormat = format;
                    break;
                }
            }

            mExtent.width = desc.miWidth;
            mExtent.height = desc.miHeight;

            uint32_t aFamilyQueueIndices[1] = { 0 };

            VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
            swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapChainCreateInfo.surface = mSurface;
            swapChainCreateInfo.minImageCount = capabilities.minImageCount + 1;

            swapChainCreateInfo.imageFormat = mSurfaceFormat.format;
            swapChainCreateInfo.imageColorSpace = mSurfaceFormat.colorSpace;
            swapChainCreateInfo.imageExtent = mExtent;
            swapChainCreateInfo.imageArrayLayers = 1;
            swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapChainCreateInfo.queueFamilyIndexCount = 0;
            swapChainCreateInfo.pQueueFamilyIndices = nullptr;

            swapChainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapChainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
            swapChainCreateInfo.clipped = VK_TRUE;
            swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
            
            RenderDriver::Vulkan::CDevice& vulkanDevice = reinterpret_cast<RenderDriver::Vulkan::CDevice&>(device);
            VkDevice* pNativeDevice = (VkDevice*)vulkanDevice.getNativeDevice();
            VkDevice& nativeDevice = *pNativeDevice;

            ret = vkCreateSwapchainKHR(nativeDevice, &swapChainCreateInfo, nullptr, &mNativeSwapChain);
            WTFASSERT(ret == VK_SUCCESS, "Error creating swap chain, result: %d", ret);

            uint32_t iNumImages = 0;
            vkGetSwapchainImagesKHR(nativeDevice, mNativeSwapChain, &iNumImages, nullptr);
            maColorImages.resize(iNumImages);
            vkGetSwapchainImagesKHR(nativeDevice, mNativeSwapChain, &iNumImages, maColorImages.data());
            
            maImages.resize(iNumImages);
            for(uint32_t i = 0; i < iNumImages; i++)
            {
                maImages[i] = std::make_unique<RenderDriver::Vulkan::CImage>();
                maImages[i]->setNativeImage(maColorImages[i]);
                mapColorRenderTargets[i] = maImages[i].get();
            }

            // color images
            maColorImageViews.resize(iNumImages);
            for(uint32_t i = 0; i < iNumImages; i++)
            {
                VkImageViewCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = maColorImages[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = mSurfaceFormat.format;
                createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                VkResult ret = vkCreateImageView(nativeDevice, &createInfo, nullptr, &maColorImageViews[i]);
                WTFASSERT(ret == VK_SUCCESS, "Error creating image view %d: %d", i, ret);
            }

            // depth images
            maDepthImages.resize(iNumImages);
            maDepthImageMemory.resize(iNumImages);
            {
                VkImageCreateInfo imageCreateInfo = {};
                imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
                imageCreateInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                imageCreateInfo.extent = { mExtent.width, mExtent.height, 1 };
                imageCreateInfo.mipLevels = 1;
                imageCreateInfo.arrayLayers = 1;
                imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

                for(uint32_t i = 0; i < iNumImages; i++)
                {
                    VkResult ret = vkCreateImage(
                        nativeDevice,
                        &imageCreateInfo,
                        nullptr, 
                        &maDepthImages[i]);
                    WTFASSERT(ret == VK_SUCCESS, "Error creating depth image: %d", ret);

                    VkMemoryRequirements memReqs = {};
                    vkGetImageMemoryRequirements(
                        nativeDevice, 
                        maDepthImages[i], 
                        &memReqs);
                    VkMemoryAllocateInfo memAllloc = {};
                    memAllloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    memAllloc.allocationSize = memReqs.size;
                    memAllloc.memoryTypeIndex = 0;
                    ret = vkAllocateMemory(
                        nativeDevice, 
                        &memAllloc, 
                        nullptr, 
                        &maDepthImageMemory[i]);
                    WTFASSERT(ret == VK_SUCCESS, "Error allocating memory for depth image: %d", ret);
                    ret = vkBindImageMemory(
                        nativeDevice, 
                        maDepthImages[i], 
                        maDepthImageMemory[i], 
                        0);
                    WTFASSERT(ret == VK_SUCCESS, "Error binding memory for depth image: %d", ret);
                }
            }
            
            maDepthImageViews.resize(iNumImages);
            for(uint32_t i = 0; i < iNumImages; i++)
            {
                VkImageViewCreateInfo createInfo = {};
                createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                createInfo.image = maDepthImages[i];
                createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                createInfo.format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                createInfo.subresourceRange.baseMipLevel = 0;
                createInfo.subresourceRange.levelCount = 1;
                createInfo.subresourceRange.baseArrayLayer = 0;
                createInfo.subresourceRange.layerCount = 1;

                VkResult ret = vkCreateImageView(
                    nativeDevice, 
                    &createInfo, 
                    nullptr, 
                    &maDepthImageViews[i]);
                WTFASSERT(ret == VK_SUCCESS, "Error creating image view %d: %d", i, ret);
            }

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreInfo.pNext = nullptr;
            semaphoreInfo.flags = 0;

            ret = vkCreateSemaphore(
                nativeDevice,
                &semaphoreInfo,
                nullptr,
                &mImageAvailableSemaphore);
            WTFASSERT(ret == VK_SUCCESS, "Error creating image creation semaphore: %d", ret);

            ret = vkAcquireNextImageKHR(
                nativeDevice,
                mNativeSwapChain,
                UINT64_MAX,
                mImageAvailableSemaphore,
                VK_NULL_HANDLE,
                &miFrameIndex);
            WTFASSERT(ret == VK_SUCCESS, "Error acquiring next swap chain image: %d", ret);

            VkFenceCreateInfo fenceCreateInfo = {};
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            ret = vkCreateFence(
                nativeDevice,
                &fenceCreateInfo,
                nullptr,
                &mWaitFence);
            WTFASSERT(ret == VK_SUCCESS, "Error creating swap chain wait fence: %d", ret);

            return mHandle;

        } 

        /*
        **
        */
        void CSwapChain::clear(RenderDriver::Common::ClearRenderTargetDescriptor const& desc)
        {

        }

        /*
        **
        */
        void CSwapChain::present(RenderDriver::Common::SwapChainPresentDescriptor const& desc)
        {
            VkDevice& nativeDevice = *(static_cast<VkDevice*>(desc.mpDevice->getNativeDevice()));
            VkQueue& nativeQueue = *(static_cast<VkQueue*>(desc.mpPresentQueue->getNativeCommandQueue()));

            VkSwapchainKHR aSwapChains[] = { mNativeSwapChain };
            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &mImageAvailableSemaphore;
            presentInfo.pResults = nullptr;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &aSwapChains[0];
            presentInfo.pImageIndices = &miFrameIndex;

            VkResult ret = vkQueuePresentKHR(nativeQueue, &presentInfo);
            WTFASSERT(ret == VK_SUCCESS, "Error presenting swap chain: %d", ret);

            //VkSemaphoreWaitInfo waitInfo;
            //waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
            //waitInfo.pNext = NULL;
            //waitInfo.flags = 0;
            //waitInfo.semaphoreCount = 1;
            //waitInfo.pSemaphores = &mImageAvailableSemaphore;
            //waitInfo.pValues = nullptr;
            //
            //ret = vkWaitForFences(
            //    nativeDevice,
            //    1,
            //    &mWaitFence,
            //    VK_TRUE,
            //    UINT64_MAX);
            //WTFASSERT(ret == VK_SUCCESS, "Error waiting for fence: %d", ret);
            //
            //ret = vkResetFences(
            //    nativeDevice,
            //    1,
            //    &mWaitFence);
            //WTFASSERT(ret == VK_SUCCESS, "Error resetting fence: %d", ret);

            vkQueueWaitIdle(nativeQueue);

            // next frame image
            ret = vkAcquireNextImageKHR(
                nativeDevice,
                mNativeSwapChain,
                UINT64_MAX,
                mImageAvailableSemaphore,
                VK_NULL_HANDLE,
                &miFrameIndex);
            WTFASSERT(ret == VK_SUCCESS, "Error acquiring next swap chain image: %d", ret);
        }

        /*
        **
        */
        uint32_t CSwapChain::getCurrentBackBufferIndex()
        {
            return miFrameIndex;
        }

        /*
        **
        */
        void CSwapChain::createFrameBuffers(
            VkRenderPass& renderPass,
            VkDevice& device)
        {
            WTFASSERT(renderPass, "No render pass given for frame buffer");

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            
            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT");
            

            uint32_t iNumImages = static_cast<uint32_t>(maColorImages.size());
            maFrameBuffers.resize(iNumImages);
            for(uint32_t i = 0; i < iNumImages; i++)
            {
                VkImageView aImageViews[2] =
                {
                    maColorImageViews[i],
                    maDepthImageViews[i]
                };

                VkFramebufferCreateInfo framebufferInfo = {};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = 2;
                framebufferInfo.pAttachments = aImageViews;
                framebufferInfo.width = mExtent.width;
                framebufferInfo.height = mExtent.height;
                framebufferInfo.layers = 1;

                VkResult ret = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &maFrameBuffers[i]);
                WTFASSERT(ret == VK_SUCCESS, "Error creating frame buffer %d: %d", i, ret);

                std::ostringstream oss;
                oss << "Swap Chain Frame Buffer " << i;
                objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(maFrameBuffers[i]);
                std::string name = oss.str();
                objectNameInfo.pObjectName = name.c_str();
                objectNameInfo.objectType = VK_OBJECT_TYPE_FRAMEBUFFER;
                setDebugUtilsObjectNameEXT(
                    device,
                    &objectNameInfo);

                std::ostringstream imageOSS;
                imageOSS << "Swap Chain Frame Buffer Image " << i;
                objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(maColorImages[i]);
                name = imageOSS.str();
                objectNameInfo.pObjectName = name.c_str();
                objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
                setDebugUtilsObjectNameEXT(
                    device,
                    &objectNameInfo);

                std::ostringstream imageViewOSS;
                imageViewOSS << "Swap Chain Frame Buffer Image View " << i;
                objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(maColorImageViews[i]);
                name = imageViewOSS.str();
                objectNameInfo.pObjectName = name.c_str();
                objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
                setDebugUtilsObjectNameEXT(
                    device,
                    &objectNameInfo);

                std::ostringstream depthImageOSS;
                depthImageOSS << "Swap Chain Frame Buffer Depth Image " << i;
                objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(maDepthImages[i]);
                name = depthImageOSS.str();
                objectNameInfo.pObjectName = name.c_str();
                objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
                setDebugUtilsObjectNameEXT(
                    device,
                    &objectNameInfo);

                std::ostringstream depthImageViewOSS;
                depthImageViewOSS << "Swap Chain Frame Buffer Depth Image View " << i;
                objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(maDepthImageViews[i]);
                name = depthImageViewOSS.str();
                objectNameInfo.pObjectName = name.c_str();
                objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
                setDebugUtilsObjectNameEXT(
                    device,
                    &objectNameInfo);

                std::ostringstream depthImageMemoryOSS;
                depthImageMemoryOSS << "Swap Chain Frame Buffer Depth Image Memory " << i;
                objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(maDepthImageMemory[i]);
                name = depthImageMemoryOSS.str();
                objectNameInfo.pObjectName = name.c_str();
                objectNameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
                setDebugUtilsObjectNameEXT(
                    device,
                    &objectNameInfo);
            }
        }

    }   // Vulkan

}   // RenderDriver
