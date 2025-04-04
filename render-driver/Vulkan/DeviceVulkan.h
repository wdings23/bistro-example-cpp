#pragma once

#include <render-driver/Device.h>
#include <render-driver/Vulkan/PhysicalDeviceVulkan.h>

#include <memory>

#if defined(_MSC_VER)
#include <vulkan/vulkan.h>
#endif // _MSC_VER

namespace RenderDriver
{
    namespace Vulkan
    {
        class CDevice : public RenderDriver::Common::CDevice
        {
        public:
            struct CreateDesc : public RenderDriver::Common::CDevice::CreateDesc
            {
                char* const*                maLayerNames;
                uint32_t                    miNumLayers;

                char* const*                maExtensionNames;
                uint32_t                    miNumExtensions;

            };

        public:
            CDevice() = default;
            virtual ~CDevice() = default;

            virtual PLATFORM_OBJECT_HANDLE create(RenderDriver::Common::CDevice::CreateDesc& createDesc);

            virtual void* getNativeDevice();

            virtual void setID(std::string const& name);

            inline RenderDriver::Vulkan::CPhysicalDevice* getPhysicalDevice()
            {
                return mpPhysicalDevice;
            }

        protected:
            VkDevice                                        mDevice;
            
            VkQueue                                         mGraphicsQueue;
            VkQueue                                         mPresentQueue;

            RenderDriver::Vulkan::CPhysicalDevice*          mpPhysicalDevice;
        };

    }   // Vulkan

}   // RenderDriver
