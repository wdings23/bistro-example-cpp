#include <render-driver/Device.h>
#include <render-driver/Format.h>
#include <render-driver/RenderTarget.h>


namespace RenderDriver
{
    namespace Common
    {
        PLATFORM_OBJECT_HANDLE CRenderTarget::create(
            RenderTargetDescriptor renderTargetDesc,
            CDevice& device)
        {
            mHandle = device.assignHandleAndUpdate();
            mDesc = renderTargetDesc;

            return mHandle;
        }

        /*
        **
        */
        std::unique_ptr<RenderDriver::Common::CImage>& CRenderTarget::getImage()
        {
            return mpColorImage;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CRenderTarget::getColorDescriptorHeapHandle()
        {
            return mDesc.mpColorDescriptorHeap->getHandle();
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CRenderTarget::getDepthStencilDescriptorHeapHandle()
        {
            return mDesc.mpDepthStencilDescriptorHeap->getHandle();
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CRenderTarget::createImageView(
            RenderDriver::Common::ImageViewDescriptor const& imageViewDesc,
            RenderDriver::Common::CDevice& device)
        {
            return 0;
        }

    }   // Common

}   // RenderDriver