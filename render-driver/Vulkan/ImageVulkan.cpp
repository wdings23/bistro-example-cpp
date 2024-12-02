#include <render-driver/Vulkan/ImageVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>

#include <serialize_utils.h>
#include <wtfassert.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CImage::create(
            RenderDriver::Common::ImageDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            bool bIsDepthStencil = (
                desc.mFormat == RenderDriver::Common::Format::D16_UNORM ||
                desc.mFormat == RenderDriver::Common::Format::D24_UNORM_S8_UINT ||
                desc.mFormat == RenderDriver::Common::Format::D32_FLOAT ||
                desc.mFormat == RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT);

            RenderDriver::Common::CImage::create(desc, device);

            RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
            mpNativeDevice = reinterpret_cast<VkDevice*>(deviceVulkan.getNativeDevice());

            RenderDriver::Vulkan::CPhysicalDevice* pPhysicalDeviceVulkan = deviceVulkan.getPhysicalDevice();
            VkPhysicalDevice* pNativePhysicalDevice = reinterpret_cast<VkPhysicalDevice*>(pPhysicalDeviceVulkan->getNativeDevice());

            VkImageCreateInfo imageInfo = {};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = desc.miWidth;
            imageInfo.extent.height = desc.miHeight;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = desc.miNumImages;
            imageInfo.format = SerializeUtils::Vulkan::convert(desc.mFormat);
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            imageInfo.usage = /*imageInfo.usage |
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;*/
                (bIsDepthStencil) ? 
                imageInfo.usage | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 
                imageInfo.usage | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
            imageInfo.usage = (desc.mResourceFlags == RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess) ? imageInfo.usage | VK_IMAGE_USAGE_STORAGE_BIT : imageInfo.usage;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.flags = 0; //SerializeUtils::Vulkan::convert(desc.mResourceFlags);
            imageInfo.queueFamilyIndexCount = 0;
            imageInfo.pQueueFamilyIndices = nullptr;
            
            VkResult ret = vkCreateImage(*mpNativeDevice, &imageInfo, nullptr, &mNativeImage);
            WTFASSERT(ret == VK_SUCCESS, "Error creating image: %d", ret);

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(*mpNativeDevice, mNativeImage, &memRequirements);
            
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;

            uint32_t iMemoryTypeIndex = RenderDriver::Vulkan::Utils::findMemoryType(
                memRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                *pNativePhysicalDevice);
            WTFASSERT(iMemoryTypeIndex != UINT32_MAX, "can\'t find memory type for image");

            allocInfo.memoryTypeIndex = iMemoryTypeIndex;

            ret = vkAllocateMemory(*mpNativeDevice, &allocInfo, nullptr, &mNativeMemory);
            WTFASSERT(ret == VK_SUCCESS, "Error allocating memory for image: %d", ret);

            vkBindImageMemory(*mpNativeDevice, mNativeImage, mNativeMemory, 0);

            maCurrImageLayout[0] = RenderDriver::Common::ImageLayout::UNDEFINED;
            maCurrImageLayout[1] = RenderDriver::Common::ImageLayout::UNDEFINED;
            maCurrImageLayout[2] = RenderDriver::Common::ImageLayout::UNDEFINED;

            return mHandle;
        }

        /*
        **
        */
        void CImage::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeImage);
            objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE;
            objectNameInfo.pObjectName = id.c_str();

            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);

            std::string memoryName = id + " Device Memory";
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeMemory);
            objectNameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
            objectNameInfo.pObjectName = memoryName.c_str();
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);
        }

        /*
        **
        */
        void CImage::setImageLayout(
            RenderDriver::Common::ImageLayout const& layout,
            uint32_t iQueueType)
        {
            maCurrImageLayout[0] = layout;
            maCurrImageLayout[1] = layout;
            maCurrImageLayout[2] = layout;
        }

        /*
        **
        */
        void CImage::setInitialImageLayout(
            RenderDriver::Common::ImageLayout const& layout,
            uint32_t iQueueType)
        {
            maInitialLayouts[0] = layout;
            maInitialLayouts[1] = layout;
            maInitialLayouts[2] = layout;
        }

        /*
        **
        */
        void* CImage::getNativeImage()
        {
            return &mNativeImage;
        }

    }   // Vulkan

}   // RenderDriver