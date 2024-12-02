#pragma once

#include <render-driver/Image.h>
#include <render-driver/DX12/DeviceDX12.h>

namespace RenderDriver
{
    namespace DX12
    {
        class CImage : public RenderDriver::Common::CImage
        {
        public:
            CImage() = default;
            virtual ~CImage() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::ImageDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            inline void setNativeImage(ComPtr<ID3D12Resource>& nativeImage)
            {
                // swap?

                mNativeImage = nativeImage;
            }

            virtual void setID(std::string const& name);

            virtual void* getNativeImage();

        protected:
            Microsoft::WRL::ComPtr<ID3D12Resource>              mNativeImage;
            D3D12_RESOURCE_DESC                                 mResourceDesc;
            
            
        
        };

    }   // Common

}   // RenderDriver