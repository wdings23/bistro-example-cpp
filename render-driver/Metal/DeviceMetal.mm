#include "DeviceMetal.h"

namespace RenderDriver
{
    namespace Metal
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CDevice::create(CreateDesc& createDesc)
        {
            RenderDriver::Common::CDevice::create(createDesc);

            mNativeDevice = MTLCreateSystemDefaultDevice();
            
            mHandle = 2;

            return mHandle;
        }

        /*
        **
        */
        void CDevice::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            
        }

        /*
        **
        */
        void* CDevice::getNativeDevice()
        {
            return (__bridge void*)mNativeDevice;
        }

    }   // DX12

}   // RenderDriver
