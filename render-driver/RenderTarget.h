#pragma once

#include <render-driver/Device.h>
#include <render-driver/Format.h>
#include <render-driver/Image.h>
#include <render-driver/ImageView.h>

#include <string>

namespace RenderDriver
{
    namespace Common
    {
        struct RenderTargetDescriptor
        {
            uint32_t                                        miWidth;
            uint32_t                                        miHeight;
            Format                                          mColorFormat;
            Format                                          mDepthStencilFormat;
            float                                           mafClearColor[4];
            float                                           mafClearDepthStencil[4];
            RenderDriver::Common::CDescriptorHeap*          mpColorDescriptorHeap;
            RenderDriver::Common::CDescriptorHeap*          mpDepthStencilDescriptorHeap;

            bool                                            mbComputeShaderWritable = false;
            uint32_t                                        miShaderResourceIndex = UINT32_MAX;
        };

        class CRenderTarget : public CObject
        {
        public:
            CRenderTarget() = default;
            virtual ~CRenderTarget() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::RenderTargetDescriptor desc,
                RenderDriver::Common::CDevice& device);

            std::unique_ptr<RenderDriver::Common::CImage>& getImage();

            PLATFORM_OBJECT_HANDLE getColorDescriptorHeapHandle();
            PLATFORM_OBJECT_HANDLE getDepthStencilDescriptorHeapHandle();

            virtual PLATFORM_OBJECT_HANDLE createImageView(
                RenderDriver::Common::ImageViewDescriptor const& imageViewDesc,
                RenderDriver::Common::CDevice& device);

            inline RenderTargetDescriptor getDescriptor()
            {
                return mDesc;
            }

            inline RenderDriver::Common::CImageView* getColorImageView()
            {
                return mpColorImageView.get();
            }

            inline RenderDriver::Common::CImageView* getDepthStencilImageView()
            {
                return mpDepthStencilImageView.get();
            }

        protected:

            std::unique_ptr<RenderDriver::Common::CImage>                            mpColorImage;
            std::unique_ptr<RenderDriver::Common::CImage>                            mpDepthStencilImage;

            std::unique_ptr<RenderDriver::Common::CImageView>                        mpColorImageView;
            std::unique_ptr<RenderDriver::Common::CImageView>                        mpDepthStencilImageView;

            RenderTargetDescriptor                                                   mDesc;
                                                                                     
            PLATFORM_OBJECT_HANDLE                                                   mColorImageHandle;
            PLATFORM_OBJECT_HANDLE                                                   mColorImageViewHandle;
            PLATFORM_OBJECT_HANDLE                                                   mDepthStencilImageHandle;
            PLATFORM_OBJECT_HANDLE                                                   mDepthStencilImageViewHandle;

            std::vector<std::unique_ptr<RenderDriver::Common::CImageView>>          mapAdditionalImageViews;
        };

    }   // Common

}   // RenderDriver