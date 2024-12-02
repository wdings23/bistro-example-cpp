#include <render-driver/PhysicalDevice.h>

namespace RenderDriver
{
    namespace Common
    {
        PLATFORM_OBJECT_HANDLE CPhysicalDevice::create(Descriptor const& desc)
        {
            mHandle = 1;
            mDesc = desc;

            return mHandle;
        }

    }   // Common

}   // RenderDriver