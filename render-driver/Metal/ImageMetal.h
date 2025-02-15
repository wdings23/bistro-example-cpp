#pragma once

#include <render-driver/Image.h>
#include <render-driver/Metal/DeviceMetal.h>

namespace RenderDriver
{
    namespace Metal
    {
        class CImage : public RenderDriver::Common::CImage
        {
        public:
            CImage() = default;
            virtual ~CImage() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::ImageDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            inline void setNativeImage(id<MTLTexture> nativeImage)
            {
                mNativeImage = nativeImage;
            }

            virtual void setID(std::string const& name);

            virtual void* getNativeImage();

            void makeBuffer(id<MTLCommandBuffer> commandBuffer, id<MTLDevice> device);
            
            id<MTLBuffer> getNativeBuffer()
            {
                return mNativeBuffer;
            }
            
        protected:
            id<MTLTexture>              mNativeImage;
            id<MTLBuffer>               mNativeBuffer;
            id<MTLDevice>               mNativeDevice;
        };

    }   // Metal

}   // RenderDriver
