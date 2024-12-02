#pragma once

#include <render-driver/Device.h>
#include <render-driver/ImageView.h>

#include <render_enums.h>
#include <serialize_utils.h>

namespace RenderDriver
{
    namespace Vulkan
    {


        class CImageView : public RenderDriver::Common::CImageView
        {
        public:
            CImageView() = default;
            virtual ~CImageView() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::ImageViewDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            VkImageView& getNativeImageView()
            {
                return mNativeImageView;
            }

            virtual void setID(std::string const& id);

            inline PLATFORM_OBJECT_HANDLE getImageHandle()
            {
                return mImageHandle;
            }

        protected:
            VkImageView                 mNativeImageView;
            VkDevice*                   mpNativeDevice;
            PLATFORM_OBJECT_HANDLE      mImageHandle;
        };

    }   // Vulkan

}   // RenderDriver