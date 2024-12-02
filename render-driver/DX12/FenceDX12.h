#pragma once

#include <render-driver/Fence.h>
#include <render-driver/DX12/CommandQueueDX12.h>

namespace RenderDriver
{
    namespace DX12
    {
        class CFence : public RenderDriver::Common::CFence
        {
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

            virtual void waitCPU2(uint64_t iMilliseconds, RenderDriver::Common::CCommandQueue* pCommandQueue, uint64_t iWaitValue = UINT64_MAX);
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

            inline HANDLE getEvent()
            {
                return mFenceEvent;
            }

            HANDLE                                  mFenceEvent;

        protected:
            ComPtr<ID3D12Fence>                     mNativeFence;
            
            uint64_t                                miFenceValue;
        };

    }   // Common

}   // RenderDriver