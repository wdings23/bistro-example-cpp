#pragma once
#pragma once

#include <render-driver/Device.h>
#include <render-driver/PipelineState.h>
#include <render-driver/RenderTarget.h>

namespace RenderDriver
{
    namespace Common
    {
        struct FrameBufferDescriptor
        {
            std::vector<RenderDriver::Common::CImageView*>*             mpaImageView = nullptr;
            RenderDriver::Common::CPipelineState*                       mpPipelineState = nullptr;
            std::vector<RenderDriver::Common::CRenderTarget*>*          mpaRenderTargets = nullptr;
        };

        struct FrameBufferDescriptor2
        {
            std::vector<RenderDriver::Common::CImageView*>* mpaImageView = nullptr;
            RenderDriver::Common::CPipelineState* mpPipelineState = nullptr;
            std::pair<RenderDriver::Common::CImageView*, uint32_t> mDepthView;
            uint32_t miWidth;
            uint32_t miHeight;
        };

        class CFrameBuffer : public CObject
        {
        public:
            CFrameBuffer() = default;
            virtual ~CFrameBuffer() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                FrameBufferDescriptor const& desc,
                CDevice& device) = 0;

            virtual void create2(
                FrameBufferDescriptor2 const& desc,
                CDevice& device) = 0;

            inline FrameBufferDescriptor& getDescriptor()
            {
                return mDesc;
            }

            virtual void* getNativeFrameBuffer() = 0;

        protected:
            FrameBufferDescriptor                                    mDesc;
        };

    }   // Common

}   // RenderDriver