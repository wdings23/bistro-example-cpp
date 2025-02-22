#pragma once

#include <render-driver/CommandQueue.h>
#include <render-driver/CommandBuffer.h>
#include <render-driver/Device.h>
#include <Metal/Metal.h>

#include <atomic>

namespace RenderDriver
{
    namespace Metal
    {
        class CCommandQueue : public RenderDriver::Common::CCommandQueue
        {
        public:
            struct CreateDesc : public RenderDriver::Common::CCommandQueue::CreateDesc
            {
            };
        public:
            CCommandQueue() = default;
            virtual ~CCommandQueue() = default;

            PLATFORM_OBJECT_HANDLE create(RenderDriver::Common::CCommandQueue::CreateDesc& desc);

            virtual void execCommandBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CDevice& device);

            virtual void execCommandBuffer2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CFence& fence,
                uint64_t iFenceValue,
                RenderDriver::Common::CDevice& device);
            
            virtual void execCommandBuffer3(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint64_t* piWaitValue,
                uint64_t* piSignalValue,
                RenderDriver::Common::CFence* pWaitFence,
                RenderDriver::Common::CFence* pSignalFence);
            
            virtual void setID(std::string const& id);

            virtual void* getNativeCommandQueue();
            
        protected:
            id<MTLCommandQueue>         mNativeCommandQueue;
            id<MTLDevice>               mNativeDevice;
        };

    }   // Metal

}   // RenderDriver
