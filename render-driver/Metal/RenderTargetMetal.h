#pragma once

#include <render-driver/Device.h>
#include <render-driver/RenderTarget.h>
#include <render-driver/Metal/ImageMetal.h>

namespace RenderDriver
{
    namespace Metal
    {
        class CRenderTarget : public RenderDriver::Common::CRenderTarget
        {
        public:
            CRenderTarget() = default;
            virtual ~CRenderTarget() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::RenderTargetDescriptor desc,
                RenderDriver::Common::CDevice& device);

            virtual PLATFORM_OBJECT_HANDLE createImageView(
                RenderDriver::Common::ImageViewDescriptor const& imageViewDesc,
                RenderDriver::Common::CDevice& device);
            
            inline RenderDriver::Common::CImage* getDepthImage()
            {
                return mpDepthStencilImage.get();
            }
            
        };

    }   // Common

}   // RenderDriver
