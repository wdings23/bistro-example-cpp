#include <render-driver/BufferView.h>

namespace RenderDriver
{
    namespace Common
    {
        PLATFORM_OBJECT_HANDLE CBufferView::create(
            BufferViewDescriptor& desc,
            CDevice& device)
        {
            mHandle = device.assignHandleAndUpdate();
            mDesc = desc;

            return mHandle;
        }

    }   // Common

}   // RenderDriver