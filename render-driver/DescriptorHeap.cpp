#include <render-driver/DescriptorHeap.h>

namespace RenderDriver
{
    namespace Common
    {
        PLATFORM_OBJECT_HANDLE CDescriptorHeap::create(
            DescriptorHeapDescriptor const& desc,
            CDevice& device)
        {
            mHandle = device.assignHandleAndUpdate();
            mDesc = desc;

            return mHandle;
        }

        /*
        **
        */
        void* CDescriptorHeap::getNativeDescriptorHeap()
        {
            return nullptr;
        }

    }   // Common

}   // RenderDriver