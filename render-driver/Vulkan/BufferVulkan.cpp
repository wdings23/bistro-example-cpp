#include <render-driver/Vulkan/BufferVulkan.h>
#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>

#include <serialize_utils.h>

#include <LogPrint.h>

#include <wtfassert.h>


namespace RenderDriver
{
    namespace Vulkan
    {
        PLATFORM_OBJECT_HANDLE CBuffer::create(
            RenderDriver::Common::BufferDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CBuffer::create(desc, device);

            RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
            mpNativeDevice = static_cast<VkDevice*>(deviceVulkan.getNativeDevice());

            RenderDriver::Vulkan::CPhysicalDevice* physicalDeviceVulkan = deviceVulkan.getPhysicalDevice();
            VkPhysicalDevice* pNativePhysicalDevice = static_cast<VkPhysicalDevice*>(physicalDeviceVulkan->getNativePhysicalDevice());

            VkBufferUsageFlags usageFlags = 0;
            for(uint32_t i = 1; i < 32; i++)
            {
                RenderDriver::Common::BufferUsage usage = static_cast<RenderDriver::Common::BufferUsage>(static_cast<uint32_t>(desc.mBufferUsage) & (1 << i));
                usageFlags |= SerializeUtils::Vulkan::convert(usage);
            }

            VkBufferCreateInfo bufferInfo = {};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = desc.miSize;
            bufferInfo.usage = usageFlags;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult ret = vkCreateBuffer(*mpNativeDevice, &bufferInfo, nullptr, &mNativeBuffer);
            WTFASSERT(ret == VK_SUCCESS, "Error creating buffer: %s\n", Utils::getErrorCode(ret));

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(*mpNativeDevice, mNativeBuffer, &memRequirements);

            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(*pNativePhysicalDevice, &memProperties);

            bool bCPUVisible = (
                ((uint32_t(desc.mBufferUsage) & uint32_t(RenderDriver::Common::BufferUsage::TransferSrc)) > 0) ||
                desc.mHeapType == RenderDriver::Common::HeapType::Upload
            );

            // find correct memory index
            VkMemoryPropertyFlags  memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            if(bCPUVisible)
            {
                memoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            }
            else if((uint32_t(desc.mBufferUsage) & uint32_t(RenderDriver::Common::BufferUsage::ShaderDeviceAddress)) > 0)
            {
                memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            }

            uint32_t iMemTypeIndex = RenderDriver::Vulkan::Utils::findMemoryType(
                memRequirements.memoryTypeBits, 
                memoryPropertyFlags, 
                *pNativePhysicalDevice);
            WTFASSERT(iMemTypeIndex != UINT32_MAX, "didn\'t find memory type");

            VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
            memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
            memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

            // allocate memory
            VkMemoryAllocateInfo allocInfo = {};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = iMemTypeIndex;
            allocInfo.pNext = &memoryAllocateFlagsInfo;
            ret = vkAllocateMemory(*mpNativeDevice, &allocInfo, nullptr, &mNativeDeviceMemory);
            WTFASSERT(ret == VK_SUCCESS, "Error allocating memory for buffer: %s", Utils::getErrorCode(ret));
            ret = vkBindBufferMemory(*mpNativeDevice, mNativeBuffer, mNativeDeviceMemory, 0);
            WTFASSERT(ret == VK_SUCCESS, "Error binding memory to buffer: %s", Utils::getErrorCode(ret));

            return mHandle;
        }

        /*
        **
        */
        void CBuffer::copy(
            RenderDriver::Common::CBuffer& buffer,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iDestOffset,
            uint32_t iSrcOffset,
            uint32_t iDataSize)
        {
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));
            VkBuffer& nativeSrcBuffer = *(static_cast<VkBuffer*>(buffer.getNativeBuffer()));

            VkBufferCopy region;
            region.dstOffset = iDestOffset;
            region.size = iDataSize;
            region.srcOffset = iSrcOffset;

