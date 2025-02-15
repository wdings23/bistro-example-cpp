
#pragma once

#include <render-driver/SwapChain.h>
#include <render-driver/Metal/ImageMetal.h>
#import <Metal/Metal.h>

namespace RenderDriver
{
    namespace Metal
    {
        struct SwapChainDescriptor : public RenderDriver::Common::SwapChainDescriptor
        {
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

            void setDrawable(
                id<MTLDrawable> drawable,
                id<MTLTexture> drawableTexture,
                uint32_t iWidth,
                uint32_t iHeight);
            
            inline id<MTLDrawable> getNativeDrawable()
            {
                return mNativeDrawable;
            }
            
            inline id<MTLTexture> getNativeDepthTexture()
            {
                return mNativeDepthTexture;
            }
            
            inline RenderDriver::Metal::CImage* getColorImage()
            {
                return mColorImage.get();
            }
            
            inline RenderDriver::Metal::CImage* getDepthImage()
            {
                return mDepthImage.get();
            }
            
        protected:
            id<MTLDrawable>            mNativeDrawable;
            id<MTLTexture>             mNativeDrawableTexture;
            id<MTLDevice>              mNativeDevice;
            id<MTLTexture>             mNativeDepthTexture;
            id<MTLTexture>             mNativeColorTexture;
            id<MTLCommandBuffer>       mNativeSwapChainPassCommandBuffer;
            
            std::unique_ptr<RenderDriver::Metal::CImage>    mColorImage;
            std::unique_ptr<RenderDriver::Metal::CImage>    mDepthImage;
        };

    }   // Common

}   // RenderDriver
