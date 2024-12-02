#pragma once

#include <render-driver/BufferView.h>
#include <render-driver/Vulkan/DeviceVulkan.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        class CBufferView : public RenderDriver::Common::CBufferView
        {
        public:
            CBufferView() = default;
            virtual ~CBufferView() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::BufferViewDescriptor& desc,
                RenderDriver::Common::CDevice& device);

            inline void* getNativeBufferView()
            {
                return &mNativeBufferView;
            }

            virtual void setID(std::string const& id);

        protected:
            VkBufferView                mNativeBufferView;
            VkDevice*                   mpNativeDevice;

        };


    }   // Common

}   // RenderDriver