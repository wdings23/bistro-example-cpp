#pragma once

#include <render-driver/Device.h>
#include <render-driver/AccelerationStructure.h>

namespace RenderDriver
{
    namespace Metal
    {
        struct AccelerationStructureDescriptor : public RenderDriver::Common::AccelerationStructureDescriptor
        {
        };

        class CAccelerationStructure : public RenderDriver::Common::CAccelerationStructure
        {
        public:
            CAccelerationStructure() = default;
            virtual ~CAccelerationStructure() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                AccelerationStructureDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void* getNativeAccelerationStructure();

            virtual void setNativeAccelerationStructure(void* pData);

        protected:
            AccelerationStructureDescriptor             mDesc;

            id<MTLAccelerationStructure>                mCompactedAccelerationStructure;
            id<MTLAccelerationStructure>                mAccelerationStructure;
           
        };
    }
}
