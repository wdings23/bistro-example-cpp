#pragma once

#include <RenderDriver/Fence.h>
#include <RenderDriver/Metal/CommandQueueMetal.h>

namespace RenderDriver
{
    namespace Metal
    {
        class CFence : public RenderDriver::Common::CFence
        {
        public:
            struct FenceDescriptor : public RenderDriver::Common::FenceDescriptor
            {
                RenderDriver::Common::CCommandBuffer*           mpCommandBuffer = nullptr;
            };
        public:
            CFence() = default;
            virtual ~CFence() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::FenceDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void place(RenderDriver::Common::PlaceFenceDescriptor const& desc);

            virtual bool isCompleted();
            virtual void waitCPU(uint64_t iMilliseconds, uint64_t iWaitValue = UINT64_MAX);
            virtual void waitGPU(
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                RenderDriver::Common::CFence* pSignalFence,
                uint64_t iWaitValue = UINT64_MAX);

            virtual void waitCPU2(
                uint64_t iMilliseconds,
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                RenderDriver::Common::CCommandBuffer* pCommandBuffer,
                uint64_t iWaitValue = UINT64_MAX);
            virtual void reset(RenderDriver::Common::CCommandQueue* pCommandQueue);

            virtual void setID(std::string const& name);

            virtual uint64_t getFenceValue();

            virtual void* getNativeFence();

            virtual void signal(
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                uint64_t const& iWaitFenceValue);
            virtual void waitCPU(
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                uint64_t iFenceValue);
            virtual void waitGPU();

            inline id<MTLEvent> getEvent()
            {
                return mEvent;
            }
            
        protected:
            
            uint64_t                                miFenceValue;
            id<MTLDevice>                           mNativeDevice;
            
            id<MTLFence>                            mNativeFence;
            id<MTLEvent>                            mEvent;
            
            RenderDriver::Common::CCommandBuffer*   mpCommandBuffer;
        };

    }   // Common

}   // RenderDriver
