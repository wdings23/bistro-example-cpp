#pragma once

#include <render-driver/Device.h>
#include <render-driver/AccelerationStructure.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        struct AccelerationStructureDescriptor : public RenderDriver::Common::AccelerationStructureDescriptor
        {
            void* mpInstance;
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

            VkAccelerationStructureKHR                  mTopLevelAccelerationStructure;
           
        };
    }
}