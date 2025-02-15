#include <render-driver/Metal/CommandAllocatorMetal.h>
#include <render-driver/Metal/DeviceMetal.h>

#include <utils/wtfassert.h>

namespace RenderDriver
{
    namespace Metal
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandAllocator::create(
            RenderDriver::Common::CommandAllocatorDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CCommandAllocator::create(desc, device);

            RenderDriver::Metal::CDevice& deviceMetal = static_cast<RenderDriver::Metal::CDevice&>(device);
            mNativeDevice = (__bridge id<MTLDevice>)device.getNativeDevice();
            
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
        void CCommandAllocator::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
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
