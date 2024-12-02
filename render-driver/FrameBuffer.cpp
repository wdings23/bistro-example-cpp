#include <render-driver/FrameBuffer.h>

namespace RenderDriver
{
    namespace Common
    {
        PLATFORM_OBJECT_HANDLE CFrameBuffer::create(
            FrameBufferDescriptor const& desc,
            CDevice& device)
        {
            mDesc = desc;
            mHandle = device.assignHandleAndUpdate();

            return mHandle;
        }

    }   // Common

}   // RenderDriver
