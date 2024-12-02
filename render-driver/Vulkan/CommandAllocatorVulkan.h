#pragma once

#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/CommandAllocator.h>


namespace RenderDriver
{
    namespace Vulkan
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
            VkCommandPool       mNativeCommandAllocator;
        };


    }   // Vulkan

}   // RenderDriver
