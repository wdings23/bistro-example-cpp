#pragma once

#include <render-driver/DescriptorHeap.h>
#include <render-driver/Vulkan/DeviceVulkan.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        class CDescriptorHeap : public RenderDriver::Common::CDescriptorHeap
        {
        public:
            CDescriptorHeap() = default;
            virtual ~CDescriptorHeap() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::DescriptorHeapDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& name);

            virtual void* getNativeDescriptorHeap();

            virtual uint64_t getCPUHandle(uint32_t iIndex, RenderDriver::Common::CDevice& device);
            virtual uint64_t getGPUHandle(uint32_t iIndex, RenderDriver::Common::CDevice& device);

            virtual void setImageView(
                RenderDriver::Common::CImage* pImage,
                uint32_t iIndex,
                RenderDriver::Common::CDevice& device);

        protected:
            VkDescriptorPool                         mNativeDescriptorPool;
        };

    }   // Vulkan

}   // RenderDriver