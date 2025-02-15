#include <render-driver/Metal/ImageMetal.h>
#include <render-driver/Metal/UtilsMetal.h>

#include <utils/wtfassert.h>

namespace RenderDriver
{
    namespace Metal
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CImage::create(
            RenderDriver::Common::ImageDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CImage::create(desc, device);

            mNativeDevice = (__bridge id<MTLDevice>)device.getNativeDevice();

            MTLPixelFormat pixelFormat = RenderDriver::Metal::Utils::convert(desc.mFormat);
            
            MTLTextureDescriptor* textureDescriptor = [
                MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat: pixelFormat
                width: desc.miWidth
                height: desc.miHeight
                mipmapped: FALSE];
            
            bool bRenderTarget = ((static_cast<uint32_t>(desc.mResourceFlags) & static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowRenderTarget)) > 0);
            
            textureDescriptor.arrayLength = desc.miNumImages;
            textureDescriptor.textureType = (desc.miNumImages > 1) ? MTLTextureType2DArray : MTLTextureType2D;
            textureDescriptor.usage =  bRenderTarget ? (MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead) : MTLTextureUsageShaderRead;
            textureDescriptor.usage =
                ((static_cast<uint32_t>(desc.mResourceFlags) & static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess)) > 0) ?
                (textureDescriptor.usage | MTLTextureUsageShaderWrite) :
                textureDescriptor.usage;
            
            textureDescriptor.storageMode = (bRenderTarget) ? MTLStorageModePrivate : MTLStorageModeManaged;
            mNativeImage = [mNativeDevice newTextureWithDescriptor:textureDescriptor];
            
            return mHandle;
        }

        /*
        **
        */
        void CImage::makeBuffer(id<MTLCommandBuffer> commandBuffer, id<MTLDevice> device)
        {
            MTLOrigin origin;
            origin.x = 0; origin.y = 0; origin.z = 0;
            
            MTLSize size;
            size.width = mDesc.miWidth; size.height = mDesc.miHeight; size.depth = 1;
            
            uint32_t iNumComponents = SerializeUtils::Common::getNumComponents(mDesc.mFormat);
            uint32_t iComponentSize = SerializeUtils::Common::getBaseComponentSize(mDesc.mFormat);
            uint32_t iBytesPerRow = mDesc.miWidth * iNumComponents * iComponentSize;
            uint32_t iTotalSize = iBytesPerRow * mDesc.miHeight;
            
            mNativeBuffer = [device
             newBufferWithLength: iTotalSize
             options: MTLResourceOptionCPUCacheModeDefault];
            
            id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
            [commandEncoder
             copyFromTexture: mNativeImage
             sourceSlice: 0
             sourceLevel: 0
             sourceOrigin: origin
             sourceSize: size
             toBuffer: mNativeBuffer
             destinationOffset: 0
             destinationBytesPerRow: iBytesPerRow
             destinationBytesPerImage: iTotalSize];
            
            [mNativeBuffer setLabel: [NSString stringWithUTF8String: std::string(mID + " Texture Buffer").c_str()]];
            
            [commandEncoder endEncoding];
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
        }
    
        /*
        **
        */
        void CImage::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            [mNativeImage setLabel: [NSString stringWithUTF8String: id.c_str()]];
        }

        /*
        **
        */
        void* CImage::getNativeImage()
        {
            return (__bridge void*)mNativeImage;
        }

    }   // Metal

}   // RenderDriver
