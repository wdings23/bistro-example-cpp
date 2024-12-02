#pragma once

#include <render-driver/PhysicalDevice.h>
#include <render-driver/Device.h>

#include <vulkan/vulkan.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        class CPhysicalDevice : public RenderDriver::Common::CPhysicalDevice
        {
        public:

            CPhysicalDevice() = default;
            virtual ~CPhysicalDevice() = default;

            virtual PLATFORM_OBJECT_HANDLE create(Descriptor const& desc);

            inline uint32_t getGraphicsQueueFamilyIndex()
            {
                return miGraphicsQueueFamilyIndex;
            }
            
            inline uint32_t getPresentQueueFamilyIndex()
            {
                return miPresentQueueFamilyIndex;
            }

            inline uint32_t getComputeQueueFamilyIndex()
            {
                return miComputeQueueFamilyIndex;
            }

            inline uint32_t getTransferFamilyIndex()
            {
                return miTransferQueueFamilyIndex;
            }

            inline void* getNativePhysicalDevice()
            {
                return &mPhysicalDevice;
            }

            inline VkPhysicalDeviceDescriptorIndexingFeaturesEXT const& getIndexingFeatures()
            {
                return mIndexingFeatures;
            }

        protected:
            VkPhysicalDevice                                            mPhysicalDevice;
            VkPhysicalDeviceProperties                                  mPhyiscalDeviceProperties;
            VkPhysicalDeviceFeatures                                    mPhyiscalDeviceFeatures;
            
            VkQueueFamilyProperties                                     mGraphicsQueueFamily;
            VkQueueFamilyProperties                                     mComputeQueueFamily;
            VkQueueFamilyProperties                                     mTransferQueueFamily;

            VkPhysicalDeviceDescriptorIndexingFeaturesEXT               mIndexingFeatures;

            uint32_t                                                    miGraphicsQueueFamilyIndex = UINT32_MAX;
            uint32_t                                                    miComputeQueueFamilyIndex = UINT32_MAX;
            uint32_t                                                    miTransferQueueFamilyIndex = UINT32_MAX;
            uint32_t                                                    miPresentQueueFamilyIndex = UINT32_MAX;

            float                                                       mfQueuePriority = 1.0f;

        };

    }   // DX12

}   // RenderDriver