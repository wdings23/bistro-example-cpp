#include <render-driver/Image.h>
#include <render-driver/Device.h>

namespace RenderDriver
{
    namespace Common
    {
        PLATFORM_OBJECT_HANDLE CImage::create(
            ImageDescriptor const& desc,
            CDevice& device)
        {
            mDesc = desc;
            mHandle = device.assignHandleAndUpdate();

            return mHandle;
        }

        /*
        **
        */
        void* CImage::getNativeImage()
        {
            return nullptr;
        }

    }   // Common

}   // RenderDriver