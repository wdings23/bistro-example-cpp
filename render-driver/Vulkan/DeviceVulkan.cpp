#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/PhysicalDeviceVulkan.h>

#include <wtfassert.h>

#include <vulkan/vulkan.h>

#include <vector>

#define USE_RAY_TRACING 1

namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CDevice::create(RenderDriver::Common::CDevice::CreateDesc& createDesc)
        {
            RenderDriver::Common::CDevice::create(createDesc);
            mpPhysicalDevice = static_cast<RenderDriver::Vulkan::CPhysicalDevice*>(createDesc.mpPhyiscalDevice);
            VkPhysicalDevice* pNativePhysicalDevice = static_cast<VkPhysicalDevice*>(mpPhysicalDevice->getNativePhysicalDevice());

            RenderDriver::Vulkan::CDevice::CreateDesc& vulkanCreateDesc = static_cast<RenderDriver::Vulkan::CDevice::CreateDesc&>(createDesc);
            
            float fQueuePriority = 1.0f;
            uint32_t iQueueFamilyIndex = mpPhysicalDevice->getGraphicsQueueFamilyIndex();

#if defined(USE_RAY_TRACING)
            VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddresFeatures = {};
            enabledBufferDeviceAddresFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
            enabledBufferDeviceAddresFeatures.bufferDeviceAddress = VK_TRUE;
            enabledBufferDeviceAddresFeatures.pNext = nullptr;

            VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures = {};
            enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
            enabledRayTracingPipelineFeatures.pNext = nullptr;

            VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures = {};
            enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
            enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;
#endif // USE_RAY_TRACING

            VkPhysicalDeviceVulkan12Features vulkan12Features = {};
            vulkan12Features.drawIndirectCount = true;
            vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            
#if defined(USE_RAY_TRACING)
            vulkan12Features.pNext = &enabledAccelerationStructureFeatures;
#else 
            vulkan12Features.pNext = nullptr;
#endif // USE_RAY_TRACING

            VkPhysicalDeviceRobustness2FeaturesEXT robustness2 = {};
            robustness2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT;
            robustness2.pNext = &vulkan12Features;
            robustness2.nullDescriptor = VK_TRUE;
            robustness2.robustImageAccess2 = VK_TRUE;

            // disabled due to VkPhysicalDeviceVulkan12Features
            // descriptor indexing extension
            //VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexingFeatures = {};
            //indexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
            //indexingFeatures.pNext = &robustness2;
            //indexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
            //indexingFeatures.runtimeDescriptorArray = VK_TRUE;
            //
            // timeline semaphore
            //VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures = {};
            //timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
            //timelineSemaphoreFeatures.pNext = &sync2Features;
            //timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

            VkPhysicalDeviceSynchronization2Features sync2Features = {};
            sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
            sync2Features.pNext = &robustness2;
            sync2Features.synchronization2 = VK_TRUE;

            VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutableDescriptorTypeFeatures = {};
            mutableDescriptorTypeFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT;
            mutableDescriptorTypeFeatures.mutableDescriptorType = VK_TRUE;
            mutableDescriptorTypeFeatures.pNext = &sync2Features;

            VkPhysicalDeviceFeatures2 deviceFeatures2 = {};
            deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
            deviceFeatures2.pNext = &mutableDescriptorTypeFeatures;
            vkGetPhysicalDeviceFeatures2(*pNativePhysicalDevice, &deviceFeatures2);

            uint32_t const kiNumQueues = 4;
            float afQueuePriorities[kiNumQueues] = { 1.0f, 1.0f, 1.0f, 1.0f };
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = iQueueFamilyIndex;
            queueCreateInfo.queueCount = kiNumQueues;
            queueCreateInfo.pQueuePriorities = afQueuePriorities;

            VkPhysicalDeviceFeatures deviceFeatures = {};
            vkGetPhysicalDeviceFeatures(*pNativePhysicalDevice, &deviceFeatures);

            VkDeviceCreateInfo createDeviceInfo = {};
            createDeviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createDeviceInfo.pQueueCreateInfos = &queueCreateInfo;
            createDeviceInfo.queueCreateInfoCount = 1;
            createDeviceInfo.pEnabledFeatures = nullptr; // &deviceFeatures; disabled due to physical features 2 enabled  
            createDeviceInfo.enabledLayerCount = vulkanCreateDesc.miNumLayers;
            createDeviceInfo.ppEnabledLayerNames = vulkanCreateDesc.maLayerNames;
            createDeviceInfo.enabledExtensionCount = vulkanCreateDesc.miNumExtensions;
            createDeviceInfo.ppEnabledExtensionNames = vulkanCreateDesc.maExtensionNames;
            createDeviceInfo.pNext = &deviceFeatures2;

            VkResult ret = vkCreateDevice(*pNativePhysicalDevice, &createDeviceInfo, nullptr, &mDevice);
            WTFASSERT(ret == VK_SUCCESS, "error creating logical device: %d", ret);

            mHandle = 2;

            return mHandle;
        }

        /*
        **
        */
        void CDevice::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mDevice);
            objectNameInfo.objectType = VK_OBJECT_TYPE_DEVICE;
            objectNameInfo.pObjectName = id.c_str();

            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(mDevice, "vkSetDebugUtilsObjectNameEXT");
            setDebugUtilsObjectNameEXT(
                mDevice,
                &objectNameInfo);
        }

        /*
        **
        */
        void* CDevice::getNativeDevice()
        {
            return &mDevice;
        }

    }   // DX12

}   // RenderDriver