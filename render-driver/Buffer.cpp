#include <render-driver/Buffer.h>

namespace RenderDriver
{
    namespace Common
    {
        PLATFORM_OBJECT_HANDLE CBuffer::create(
            BufferDescriptor const& desc,
            CDevice& device)
        {
            mDesc = desc;
            mHandle = device.assignHandleAndUpdate();

            return mHandle;
        }

        /*
        **
        */
        void  CBuffer::copy(
            RenderDriver::Common::CBuffer& buffer,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iDestOffset,
            uint32_t iSrcOffset,
            uint32_t iDataSize)
        {
        }

        /*
        **
        */
        void* CBuffer::getNativeBuffer()
        {
            return nullptr;
        }

        /*
        **
        */
        uint64_t CBuffer::getGPUVirtualAddress()
        {
            return 0;
        }

        /*
        **
        */
        void CBuffer::setState(
            PLATFORM_OBJECT_HANDLE queueHandle, 
            RenderDriver::Common::ResourceStateFlagBits const& state)
        {
            mQueueResourceState[queueHandle] = state;
        }

        /*
        **
        */
        RenderDriver::Common::ResourceStateFlagBits CBuffer::getState(
            PLATFORM_OBJECT_HANDLE queueHandle)
        {
            return mQueueResourceState[queueHandle];
        }

        /*
        **
        */
        void CBuffer::setData(
            void* pSrcData,
            uint32_t iDataSize)
        {
        }

        /*
        **
        */
        void CBuffer::setDataOpen(
            void* pSrcData,
            uint32_t iDataSize)
        {
        }

        /*
        **
        */
        void CBuffer::releaseNativeBuffer()
        {

        }

        /*
        **
        */
        void* CBuffer::getMemoryOpen(uint32_t iDataSize)
        {
            return nullptr;
        }
        

    }   // Common

}   // RenderDriver