#include <render-driver/Vulkan/ImageViewVulkan.h>
#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/BufferVulkan.h>
#include <render-driver/Vulkan/ImageVulkan.h>

#include <LogPrint.h>
#include <wtfassert.h>

namespace RenderDriver
{
	namespace Vulkan
	{
		PLATFORM_OBJECT_HANDLE CImageView::create(
			RenderDriver::Common::ImageViewDescriptor const& desc,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Common::CImageView::create(desc, device);

			mpNativeDevice = static_cast<VkDevice*>(device.getNativeDevice());

			RenderDriver::Vulkan::CImage* pVulkanImage = nullptr;
			if(desc.mpImage)
			{
				pVulkanImage = static_cast<RenderDriver::Vulkan::CImage*>(desc.mpImage);
				WTFASSERT(pVulkanImage, "no image given");
			}

			RenderDriver::Vulkan::CBuffer* pVulkanBuffer = nullptr;
			if(desc.mpBuffer)
			{
				pVulkanBuffer = static_cast<RenderDriver::Vulkan::CBuffer*>(desc.mpBuffer);
				WTFASSERT(pVulkanBuffer, "no buffer given");
			}

			VkDevice* pNativeDevice = reinterpret_cast<VkDevice*>(static_cast<RenderDriver::Vulkan::CDevice&>(device).getNativeDevice());

			bool bIsDepthStencil =
				(desc.mFormat == RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT ||
					desc.mFormat == RenderDriver::Common::Format::D24_UNORM_S8_UINT ||
					desc.mFormat == RenderDriver::Common::Format::D32_FLOAT ||
					desc.mFormat == RenderDriver::Common::Format::D16_UNORM) ?
				true : false;

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = *(reinterpret_cast<VkImage*>(pVulkanImage->getNativeImage()));
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = SerializeUtils::Vulkan::convert(desc.mFormat);
			viewInfo.subresourceRange.aspectMask = (bIsDepthStencil == true) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			VkResult ret = vkCreateImageView(*pNativeDevice, &viewInfo, nullptr, &mNativeImageView);
			WTFASSERT(ret == VK_SUCCESS, "Error creating image view: %d", ret);

			mImageHandle = desc.mpImage->getHandle();

			return mHandle;
		}

		/*
		**
		*/
		void CImageView::setID(std::string const& id)
		{
			RenderDriver::Common::CObject::setID(id);

			VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
			objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeImageView);
			objectNameInfo.objectType = VK_OBJECT_TYPE_IMAGE_VIEW;
			objectNameInfo.pObjectName = id.c_str();

			PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
			setDebugUtilsObjectNameEXT(
				*mpNativeDevice,
				&objectNameInfo);
		}

	}   // Vulkan

}   // RenderDriver