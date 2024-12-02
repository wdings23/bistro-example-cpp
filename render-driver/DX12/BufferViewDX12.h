#pragma once

#include <render-driver/BufferView.h>
#include <render-driver/DX12/DeviceDX12.h>

namespace RenderDriver
{
    namespace DX12
    {
        class CBufferView : public RenderDriver::Common::CBufferView
        {
        public:
            CBufferView() = default;
            virtual ~CBufferView() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::BufferViewDescriptor& desc,
                RenderDriver::Common::CDevice& device);
        };


    }   // Common

}   // RenderDriver