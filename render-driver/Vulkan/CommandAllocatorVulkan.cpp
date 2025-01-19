#include <render-driver/Vulkan/CommandAllocatorVulkan.h>
#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>

#include <wtfassert.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandAllocator::create(
            RenderDriver::Common::CommandAllocatorDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CCommandAllocator::create(desc, device);

            RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
            VkDevice& nativeDevice = *(reinterpret_cast<VkDevice*>(deviceVulkan.getNativeDevice()));
            
            RenderDriver::Vulkan::CPhysicalDevice* pPhysicalDeviceVulkan = static_cast<RenderDriver::Vulkan::CPhysicalDevice*>(deviceVulkan.getPhysicalDevice());
            VkPhysicalDevice* pNativePhysicalDevice = static_cast<VkPhysicalDevice*>(pPhysicalDeviceVulkan->getNativePhysicalDevice());

            uint32_t iQueueFamilyIndex = pPhysicalDeviceVulkan->getGraphicsQueueFamilyIndex();
            if(desc.mType == RenderDriver::Common::CommandBufferType::Compute)
            {
                iQueueFamilyIndex = pPhysicalDeviceVulkan->getComputeQueueFamilyIndex();
            }
            else if(desc.mType == RenderDriver::Common::CommandBufferType::Copy)
            {
                iQueueFamilyIndex = pPhysicalDeviceVulkan->getTransferFamilyIndex();
            }

            VkCommandPoolCreateInfo poolInfo = {};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = iQueueFamilyIndex;

            VkResult ret = vkCreateCommandPool(nativeDevice, &poolInfo, nullptr, &mNativeCommandAllocator);
            WTFASSERT(ret == VK_SUCCESS, "Error creating allocator: %s", Utils::getErrorCode(ret));

            return mHandle;
        }

        /*
        **
        */
        void CCommandAllocator::reset()
        {
           
        }

        /*
        **
        */
        void CCommandAllocator::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
        }

        /*
        **
        */
        void* CCommandAllocator::getNativeCommandAllocator()
        {
            return &mNativeCommandAllocator;
        }

    }   // Common

}   // RenderDriver