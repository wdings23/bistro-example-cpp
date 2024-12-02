#include <render-driver/DX12/RenderTargetDX12.h>

namespace RenderDriver
{
    namespace DX12
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CRenderTarget::create(
            RenderDriver::Common::RenderTargetDescriptor desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CRenderTarget::create(desc, device);

            mpColorImage = std::make_unique<RenderDriver::DX12::CImage>();
            mpColorImageView = std::make_unique<RenderDriver::DX12::CImageView>();

            RenderDriver::Common::ImageDescriptor imageDesc = {};
            imageDesc.miWidth = desc.miWidth;
            imageDesc.miHeight = desc.miHeight;
            imageDesc.mFormat = desc.mColorFormat;
            imageDesc.miNumImages = 1;
            imageDesc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowRenderTarget;
            imageDesc.mafClearColor = desc.mafClearColor;

            if(desc.mbComputeShaderWritable)
            {
                imageDesc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess;
                imageDesc.mStateFlags = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;
            }

            mColorImageHandle = mpColorImage->create(imageDesc, device);
            mpColorImage->setID(mID + " Color Image");

            RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
            imageViewDesc.mpDescriptorHeap = desc.mpColorDescriptorHeap;
            imageViewDesc.mFormat = desc.mColorFormat;
            imageViewDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
            imageViewDesc.mpImage = mpColorImage.get();
            imageViewDesc.mViewType = RenderDriver::Common::ResourceViewType::RenderTargetView;
            imageViewDesc.miShaderResourceIndex = desc.miShaderResourceIndex;
            assert(imageViewDesc.miShaderResourceIndex != UINT32_MAX);

            if(desc.mbComputeShaderWritable)
            {
                imageViewDesc.mViewType = RenderDriver::Common::ResourceViewType::UnorderedAccessView;
            }

            mColorImageViewHandle = mpColorImageView->create(imageViewDesc, device);

            if(!desc.mbComputeShaderWritable)
            {
                if(desc.mDepthStencilFormat != RenderDriver::Common::Format::UNKNOWN)
                {
                    mpDepthStencilImage = std::make_unique<RenderDriver::DX12::CImage>();
                    mpDepthStencilImageView = std::make_unique<RenderDriver::DX12::CImageView>();

                    imageDesc.mFormat = desc.mDepthStencilFormat;
                    imageDesc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowDepthStencil;
                    mDepthStencilImageHandle = mpDepthStencilImage->create(imageDesc, device);

                    RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);

                    imageViewDesc.mpDescriptorHeap = desc.mpDepthStencilDescriptorHeap;
                    imageViewDesc.mFormat = desc.mDepthStencilFormat;
                    imageViewDesc.mpImage = mpDepthStencilImage.get();
                    mDepthStencilImageViewHandle = mpDepthStencilImageView->create(imageViewDesc, device);

                    mpDepthStencilImage->setID(mID + " Depth Stencil Image");
                }
            }

            return mHandle;
        }
        
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CRenderTarget::createImageView(
            RenderDriver::Common::ImageViewDescriptor const& imageViewDesc,
            RenderDriver::Common::CDevice& device)
        {
            std::unique_ptr<RenderDriver::Common::CImageView> pImageView = std::make_unique<RenderDriver::DX12::CImageView>();
            PLATFORM_OBJECT_HANDLE handle = pImageView->create(imageViewDesc, device);
            mapAdditionalImageViews.push_back(std::move(pImageView));

            return handle;
        }


    }   // DX12

}   // RenderDriver