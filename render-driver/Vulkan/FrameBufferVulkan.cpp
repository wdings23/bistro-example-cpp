#include <render-driver/Vulkan/FrameBufferVulkan.h>
#include <render-driver/Vulkan/PipelineStateVulkan.h>
#include <render-driver/Vulkan/RenderTargetVulkan.h>

#include <wtfassert.h>
#include <LogPrint.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CFrameBuffer::create(RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::FrameBufferDescriptor desc = {};
            RenderDriver::Common::CFrameBuffer::create(desc, device);
            return mHandle;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CFrameBuffer::create(
            RenderDriver::Common::FrameBufferDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CFrameBuffer::create(desc, device);

            std::vector<RenderDriver::Common::CRenderTarget*>& apRenderTargets = *desc.mpaRenderTargets;

            uint32_t iNumValidRenderTargets = 0;
            RenderDriver::Vulkan::CImageView* pSelectedDepthImageView = nullptr;
            uint32_t iFrameBufferWidth = 0, iFrameBufferHeight = 0;
            std::vector<VkImageView> aNativeImageViews;
            for(uint32_t i = 0; i < static_cast<uint32_t>(apRenderTargets.size()); i++)
            {
                if(apRenderTargets[i])
                {
                    RenderDriver::Vulkan::CRenderTarget* pRenderTargetVulkan = static_cast<RenderDriver::Vulkan::CRenderTarget*>(apRenderTargets[i]);
                    RenderDriver::Vulkan::CImageView* pColorImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pRenderTargetVulkan->getColorImageView());
                    aNativeImageViews.push_back(pColorImageView->getNativeImageView());

                    RenderDriver::Vulkan::CImageView* pDepthImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pRenderTargetVulkan->getDepthStencilImageView());
                    if(pDepthImageView && pSelectedDepthImageView == nullptr)
                    {
                        pSelectedDepthImageView = pDepthImageView;
                    }

                    iFrameBufferWidth = pRenderTargetVulkan->getDescriptor().miWidth;
                    iFrameBufferHeight = pRenderTargetVulkan->getDescriptor().miHeight;

                    ++iNumValidRenderTargets;
                }
                else
                {
                    DEBUG_PRINTF("no render target %d for \"%s\"", 
                        i,
                        desc.mpPipelineState->getID().c_str());
                }
            }

            // add depth 
            if(pSelectedDepthImageView)
            {
                aNativeImageViews.push_back(pSelectedDepthImageView->getNativeImageView());
            }

            RenderDriver::Vulkan::CPipelineState* pPipelineStateVulkan = static_cast<RenderDriver::Vulkan::CPipelineState*>(desc.mpPipelineState);

            VkFramebufferCreateInfo frameBufferCreateInfo = {};
            frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferCreateInfo.pNext = nullptr;
            frameBufferCreateInfo.renderPass = *(static_cast<VkRenderPass*>(pPipelineStateVulkan->getNativeRenderPass()));
            frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(aNativeImageViews.size());
            frameBufferCreateInfo.pAttachments = aNativeImageViews.data();
            frameBufferCreateInfo.width = iFrameBufferWidth;
            frameBufferCreateInfo.height = iFrameBufferHeight;
            frameBufferCreateInfo.layers = 1;

            mpNativeDevice = static_cast<VkDevice*>(device.getNativeDevice());

            VkResult ret = vkCreateFramebuffer(
                *mpNativeDevice,
                &frameBufferCreateInfo,
                nullptr,
                &mNativeFrameBuffer);
            WTFASSERT(ret == VK_SUCCESS, "Error creating frame buffer: %d", ret);

            return mHandle;
        }
        
        /*
        **
        */
        void CFrameBuffer::create2(
            RenderDriver::Common::FrameBufferDescriptor2 const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::FrameBufferDescriptor desc0;
            RenderDriver::Common::CFrameBuffer::create(desc0, device);

            uint32_t iNumImages = (uint32_t)desc.mpaImageView->size();

            uint32_t iNumValidRenderTargets = 0;
            RenderDriver::Vulkan::CImageView* pSelectedDepthImageView = nullptr;
            uint32_t iFrameBufferWidth = 0, iFrameBufferHeight = 0;
            std::vector<VkImageView> aNativeImageViews;
            for(uint32_t i = 0; i < iNumImages; i++)
            {
                RenderDriver::Vulkan::CImageView* pColorImageView = static_cast<RenderDriver::Vulkan::CImageView*>((*desc.mpaImageView)[i]);
                aNativeImageViews.push_back(pColorImageView->getNativeImageView());

                if(i == desc.mDepthView.second)
                {
                    pSelectedDepthImageView = (RenderDriver::Vulkan::CImageView*)desc.mDepthView.first;
                }
                
                iFrameBufferWidth = desc.miWidth;
                iFrameBufferHeight = desc.miHeight;

                ++iNumValidRenderTargets;
                
            }

            // add depth 
            if(pSelectedDepthImageView)
            {
                aNativeImageViews.push_back(pSelectedDepthImageView->getNativeImageView());
            }

            RenderDriver::Vulkan::CPipelineState* pPipelineStateVulkan = static_cast<RenderDriver::Vulkan::CPipelineState*>(desc.mpPipelineState);

            VkFramebufferCreateInfo frameBufferCreateInfo = {};
            frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferCreateInfo.pNext = nullptr;
            frameBufferCreateInfo.renderPass = *(static_cast<VkRenderPass*>(pPipelineStateVulkan->getNativeRenderPass()));
            frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(aNativeImageViews.size());
            frameBufferCreateInfo.pAttachments = aNativeImageViews.data();
            frameBufferCreateInfo.width = iFrameBufferWidth;
            frameBufferCreateInfo.height = iFrameBufferHeight;
            frameBufferCreateInfo.layers = 1;

            mpNativeDevice = static_cast<VkDevice*>(device.getNativeDevice());

            VkResult ret = vkCreateFramebuffer(
                *mpNativeDevice,
                &frameBufferCreateInfo,
                nullptr,
                &mNativeFrameBuffer);
            WTFASSERT(ret == VK_SUCCESS, "Error creating frame buffer: %d", ret);
        }

        /*
        **
        */
        void CFrameBuffer::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeFrameBuffer);
            objectNameInfo.objectType = VK_OBJECT_TYPE_FRAMEBUFFER;
            objectNameInfo.pObjectName = id.c_str();

            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);
        }

    }   // Vulkan

}   // RenderDriver