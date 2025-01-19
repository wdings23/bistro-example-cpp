#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>

#include <render-driver/Image.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        namespace Utils
        {
            struct ImageLayoutTransition
            {
                RenderDriver::Common::CImage*           mpImage = nullptr;
                RenderDriver::Common::ImageLayout       mBefore = RenderDriver::Common::ImageLayout::UNDEFINED;
                RenderDriver::Common::ImageLayout       mAfter = RenderDriver::Common::ImageLayout::UNDEFINED;
            };

            uint32_t findMemoryType(
                uint32_t iTypeFilter,
                VkMemoryPropertyFlags properties,
                VkPhysicalDevice& physicalDevice);

            char const* getErrorCode(
                VkResult result);

        }   // Utils

    }   // Vulkan

}   // RenderDriver