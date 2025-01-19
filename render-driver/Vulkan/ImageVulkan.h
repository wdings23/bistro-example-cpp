#pragma once

#include <render-driver/Image.h>
#include <render-driver/Vulkan/DeviceVulkan.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        class CImage : public RenderDriver::Common::CImage
        {
        public:
            CImage() = default;
            virtual ~CImage() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::ImageDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            inline void setNativeImage(VkImage& nativeImage)
            {
                // swap?

                mNativeImage = nativeImage;
            }

            virtual void setID(std::string const& name);

            virtual void* getNativeImage();

            inline RenderDriver::Common::ImageLayout getImageLayout(uint32_t iQueueType)
            {
                return maCurrImageLayout[iQueueType];
            }

            inline RenderDriver::Common::ImageLayout getInitialImageLayout(uint32_t iQueueType)
            {
                return maInitialLayouts[iQueueType];
            }

            inline VkDeviceMemory& getNativeMemory()
            {
                return mNativeMemory;
            }

            void setImageLayout(
                RenderDriver::Common::ImageLayout const& layout,
                uint32_t iQueueType);

            void setInitialImageLayout(
                RenderDriver::Common::ImageLayout const& layout,
                uint32_t iQueueType);

            inline void setImageTransitionInfo(
                VkImageLayout imageLayout,
                VkAccessFlags imageAccessFlags,
                VkPipelineStageFlags pipelineStageFlags
            )
            {
                mImageLayout = imageLayout;
                mImageAccessFlags = imageAccessFlags;
                mPipelineStageFlags = pipelineStageFlags;
            }

            VkImageLayout               mImageLayout;
            VkAccessFlags               mImageAccessFlags;
            VkPipelineStageFlags        mPipelineStageFlags;

        protected:
            VkImage                                             mNativeImage;
            VkDevice*                                           mpNativeDevice;
            VkDeviceMemory                                      mNativeMemory;

            RenderDriver::Common::ImageLayout                   maCurrImageLayout[3];
            RenderDriver::Common::ImageLayout                   maInitialLayouts[3];

            
        };

    }   // Vulkan

}   // RenderDriver