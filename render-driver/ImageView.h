#pragma once

#include <render-driver/Device.h>
#include <render-driver/Image.h>
#include <render-driver/DescriptorHeap.h>
#include <render-driver/DescriptorSet.h>

namespace RenderDriver
{
    namespace Common
    {
        struct ImageViewDescriptor
        {
            
            RenderDriver::Common::ResourceViewType  mViewType;
            RenderDriver::Common::Dimension         mDimension;
            RenderDriver::Common::ShaderResourceViewDimension   mShaderResourceViewDimension;
            RenderDriver::Common::Format            mFormat;
            uint32_t                                miTotalSize;
            uint32_t                                miElementSize;
            CDescriptorHeap*                        mpDescriptorHeap;
            CImage*                                 mpImage;
            CBuffer*                                mpBuffer;
            uint32_t                                miShaderResourceIndex;
            uint32_t                                miNumImages = 1;
            CDescriptorSet*                         mpDescriptorSet;
        };

        class CImageView : public CObject
        {
        public:
            CImageView() = default;
            virtual ~CImageView() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                ImageViewDescriptor const& desc,
                CDevice& device);
           
        protected:
            ImageViewDescriptor         mDesc;
        };


    }   // Common

}   // RenderDriver
