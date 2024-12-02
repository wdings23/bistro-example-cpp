#include <render-driver/AccelerationStructure.h>

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CAccelerationStructure::create(
            AccelerationStructureDescriptor const& desc,
            CDevice& device)
        {
            mDesc = desc;
            mHandle = device.assignHandleAndUpdate();

            return mHandle;
        }

    }   // Common

}   // RenderDriver