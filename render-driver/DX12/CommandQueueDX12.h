#pragma once

#include <render-driver/CommandQueue.h>
#include <render-driver/CommandBuffer.h>
#include <render-driver/Device.h>

namespace RenderDriver
{
    namespace DX12
    {
        class CCommandQueue : public RenderDriver::Common::CCommandQueue
        {
        public:
            CCommandQueue() = default;
            virtual ~CCommandQueue() = default;

            PLATFORM_OBJECT_HANDLE create(RenderDriver::Common::CCommandQueue::CreateDesc& desc);

            virtual void execCommandBuffer2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CFence& fence,
                uint64_t iFenceValue,
                RenderDriver::Common::CDevice& device);

            virtual void execCommandBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& id);

            virtual void* getNativeCommandQueue();

        protected:

            ComPtr<ID3D12CommandQueue>  mNativeCommandQueue;
        };

    }   // Common

}   // RenderDriver