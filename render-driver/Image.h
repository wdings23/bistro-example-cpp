#pragma once

#include <render-driver/Device.h>
#include <render-driver/Format.h>
#include <utils/serialize_utils.h>

namespace RenderDriver
{
    namespace Common
    {
        struct ImageDescriptor
        {
            uint32_t                    miWidth;
            uint32_t                    miHeight;
            Format                      mFormat;
            ResourceStateFlagBits       mStateFlags;
            HeapType                    mHeapType;
            CPUPageProperty             mCPUPageProperty;
            TextureLayout               mLayout;
            ResourceFlagBits            mResourceFlags;
            uint32_t                    miNumImages;
            float const*                mafClearColor;
            ImageLayout                 mImageLayout;
        };

        class CImage : public CObject
        {
        public:
            CImage() = default;
            virtual ~CImage() = default;

            inline RenderDriver::Common::Format getFormat()
            {
                return mDesc.mFormat;
            }

            inline RenderDriver::Common::ImageDescriptor& getDescriptor()
            {
                return mDesc;
            }

            virtual PLATFORM_OBJECT_HANDLE create(
                ImageDescriptor const& desc, 
                CDevice& device);

            virtual void* getNativeImage();

        protected:
            ImageDescriptor   mDesc;
        };


    }   // Common

}   // RenderDriver
