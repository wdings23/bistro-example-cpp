
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

            /*
            **
            */
            static char sacData[256];
            char const* getErrorCode(
                VkResult result)
            {
#define VK_ERROR_CODE_STR(result) \
        case VK_ ##result: { strcpy(sacData, #result); break; } 

                switch(result)
                {
                    VK_ERROR_CODE_STR(SUCCESS)
                    VK_ERROR_CODE_STR(NOT_READY)
                    VK_ERROR_CODE_STR(TIMEOUT)
                    VK_ERROR_CODE_STR(EVENT_SET)
                    VK_ERROR_CODE_STR(EVENT_RESET)
                    VK_ERROR_CODE_STR(INCOMPLETE)
                    VK_ERROR_CODE_STR(ERROR_OUT_OF_HOST_MEMORY)
                    VK_ERROR_CODE_STR(ERROR_OUT_OF_DEVICE_MEMORY)
                    VK_ERROR_CODE_STR(ERROR_INITIALIZATION_FAILED)
                    VK_ERROR_CODE_STR(ERROR_DEVICE_LOST)
                    VK_ERROR_CODE_STR(ERROR_MEMORY_MAP_FAILED)
                    VK_ERROR_CODE_STR(ERROR_LAYER_NOT_PRESENT)
                    VK_ERROR_CODE_STR(ERROR_EXTENSION_NOT_PRESENT)
                    VK_ERROR_CODE_STR(ERROR_FEATURE_NOT_PRESENT)
                    VK_ERROR_CODE_STR(ERROR_INCOMPATIBLE_DRIVER)
                    VK_ERROR_CODE_STR(ERROR_TOO_MANY_OBJECTS)
                    VK_ERROR_CODE_STR(ERROR_FORMAT_NOT_SUPPORTED)
                    VK_ERROR_CODE_STR(ERROR_FRAGMENTED_POOL)
                    VK_ERROR_CODE_STR(ERROR_UNKNOWN)
                    VK_ERROR_CODE_STR(ERROR_OUT_OF_POOL_MEMORY)
                    VK_ERROR_CODE_STR(ERROR_INVALID_EXTERNAL_HANDLE)
                    VK_ERROR_CODE_STR(ERROR_FRAGMENTATION)
                    VK_ERROR_CODE_STR(ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
                    VK_ERROR_CODE_STR(PIPELINE_COMPILE_REQUIRED)
                    VK_ERROR_CODE_STR(ERROR_SURFACE_LOST_KHR)
                    VK_ERROR_CODE_STR(ERROR_NATIVE_WINDOW_IN_USE_KHR)
                    VK_ERROR_CODE_STR(SUBOPTIMAL_KHR)
                    VK_ERROR_CODE_STR(ERROR_OUT_OF_DATE_KHR)
                    VK_ERROR_CODE_STR(ERROR_INCOMPATIBLE_DISPLAY_KHR)
                    VK_ERROR_CODE_STR(ERROR_VALIDATION_FAILED_EXT)
                    VK_ERROR_CODE_STR(ERROR_INVALID_SHADER_NV)
                    VK_ERROR_CODE_STR(ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR)
                    VK_ERROR_CODE_STR(ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR)
                    VK_ERROR_CODE_STR(ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR)
                    VK_ERROR_CODE_STR(ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR)
                    VK_ERROR_CODE_STR(ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR)
                    VK_ERROR_CODE_STR(ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR)
                    VK_ERROR_CODE_STR(ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
                    VK_ERROR_CODE_STR(ERROR_NOT_PERMITTED_KHR)
                    VK_ERROR_CODE_STR(ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
                    VK_ERROR_CODE_STR(THREAD_IDLE_KHR)
                    VK_ERROR_CODE_STR(THREAD_DONE_KHR)
                    VK_ERROR_CODE_STR(OPERATION_DEFERRED_KHR)
                    VK_ERROR_CODE_STR(OPERATION_NOT_DEFERRED_KHR)
                    VK_ERROR_CODE_STR(ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR)
                    VK_ERROR_CODE_STR(ERROR_COMPRESSION_EXHAUSTED_EXT)
                    VK_ERROR_CODE_STR(INCOMPATIBLE_SHADER_BINARY_EXT)
                    VK_ERROR_CODE_STR(PIPELINE_BINARY_MISSING_KHR)
                    VK_ERROR_CODE_STR(ERROR_NOT_ENOUGH_SPACE_KHR)
                    VK_ERROR_CODE_STR(RESULT_MAX_ENUM)

                default:
                    {
                        strcpy(sacData, "SUCCESS");
                    }
                }
#undef VK_ERROR_CODE_STR

                return sacData;
            }


        }   // Utils

    }   // Vulkan

}   // RenderDriver
