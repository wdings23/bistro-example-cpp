#include <render-driver/Fence.h>

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CFence::create(
            FenceDescriptor const& desc,
            CDevice& device)
        {
            mDesc = desc;
            mHandle = device.assignHandleAndUpdate();

            return mHandle;
        }

        /*
        **
        */
        void CFence::place(PlaceFenceDescriptor const& desc)
        {

        }

        /*
        **
        */
        bool CFence::isCompleted()
        {
            return true;
        }

        /*
        **
        */
        void CFence::waitCPU(
            uint64_t iMilliseconds, 
            uint64_t iWaitValue)
        {
        }

        /*
        **
        */
        void CFence::waitGPU(
            RenderDriver::Common::CCommandQueue* pCommandQueue, 
            RenderDriver::Common::CFence* pFence,
            uint64_t iWaitValue)
        {
        }

        /*
        **
        */
        void CFence::waitCPU2(
            uint64_t iMilliseconds,
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t iWaitValue)
        {
        }

        /*
        **
        */
        void CFence::reset(RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
        }

        /*
        **
        */
        void CFence::reset2()
        {
        }

        /*
        **
        */
        uint64_t CFence::getFenceValue()
        {
            return UINT64_MAX;
        }

        /*
        **
        */
        void* CFence::getNativeFence()
        {
            return nullptr;
        }

        /*
        **
        */
        void CFence::signal(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t const& iWaitFenceValue)
        {
        }

        /*
        **
        */
        void CFence::waitCPU(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t iFenceValue)
        {
        }

        /*
        **
        */
        void CFence::waitGPU()
        {
        }
    }
}
