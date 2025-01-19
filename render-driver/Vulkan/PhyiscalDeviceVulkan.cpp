#include <render-driver/Vulkan/PhysicalDeviceVulkan.h>

#include <vector>


#include <wtfassert.h>

/*
**
*/
namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPhysicalDevice::create(Descriptor const& desc)
        {
            uint32_t iNumDevices = 0;
            VkInstance* pInstance = reinterpret_cast<VkInstance*>(desc.mpAppInstance);
            WTFASSERT(pInstance, "no given app instance");
            VkInstance& instance = *pInstance;

            vkEnumeratePhysicalDevices(instance, &iNumDevices, nullptr);

            std::vector<VkPhysicalDevice> aPhysicalDevices(iNumDevices);
            vkEnumeratePhysicalDevices(instance, &iNumDevices, aPhysicalDevices.data());

            // pick device
            VkPhysicalDeviceType deviceOrder[] =
            {
                VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
                VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
                VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
                VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU
            };
            uint32_t iPrevOrder = UINT32_MAX;
            for(auto const& physicalDevice : aPhysicalDevices)
            {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

                VkPhysicalDeviceFeatures deviceFeatures;
                vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

                VkPhysicalDeviceVulkan12Features vulkan12Features = {};
                vulkan12Features.drawIndirectCount = true;
                vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
                vulkan12Features.pNext = nullptr;

                VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
                deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
                deviceFeatures2.pNext = &vulkan12Features;
                vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);

                uint32_t queueFamilyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

                std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
                vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

                uint32_t iOrder = UINT32_MAX;
                for(uint32_t i = 0; i < sizeof(deviceOrder) / sizeof(*deviceOrder); i++)
                {
                    if(deviceProperties.deviceType == deviceOrder[i])
                    {
                        iOrder = i;
                        break;
                    }
                }

                if(iOrder < iPrevOrder)
                {
                    mPhysicalDevice = physicalDevice;
                    mPhyiscalDeviceProperties = deviceProperties;
                    mPhyiscalDeviceFeatures = deviceFeatures;

                    VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorTypeFeatures = {};
                    mutableDescriptorTypeFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
                    mutableDescriptorTypeFeatures.mutableDescriptorType = VK_TRUE;
                    mutableDescriptorTypeFeatures.pNext = &vulkan12Features;
                    
                    VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {};
                    indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
                    indexingFeatures.pNext = &mutableDescriptorTypeFeatures;

                    VkPhysicalDeviceFeatures2 deviceFeatures = {};
                    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                    deviceFeatures.pNext = &mutableDescriptorTypeFeatures; // &indexingFeatures;
                    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);

                    uint32_t iIndex = 0;
                    for(auto const& queueFamily : queueFamilies)
                    {
                        if(miGraphicsQueueFamilyIndex == UINT32_MAX && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
                        {
                            mGraphicsQueueFamily = queueFamily;
                            miGraphicsQueueFamilyIndex = iIndex;
                        }

                        if(miComputeQueueFamilyIndex == UINT32_MAX && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
                        {
                            mComputeQueueFamily = queueFamily;
                            miComputeQueueFamilyIndex = iIndex;
                        }

                        if(miTransferQueueFamilyIndex == UINT32_MAX && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT))
                        {
                            mTransferQueueFamily = queueFamily;
                            miTransferQueueFamilyIndex = iIndex;
                        }

                        ++iIndex;
                    }

                    iPrevOrder = iOrder;
                }
            }

            mpNativeDevice = &mPhysicalDevice;
            return mHandle;
        }
    
    }   // Vulkan

}   // RenderDriver