            vkCmdCopyBuffer(
                nativeCommandBuffer,
                nativeSrcBuffer,
                mNativeBuffer,
                1,
                &region);
        }

        /*
        **
        */
        void CBuffer::copyToImage(
            RenderDriver::Common::CBuffer& buffer,
            RenderDriver::Common::CImage& image,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iImageWidth,
            uint32_t iImageHeight
        )
        {
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = 0; //  iTextureArrayIndex;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = iImageWidth;
            bufferCopyRegion.imageExtent.height = iImageHeight;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = 0;
            VkCommandBuffer& nativeUploadCommandBufferVulkan = *(static_cast<VkCommandBuffer*>(commandBuffer.getNativeCommandList()));
            VkBuffer& nativeUploadBufferVulkan = *(static_cast<VkBuffer*>(buffer.getNativeBuffer()));
            VkImage& nativeImage = *(static_cast<VkImage*>(image.getNativeImage()));
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            RenderDriver::Common::CommandBufferState const& commandBufferState = commandBuffer.getState();
            if(commandBufferState == RenderDriver::Common::CommandBufferState::Closed)
            {
                commandBuffer.reset();
            }
            VkImageMemoryBarrier barrier = {};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = nativeImage;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            vkCmdPipelineBarrier(
                nativeUploadCommandBufferVulkan,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

            vkCmdCopyBufferToImage(
                nativeUploadCommandBufferVulkan,
                nativeUploadBufferVulkan,
                nativeImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &bufferCopyRegion);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

            vkCmdPipelineBarrier(
                nativeUploadCommandBufferVulkan,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

            commandBuffer.close();
        }

        /*
        **
        */
        void CBuffer::setDataOpen(
            void* pSrcData,
            uint32_t iDataSize)
        {
            void* pDestData = nullptr;
            vkMapMemory(
                *mpNativeDevice,
                mNativeDeviceMemory,
                0,
                iDataSize,
                0,
                &pDestData);
            memcpy(pDestData, pSrcData, iDataSize);
        }

        /*
        **
        */
        void CBuffer::setData(
            void* pSrcData,
            uint32_t iDataSize)
        {
            void* pDestData = nullptr;
            vkMapMemory(
                *mpNativeDevice,
                mNativeDeviceMemory,
                0,
                iDataSize,
                0,
                &pDestData);
            memcpy(pDestData, pSrcData, iDataSize);
            vkUnmapMemory(*mpNativeDevice, mNativeDeviceMemory);
        }

        /*
        **
        */
        void* CBuffer::getMemoryOpen(uint32_t iDataSize)
        {
            void* pDestData = nullptr;
            vkMapMemory(
                *mpNativeDevice,
                mNativeDeviceMemory,
                0,
                iDataSize,
                0,
                &pDestData);

            return pDestData;
        }

        /*
        **
        */
        void CBuffer::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeBuffer);
            objectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
            objectNameInfo.pObjectName = id.c_str();
            
            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);

            std::string memoryName = id + " Device Memory";
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeDeviceMemory);
            objectNameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
            objectNameInfo.pObjectName = memoryName.c_str();
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);
        }

        /*
        **
        */
        void* CBuffer::getNativeBuffer()
        {
            return &mNativeBuffer;
        }

        /*
        **
        */
        void* CBuffer::getNativeDeviceMemory()
        {
            return &mNativeDeviceMemory;
        }

        /*
        **
        */
        uint64_t CBuffer::getGPUVirtualAddress()
        {
            return 0;
        }

        /*
        **
        */
        void CBuffer::releaseNativeBuffer()
        {
            vkDestroyBuffer(*mpNativeDevice, mNativeBuffer, nullptr);
            vkFreeMemory(*mpNativeDevice, mNativeDeviceMemory, nullptr);
            mNativeBuffer = nullptr;
            mNativeDeviceMemory = nullptr;
        }

    }   // DX12

}   // RenderDriver