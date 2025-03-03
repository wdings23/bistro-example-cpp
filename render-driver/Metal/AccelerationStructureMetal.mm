#include <render-driver/Metal/DeviceMetal.h>
#include <render-driver/Metal/AccelerationStructureMetal.h>
#include <render-driver/Metal/BufferMetal.h>


namespace RenderDriver
{
    namespace Metal
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CAccelerationStructure::create(
            AccelerationStructureDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CAccelerationStructure::create(desc, device);
            return mHandle;
        }

        /*
        **
        */
        void CAccelerationStructure::setNativeAccelerationStructure(void* pData)
        {
            mAccelerationStructure = (__bridge_transfer id<MTLAccelerationStructure>)pData;
        }

        /*
        **
        */
        void* CAccelerationStructure::getNativeAccelerationStructure()
        {
            return (__bridge void*)mAccelerationStructure;
        }

    }   // Metal

}   // RenderDriver
