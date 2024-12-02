#pragma once

#include <render-driver/Object.h>
#include <render-driver/Device.h>


namespace RenderDriver
{
    namespace Common
    {
        struct FenceDescriptor
        {
        };

        enum FenceType
        {
            CPU = 0,
            GPU,
        };

        struct PlaceFenceDescriptor
        {
            RenderDriver::Common::CCommandQueue*    mpCommandQueue;
            uint64_t                                miFenceValue;
            FenceType                               mType;
        };

        class CFence : public CObject
        {
        public:
            CFence() = default;
            virtual ~CFence() = default;

            virtual PLATFORM_OBJECT_HANDLE create(FenceDescriptor const& desc, CDevice& device);
            virtual void place(PlaceFenceDescriptor const& desc);
            virtual bool isCompleted();
            virtual void waitCPU(uint64_t iMilliseconds, uint64_t iWaitValue = UINT64_MAX);
            virtual void waitGPU(
                RenderDriver::Common::CCommandQueue* pCommandQueue, 
                RenderDriver::Common::CFence* pSignalFence,
                uint64_t iWaitValue = UINT64_MAX);

            virtual void waitCPU2(uint64_t iMilliseconds, RenderDriver::Common::CCommandQueue* pCommandQueue, uint64_t iWaitValue = UINT64_MAX);
            virtual void reset(RenderDriver::Common::CCommandQueue* pCommandQueue);
            virtual void reset2();

            virtual uint64_t getFenceValue();

            virtual void* getNativeFence();

            virtual void signal(
                RenderDriver::Common::CCommandQueue* pCommandQueue, 
                uint64_t const& iWaitFenceValue);
            virtual void waitCPU(
                RenderDriver::Common::CCommandQueue* pCommandQueue,
                uint64_t iFenceValue);
            virtual void waitGPU();

            inline uint64_t getSignalValue()
            {
                return mPlaceFenceDesc.miFenceValue;
            }

        protected:
            FenceDescriptor                 mDesc;
            PlaceFenceDescriptor            mPlaceFenceDesc;
        };
    }
}
