#include <render-driver/CommandAllocator.h>
#include <render-driver/Device.h>

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandAllocator::create(
            CommandAllocatorDescriptor const& desc,
            CDevice& device)
        {
            mDesc = desc;
            mHandle = device.assignHandleAndUpdate();

            return mHandle;
        }

        /*
        **
        */
        void CCommandAllocator::reset()
        {

        }

        /*
        **
        */
        void* CCommandAllocator::getNativeCommandAllocator()
        {
            return nullptr;
        }

    }   // Common

}   // RenderDriver