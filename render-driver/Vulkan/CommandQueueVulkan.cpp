#include <render-driver/Vulkan/CommandQueueVulkan.h>
#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/FenceVulkan.h>
#include <render-driver/Vulkan/CommandBufferVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>

#include <wtfassert.h>

#include <mutex>
std::mutex gSubmitMutex;

namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandQueue::create(RenderDriver::Common::CCommandQueue::CreateDesc& desc)
        {
            RenderDriver::Vulkan::CCommandQueue::CreateDesc& createDescVulkan = static_cast<RenderDriver::Vulkan::CCommandQueue::CreateDesc&>(desc);

            RenderDriver::Vulkan::CDevice* pDeviceVulkan = static_cast<RenderDriver::Vulkan::CDevice*>(desc.mpDevice);
            mpNativeDevice = static_cast<VkDevice*>(pDeviceVulkan->getNativeDevice());
            VkDevice& nativeDeviceVulkan = *mpNativeDevice;

            RenderDriver::Vulkan::CPhysicalDevice* pPhysicalDevice = pDeviceVulkan->getPhysicalDevice();

            uint32_t iGraphicsQueueFamilyIndex = pPhysicalDevice->getGraphicsQueueFamilyIndex();
            uint32_t iComputeQueueFamilyIndex = pPhysicalDevice->getComputeQueueFamilyIndex();
            uint32_t iCopyQueueFamilyIndex = pPhysicalDevice->getTransferFamilyIndex();
            
            if(desc.mType == Type::Graphics)
            {
                vkGetDeviceQueue(
                    nativeDeviceVulkan,
                    iGraphicsQueueFamilyIndex,
                    0,
                    &mNativeQueue);
            }
            else if(desc.mType == Type::Compute)
            {
                vkGetDeviceQueue(
                    nativeDeviceVulkan,
                    iComputeQueueFamilyIndex,
                    (iGraphicsQueueFamilyIndex == iComputeQueueFamilyIndex) ? 1 : 0,
                    &mNativeQueue);
            }
            else if(desc.mType == Type::Copy)
            {
                vkGetDeviceQueue(
                    nativeDeviceVulkan,
                    iComputeQueueFamilyIndex,
                    (iGraphicsQueueFamilyIndex == iComputeQueueFamilyIndex && iComputeQueueFamilyIndex == iCopyQueueFamilyIndex) ? 2 : 0,
                    &mNativeQueue);
            }
            else if(desc.mType == Type::CopyGPU)
            {
                vkGetDeviceQueue(
                    nativeDeviceVulkan,
                    iComputeQueueFamilyIndex,
                    (iGraphicsQueueFamilyIndex == iComputeQueueFamilyIndex && iComputeQueueFamilyIndex == iCopyQueueFamilyIndex) ? 3 : 0,
                    &mNativeQueue);
            }

#if 0
            // create semaphores with initial value to 0
            VkSemaphoreTypeCreateInfo timelineSemaphoreInfo = {};
            timelineSemaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineSemaphoreInfo.pNext = nullptr;
            timelineSemaphoreInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineSemaphoreInfo.initialValue = 0;

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreInfo.pNext = &timelineSemaphoreInfo;
            semaphoreInfo.flags = 0;
            VkResult ret = vkCreateSemaphore(
                nativeDeviceVulkan,
                &semaphoreInfo,
                nullptr,
                &mNativeSignalSemaphores);
            WTFASSERT(ret == VK_SUCCESS, "Error creating signal semaphore: %s", Utils::getErrorCode(ret));
#endif // #if 0

            miSignalValue = UINT64_MAX;
            memset(maiWaitValues, 0, sizeof(maiWaitValues));
            memset(mapNativeWaitSemaphores, 0, sizeof(mapNativeWaitSemaphores));
            miSemaphorePlaceIndex = 0;
            miNumSignalSemaphores = 0;

            mType = desc.mType;
            mHandle = pDeviceVulkan->registerCommandQueue(this);

            return mHandle;
        }

        /*
        **
        */
        void CCommandQueue::execCommandBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CDevice& device)
        {
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(static_cast<RenderDriver::Vulkan::CCommandBuffer&>(commandBuffer).getNativeCommandList()));

            VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {};
            timelineSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
            timelineSemaphoreSubmitInfo.pNext = nullptr;
            timelineSemaphoreSubmitInfo.waitSemaphoreValueCount = miSemaphorePlaceIndex;
            timelineSemaphoreSubmitInfo.pWaitSemaphoreValues = maiWaitValues;
            timelineSemaphoreSubmitInfo.signalSemaphoreValueCount = miNumSignalSemaphores;
            timelineSemaphoreSubmitInfo.pSignalSemaphoreValues = maiSignalValues;

            VkSemaphore aWaitSemaphores[MAX_SEMAPHORE_PLACEMENTS] = { 0 };
            for(uint32_t i = 0; i < miSemaphorePlaceIndex; i++)
            {
                aWaitSemaphores[i] = *mapNativeWaitSemaphores[i];
            }

            VkPipelineStageFlags destStageMask = (mType == Type::Compute) ?
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT :
                (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = &timelineSemaphoreSubmitInfo;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &nativeCommandBuffer;
            submitInfo.waitSemaphoreCount = miSemaphorePlaceIndex;
            submitInfo.pWaitSemaphores = aWaitSemaphores;
            submitInfo.pWaitDstStageMask = (miSemaphorePlaceIndex > 0) ? &destStageMask : nullptr;
            submitInfo.signalSemaphoreCount = (miSignalValue != UINT64_MAX) ? miNumSignalSemaphores.load() : 0;
            submitInfo.pSignalSemaphores = (miSignalValue != UINT64_MAX) ? maNativeSignalSemaphores : nullptr;
            
            {
                //std::lock_guard<std::mutex> lock(gSubmitMutex);
                VkResult ret = vkQueueSubmit(
                    mNativeQueue,
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE);
                WTFASSERT(ret == VK_SUCCESS, "Error submitting command buffer: %s", Utils::getErrorCode(ret));
            }

            miSignalValue = UINT64_MAX;
            miSemaphorePlaceIndex = 0;
            miNumSignalSemaphores = 0;
        }

        /*
        **
        */
        void CCommandQueue::execCommandBuffer2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CFence& fence,
            uint64_t iFenceValue,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Vulkan::CFence& fenceVulkan = static_cast<RenderDriver::Vulkan::CFence&>(fence);
            VkFence& nativeFence = *(static_cast<VkFence*>(fenceVulkan.getNativeFence()));
            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(static_cast<RenderDriver::Vulkan::CCommandBuffer&>(commandBuffer).getNativeCommandList()));

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = nullptr;
            submitInfo.waitSemaphoreCount = 0;
            submitInfo.pWaitSemaphores = nullptr;
            submitInfo.pWaitDstStageMask = nullptr;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &nativeCommandBuffer;
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.pSignalSemaphores = nullptr;
            {
                std::lock_guard<std::mutex> lock(gSubmitMutex);
                VkResult ret = vkQueueSubmit(
                    mNativeQueue,
                    1,
                    &submitInfo,
                    nativeFence);
                WTFASSERT(ret == VK_SUCCESS, "Error submitting command buffer: %s", Utils::getErrorCode(ret));
            }

            miSignalValue = UINT64_MAX;
            miSemaphorePlaceIndex = 0;
            miNumSignalSemaphores = 0;
        }

        /*
        **
        */
        void CCommandQueue::execCommandBuffer3(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint64_t* piWaitValue,
            uint64_t* piSignalValue,
            RenderDriver::Common::CFence* pWaitFence,
            RenderDriver::Common::CFence* pSignalFence
        )
        {
            RenderDriver::Vulkan::CFence* pWaitFenceVulkan = (RenderDriver::Vulkan::CFence*)pWaitFence;
            RenderDriver::Vulkan::CFence* pSignalFenceVulkan = (RenderDriver::Vulkan::CFence*)pSignalFence;

            VkCommandBuffer& nativeCommandBuffer = *(static_cast<VkCommandBuffer*>(static_cast<RenderDriver::Vulkan::CCommandBuffer&>(commandBuffer).getNativeCommandList()));

            VkPipelineStageFlags destStageMask = (mType == Type::Compute) ?
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT :
                (VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT);

            VkTimelineSemaphoreSubmitInfo timelineSemaphoreSubmitInfo = {};
            timelineSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
            timelineSemaphoreSubmitInfo.signalSemaphoreValueCount = 1;
            timelineSemaphoreSubmitInfo.pSignalSemaphoreValues = piSignalValue;
            timelineSemaphoreSubmitInfo.waitSemaphoreValueCount = 1;
            timelineSemaphoreSubmitInfo.pWaitSemaphoreValues = piWaitValue;
            timelineSemaphoreSubmitInfo.pNext = nullptr;

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = &timelineSemaphoreSubmitInfo;
            submitInfo.waitSemaphoreCount = (pWaitFenceVulkan != nullptr && *piWaitValue > 0) ? 1 : 0;
            submitInfo.pWaitSemaphores = (pWaitFenceVulkan != nullptr && *piWaitValue > 0) ? pWaitFenceVulkan->getNativeSemaphore() : VK_NULL_HANDLE;
            submitInfo.pWaitDstStageMask = &destStageMask;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &nativeCommandBuffer;
            submitInfo.signalSemaphoreCount = (pSignalFenceVulkan != nullptr) ? 1 : 0;
            submitInfo.pSignalSemaphores = (pSignalFenceVulkan != nullptr) ? pSignalFenceVulkan->getNativeSemaphore() : VK_NULL_HANDLE;
            
            VkResult ret = vkQueueSubmit(
                mNativeQueue,
                1,
                &submitInfo,
                VK_NULL_HANDLE);
            WTFASSERT(ret == VK_SUCCESS, "Error submitting command buffer: %s", Utils::getErrorCode(ret));

                
        }

        /*
        **
        */
        void CCommandQueue::execCommandBufferSynchronized(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CDevice& device,
            bool bWait)
        {
            VkCommandBuffer* pNativeCommandBuffer = static_cast<VkCommandBuffer*>(static_cast<RenderDriver::Vulkan::CCommandBuffer&>(commandBuffer).getNativeCommandList());

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.pNext = nullptr;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = pNativeCommandBuffer;
            submitInfo.waitSemaphoreCount = 0;
            submitInfo.pWaitSemaphores = nullptr;
            submitInfo.pWaitDstStageMask = nullptr;
            submitInfo.signalSemaphoreCount = 0;
            submitInfo.pSignalSemaphores = nullptr;
            {
                std::lock_guard<std::mutex> lock(gSubmitMutex);
                VkResult ret = vkQueueSubmit(
                    mNativeQueue,
                    1,
                    &submitInfo,
                    VK_NULL_HANDLE);
                WTFASSERT(ret == VK_SUCCESS, "Error submitting command buffer: %s", Utils::getErrorCode(ret));

                if(bWait)
                {
                    vkQueueWaitIdle(mNativeQueue);
                }
            }
        }

        /*
        **
        */
        void CCommandQueue::placeFence(
            RenderDriver::Common::CFence* pWaitFence,
            uint64_t iWaitValue)
        {
            RenderDriver::Vulkan::CFence* pWaitFenceVulkan = static_cast<RenderDriver::Vulkan::CFence*>(pWaitFence);
            VkSemaphore* pNativeWaitSemaphore = pWaitFenceVulkan->getNativeSemaphore();

            // add wait semaphore and value to wait on
            mapNativeWaitSemaphores[miSemaphorePlaceIndex] = pNativeWaitSemaphore;
            maiWaitValues[miSemaphorePlaceIndex] = iWaitValue;

            ++miSemaphorePlaceIndex;
        }

        /*
        **
        */
        void CCommandQueue::signal(
            RenderDriver::Common::CFence* pSignalFence,
            uint64_t iSignalValue)
        {
            RenderDriver::Vulkan::CFence* pSignalFenceVulkan = static_cast<RenderDriver::Vulkan::CFence*>(pSignalFence);
            
            uint32_t iIndex = miNumSignalSemaphores.load(); 
            
            maNativeSignalSemaphores[iIndex] = *(pSignalFenceVulkan->getNativeSemaphore());
            maiSignalValues[iIndex] = iSignalValue;
            miSignalValue = (iSignalValue == 0) ? UINT64_MAX : iSignalValue;

            if(iSignalValue != UINT64_MAX)
            {
                miNumSignalSemaphores.fetch_add(1);
            }
        }

        /*
        **
        */
        void CCommandQueue::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeQueue);
            objectNameInfo.objectType = VK_OBJECT_TYPE_QUEUE;
            objectNameInfo.pObjectName = id.c_str();

            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);
        }

        /*
        **
        */
        VkSemaphore* CCommandQueue::getNativeSignalSemaphores(uint32_t& iNumSemaphores)
        {
            iNumSemaphores = static_cast<uint32_t>(miNumSignalSemaphores.load());
            return maNativeSignalSemaphores;
        }

        /*
        **
        */
        void CCommandQueue::resetSignals()
        {

        }

        /*
        **
        */
        void* CCommandQueue::getNativeCommandQueue()
        {
            return &mNativeQueue;
        }

    }   // Vulkan 

}   // RenderDriver