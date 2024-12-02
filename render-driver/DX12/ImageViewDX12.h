#pragma once

#include <render-driver/Device.h>
#include <render-driver/ImageView.h>

#include <render_enums.h>
#include <serialize_utils.h>

namespace RenderDriver
{
    namespace DX12
    {
        

        class CImageView : public RenderDriver::Common::CImageView
        {
        public:
            CImageView() = default;
            virtual ~CImageView() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::ImageViewDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

        protected:
            ComPtr<ID3D12Resource>      mRenderTargetResource;
        };

    }   // DX12

}   // RenderDriver