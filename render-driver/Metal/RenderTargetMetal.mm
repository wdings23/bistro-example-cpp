#include <RenderDriver/Metal/RenderTargetMetal.h>

#include <wtfassert.h>

namespace RenderDriver
{
    namespace Metal
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CRenderTarget::create(
            RenderDriver::Common::RenderTargetDescriptor desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CRenderTarget::create(desc, device);

            mpColorImage = std::make_unique<RenderDriver::Metal::CImage>();

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
            
            if(!desc.mbComputeShaderWritable)
            {
                if(desc.mDepthStencilFormat != RenderDriver::Common::Format::UNKNOWN)
                {
                    mpDepthStencilImage = std::make_unique<RenderDriver::Metal::CImage>();
                    
                    imageDesc.mFormat = desc.mDepthStencilFormat;
                    uint32_t iResourceFlagBits = (static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowDepthStencil) | static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowRenderTarget));
                    imageDesc.mResourceFlags = static_cast<RenderDriver::Common::ResourceFlagBits>(iResourceFlagBits);
                    mDepthStencilImageHandle = mpDepthStencilImage->create(imageDesc, device);

                    RenderDriver::Metal::CDevice& MetalDevice = static_cast<RenderDriver::Metal::CDevice&>(device);
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
            return UINT64_MAX;
        }


    }   // Metal

}   // RenderDriver
