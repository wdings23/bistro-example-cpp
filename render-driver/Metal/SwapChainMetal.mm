#include <render-driver/Metal/SwapChainMetal.h>
#include <render-driver/Metal/DeviceMetal.h>

#include <utils/wtfassert.h>
#include <sstream>

namespace RenderDriver
{
    namespace Metal
    {
        PLATFORM_OBJECT_HANDLE CSwapChain::create(
            RenderDriver::Common::SwapChainDescriptor& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Metal::SwapChainDescriptor metalDesc = static_cast<RenderDriver::Metal::SwapChainDescriptor&>(desc);
            
            mNativeDevice = (__bridge id<MTLDevice>)device.getNativeDevice();
            
            mNativeDepthTexture = nil;
            
            mColorImage = std::make_unique<RenderDriver::Metal::CImage>();
            mDepthImage = std::make_unique<RenderDriver::Metal::CImage>();
            
            RenderDriver::Common::ImageDescriptor imageDesc = {};
            imageDesc.mFormat = desc.mFormat;
            imageDesc.miWidth = desc.miWidth;
            imageDesc.miHeight = desc.miHeight;
            imageDesc.miNumImages = 1;
            imageDesc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowRenderTarget;
            
            mColorImage->create(imageDesc, device);
            mColorImage->setNativeImage(mNativeDrawableTexture);
            mColorImage->setID("Output Color Image");
            
            imageDesc.mFormat = RenderDriver::Common::Format::D32_FLOAT;
            mDepthImage->create(imageDesc, device);
            mDepthImage->setNativeImage(mNativeDepthTexture);
            
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
            id<MTLCommandQueue> nativeCommandQueue = (__bridge id<MTLCommandQueue>)desc.mpPresentQueue->getNativeCommandQueue();
            id<MTLCommandBuffer> nativeCommandBuffer = [nativeCommandQueue commandBuffer];
            nativeCommandBuffer.label = @"Present Swap Chain Command Buffer";
            [nativeCommandBuffer presentDrawable: mNativeDrawable];
            [nativeCommandBuffer commit];
            [nativeCommandBuffer waitUntilCompleted];
            
            nativeCommandBuffer = nil;
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
        void CSwapChain::setDrawable(
            id<MTLDrawable> drawable,
            id<MTLTexture> drawableTexture,
            uint32_t iWidth,
            uint32_t iHeight)
        {
            mNativeDrawable = drawable;
            mNativeDrawableTexture = drawableTexture;
            
            mColorImage->setNativeImage(mNativeDrawableTexture);
            
            if(mNativeDepthTexture == nil)
            {
                MTLTextureDescriptor* desc = [[MTLTextureDescriptor alloc] init];
                desc.pixelFormat = MTLPixelFormatDepth32Float;
                desc.width = iWidth;
                desc.height = iHeight;
                desc.usage = MTLTextureUsageRenderTarget;
                desc.storageMode = MTLStorageModePrivate;
                
                mNativeDepthTexture = [mNativeDevice newTextureWithDescriptor: desc];
                mDepthImage->setNativeImage(mNativeDepthTexture);
            }
        }
        
    }   // Vulkan

}   // RenderDriver
