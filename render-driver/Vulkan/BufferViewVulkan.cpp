#include <render-driver/Vulkan/BufferViewVulkan.h>
#include <render-driver/Vulkan/BufferVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>
#include <LogPrint.h>
#include <wtfassert.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CBufferView::create(
            RenderDriver::Common::BufferViewDescriptor& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CBufferView::create(desc, device);

            RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
            mpNativeDevice = static_cast<VkDevice*>(deviceVulkan.getNativeDevice());

            RenderDriver::Vulkan::CBuffer* pVulkanBuffer = static_cast<RenderDriver::Vulkan::CBuffer*>(desc.mpBuffer);
            VkBufferViewCreateInfo bufferViewCreateInfo = {};
            bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            bufferViewCreateInfo.flags = 0;
            bufferViewCreateInfo.buffer = *(static_cast<VkBuffer*>(pVulkanBuffer->getNativeBuffer()));
            bufferViewCreateInfo.format = SerializeUtils::Vulkan::convert(desc.mFormat);                            
            bufferViewCreateInfo.offset = 0;                                
            bufferViewCreateInfo.range = VK_WHOLE_SIZE;

            VkResult ret = vkCreateBufferView(
                *mpNativeDevice, 
                &bufferViewCreateInfo, 
                nullptr, 
                &mNativeBufferView);
            WTFASSERT(ret == VK_SUCCESS, "Error creating buffer view: %s", Utils::getErrorCode(ret));

            return mHandle;
        }

        /*
        **
        */
        void CBufferView::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeBufferView);
            objectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER_VIEW;
            objectNameInfo.pObjectName = id.c_str();

            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);
        }

    }   // Vulkan

}   // RenderDriver