#pragma once

#include <RenderDriver/Device.h>
#include <RenderDriver/DescriptorSet.h>

namespace RenderDriver
{
    namespace Metal
    {
        class CDescriptorSet : public RenderDriver::Common::CDescriptorSet
        {
        public:

            CDescriptorSet() = default;
            virtual ~CDescriptorSet() = default;

            PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::DescriptorSetDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& name);

            inline id<MTLBuffer> getRootConstant()
            {
                return mRootConstant;
            }
            
        protected:
            id<MTLBuffer>               mRootConstant;
            id<MTLDevice>               mNativeDevice;
        };

    }   // Metal

}   // RenderDriver
