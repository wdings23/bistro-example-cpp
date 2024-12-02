#pragma once

#include <render-driver/Device.h>
#include <render-driver/Object.h>

namespace RenderDriver
{
    namespace Common
    {
        struct AccelerationStructureDescriptor
        {

        };

        class CAccelerationStructure : public CObject
        {
        public:
            CAccelerationStructure() = default;
            virtual ~CAccelerationStructure() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                AccelerationStructureDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void* getNativeAccelerationStructure()
            {
                return nullptr;
            }

            virtual void setNativeAccelerationStructure(void* pData)
            {
            }

        protected:
            AccelerationStructureDescriptor             mDesc;
        };
    }
}