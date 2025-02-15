#pragma once

#include <render-driver/Device.h>
#include <render-driver/Buffer.h>
#include <render-driver/DescriptorHeap.h>

#include <utils/serialize_utils.h>

namespace RenderDriver
{
    namespace Common
    {
        struct BufferViewDescriptor
        {
            RenderDriver::Common::Format            mFormat;
            RenderDriver::Common::CBuffer*          mpBuffer;
            RenderDriver::Common::CBuffer*          mpCounterBuffer = nullptr;
            uint32_t                                miSize;
            uint32_t                                miStructStrideSize = 0;
            UnorderedAccessViewFlags                mFlags;

            RenderDriver::Common::ResourceViewType  mViewType;

            CDescriptorHeap*                        mpDescriptorHeap;
            uint32_t                                miDescriptorOffset;

            CDescriptorSet*                         mpDescriptorSet;
        };

        class CBufferView : public CObject
        {
        public:
            CBufferView() = default;
            virtual ~CBufferView() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                BufferViewDescriptor& desc,
                CDevice& device);

        protected:
            BufferViewDescriptor    mDesc;

        };


    }   // DX12

}   // RenderDriver
