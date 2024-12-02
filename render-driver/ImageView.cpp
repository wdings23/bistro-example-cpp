#include <render-driver/ImageView.h>

namespace RenderDriver
{
    namespace Common
    {
        PLATFORM_OBJECT_HANDLE CImageView::create(
            ImageViewDescriptor const& descriptor,
            CDevice& device)
        {
            mHandle = device.assignHandleAndUpdate();
            mDesc = descriptor;

            return mHandle;
        }


    }   // Common

}   // RenderDriver