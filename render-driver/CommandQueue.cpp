#include <render-driver/Device.h>
#include <render-driver/CommandQueue.h>

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandQueue::create(CreateDesc& desc)
        {
            mHandle = desc.mpDevice->assignHandleAndUpdate();
            mType = desc.mType;

            return mHandle;
        }

        void CCommandQueue::execCommandBuffer(
            CCommandBuffer& commandBuffer,
            CDevice& device)
        {

        }

        void CCommandQueue::execCommandBuffer3(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint64_t* piWaitValue,
            uint64_t* piSignalValue,
            RenderDriver::Common::CFence* pWaitFence,
            RenderDriver::Common::CFence* pSignalFence
        )
        {

        }

        void CCommandQueue::execCommandBufferSynchronized(
            CCommandBuffer& commandBuffer,
            CDevice& device,
            bool bWait)
        {

        }

        /*
        **
        */
        void* CCommandQueue::getNativeCommandQueue()
        {
            return nullptr;
        }

    }   // Common

}   // RenderDriver