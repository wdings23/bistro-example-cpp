#pragma once

#include <render-driver/Metal/DeviceMetal.h>
#include <render-driver/CommandAllocator.h>


namespace RenderDriver
{
    namespace Metal
    {
        class CCommandAllocator : public RenderDriver::Common::CCommandAllocator
        {
        public:
            CCommandAllocator() = default;
            virtual ~CCommandAllocator() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::CommandAllocatorDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void reset();

            virtual void setID(std::string const& id);

            virtual void* getNativeCommandAllocator();

        protected:
            id<MTLDevice>           mNativeDevice;
        };


    }   // Vulkan

}   // RenderDriver
