
#include <render-driver/Vulkan/UtilsVulkan.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        namespace Utils
        {
            /*
            **
            */
            uint32_t findMemoryType(
                uint32_t iTypeFilter, 
                VkMemoryPropertyFlags properties,
                VkPhysicalDevice& physicalDevice)
            {
                uint32_t iRet = UINT32_MAX;

                VkPhysicalDeviceMemoryProperties memProperties;
                vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

                for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
                {
                    if((iTypeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                    {
                        iRet = i;
                        break;
                    }
                }

                return iRet;
            }

        }   // Utils

    }   // Vulkan

}   // RenderDriver
