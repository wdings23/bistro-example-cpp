#pragma once

#include <render-driver/SwapChain.h>
#include <render-driver/Vulkan/ImageVulkan.h>

#include <vulkan/vulkan.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        struct SwapChainDescriptor : public RenderDriver::Common::SwapChainDescriptor
        {
            HWND                            mHWND;
            VkInstance*                     mpInstance;
            VkRenderPass*                   mpRenderPass;
        };

        class CSwapChain : public RenderDriver::Common::CSwapChain
        {
        public:
            CSwapChain() = default;
            virtual ~CSwapChain() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::SwapChainDescriptor& desc,
                RenderDriver::Common::CDevice& device);

            virtual void clear(RenderDriver::Common::ClearRenderTargetDescriptor const& desc);
            virtual void present(RenderDriver::Common::SwapChainPresentDescriptor const& desc);
            virtual uint32_t getCurrentBackBufferIndex();

            void createFrameBuffers(VkRenderPass& renderPass, VkDevice& device);

            inline VkFramebuffer* getNativeFramebuffer(uint32_t iIndex)
            {
                return &maFrameBuffers[iIndex];
            }

        protected:
            VkSwapchainKHR                                          mNativeSwapChain;

            VkSurfaceKHR                                            mSurface;
            VkSurfaceFormatKHR                                      mSurfaceFormat;

            std::vector<VkImageView>                                maColorImageViews;
            std::vector<VkImage>                                    maColorImages;

            std::vector<VkImageView>                                maDepthImageViews;
            std::vector<VkImage>                                    maDepthImages;
            std::vector<VkDeviceMemory>                             maDepthImageMemory;

            std::vector<VkFramebuffer>                              maFrameBuffers;

            VkExtent2D                                              mExtent;

            VkSemaphore                                             mImageAvailableSemaphore;

            VkFence                                                 mWaitFence;

            std::vector<std::unique_ptr<RenderDriver::Vulkan::CImage>>  maImages;
        };

    }   // Common

}   // RenderDriver