#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/FenceVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>
#include <render-driver/Vulkan/CommandQueueVulkan.h>

#include <LogPrint.h>
#include <wtfassert.h>

#include <vulkan/vulkan.h>
#include <chrono>

namespace RenderDriver
{
    namespace Vulkan
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CFence::create(
            RenderDriver::Common::FenceDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CFence::create(desc, device);

            RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
            mpNativeDevice = static_cast<VkDevice*>(deviceVulkan.getNativeDevice());

            VkFenceCreateInfo fenceInfo = {};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

            VkResult ret = vkCreateFence(*mpNativeDevice, &fenceInfo, nullptr, &mNativeFence);
            WTFASSERT(ret == VK_SUCCESS, "Error creating fence: %d", ret);

            VkSemaphoreTypeCreateInfo timelineSemaphoreCreateInfo = {};
            timelineSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineSemaphoreCreateInfo.pNext = nullptr;
            timelineSemaphoreCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineSemaphoreCreateInfo.initialValue = 0;

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreInfo.pNext = &timelineSemaphoreCreateInfo;
            semaphoreInfo.flags = 0;
            ret = vkCreateSemaphore(*mpNativeDevice, &semaphoreInfo, nullptr, &mNativeSemaphore);
            WTFASSERT(ret == VK_SUCCESS, "Error creating semaphore: %d", ret);

            return mHandle;
        }

        /*
        **
        */
        void CFence::place(RenderDriver::Common::PlaceFenceDescriptor const& desc)
        {
            mPlaceFenceDesc = desc;
        }

        /*
        **
        */
        bool CFence::isCompleted()
        {
            return true;
        }

        /*
        **
        */
        void CFence::waitCPU(
            uint64_t iMilliseconds,
            uint64_t iWaitValue)
        {
            RenderDriver::Vulkan::CCommandQueue* pCommandQueueVulkan = static_cast<RenderDriver::Vulkan::CCommandQueue*>(mPlaceFenceDesc.mpCommandQueue);
            VkQueue* pNativeCommandQueue = static_cast<VkQueue*>(pCommandQueueVulkan->getNativeCommandQueue());

            VkResult ret = vkQueueWaitIdle(*pNativeCommandQueue);
            WTFASSERT(ret == VK_SUCCESS, "Error waiting for fence: %d", ret);

            //vkWaitForFences(*mpNativeDevice, 1, &mNativeFence, VK_TRUE, UINT64_MAX);
            //vkResetFences(*mpNativeDevice, 1, &mNativeFence);
        }

        /*
        **
        */
        void CFence::waitCPU2(
            uint64_t iMilliseconds,
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t iWaitValue)
        {
            vkQueueWaitIdle(*(static_cast<VkQueue*>(pCommandQueue->getNativeCommandQueue())));

#if 0
            RenderDriver::Vulkan::CCommandQueue* pCommandQueueVulkan = static_cast<RenderDriver::Vulkan::CCommandQueue*>(pCommandQueue);
            VkSemaphore* pSignalSemaphore = pCommandQueueVulkan->getNativeSignalSemaphore();
            
            uint64_t iSignalValue = UINT64_MAX;
            VkResult ret = vkGetSemaphoreCounterValue(
                *mpNativeDevice,
                *pSignalSemaphore,
                &iSignalValue);
            WTFASSERT(ret == VK_SUCCESS, "Error getting semaphore counter value: %d", ret);
            
            if(iSignalValue < iWaitValue)
            {
                VkSemaphoreWaitInfo waitInfo;
                waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
                waitInfo.pNext = NULL;
                waitInfo.flags = 0;
                waitInfo.semaphoreCount = 1;
                waitInfo.pSemaphores = pSignalSemaphore;
                waitInfo.pValues = &iWaitValue;
            
                VkResult ret = vkWaitSemaphores(
                    *mpNativeDevice,
                    &waitInfo,
                    UINT64_MAX);
                WTFASSERT(ret == VK_SUCCESS, "Error waiting for queue to be idle: %d", ret);
            }
#endif // #if 0
        }

        /*
        **
        */
        void CFence::reset(RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
            vkResetFences(*mpNativeDevice, 1, &mNativeFence);
        }

        /*
        **
        */
        void CFence::reset2()
        {
            // destroy semaphore
            vkDestroySemaphore(
                *mpNativeDevice,
                mNativeSemaphore,
                nullptr
            );

            // re-create semaphore
            VkSemaphoreTypeCreateInfo timelineSemaphoreCreateInfo = {};
            timelineSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
            timelineSemaphoreCreateInfo.pNext = nullptr;
            timelineSemaphoreCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
            timelineSemaphoreCreateInfo.initialValue = 0;

            VkSemaphoreCreateInfo semaphoreInfo = {};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphoreInfo.pNext = &timelineSemaphoreCreateInfo;
            semaphoreInfo.flags = 0;
            VkResult ret = vkCreateSemaphore(
                *mpNativeDevice, 
                &semaphoreInfo, 
                nullptr, 
                &mNativeSemaphore
            );
            WTFASSERT(ret == VK_SUCCESS, "Error creating semaphore: %d", ret);
        }

        /*
        **
        */
        void CFence::waitGPU(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            RenderDriver::Common::CFence* pSignalFence,
            uint64_t iWaitValue)
        {
            static_cast<RenderDriver::Vulkan::CCommandQueue*>(pCommandQueue)->placeFence(
                this,
                iWaitValue);
        }

        /*
        **
        */
        void CFence::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);

            std::string semaphoreName = id + " Semaphore";

            VkDebugUtilsObjectNameInfoEXT objectNameInfo = {};
            objectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeSemaphore);
            objectNameInfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
            objectNameInfo.pObjectName = semaphoreName.c_str();

            PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(*mpNativeDevice, "vkSetDebugUtilsObjectNameEXT");
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);

            objectNameInfo.pObjectName = id.c_str();
            objectNameInfo.objectType = VK_OBJECT_TYPE_FENCE;
            objectNameInfo.objectHandle = reinterpret_cast<uint64_t>(mNativeFence);
            setDebugUtilsObjectNameEXT(
                *mpNativeDevice,
                &objectNameInfo);
        }

        /*
        **
        */
        uint64_t CFence::getFenceValue()
        {
            return UINT64_MAX;
        }

        /*
        **
        */
        void* CFence::getNativeFence()
        {
            return &mNativeFence;
        }

        /*
        **
        */
        void CFence::signal(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t const& iWaitFenceValue)
        {
            miFenceValue = iWaitFenceValue;
            mPlaceFenceDesc.mpCommandQueue = pCommandQueue;
            mPlaceFenceDesc.miFenceValue = iWaitFenceValue;

            static_cast<RenderDriver::Vulkan::CCommandQueue*>(pCommandQueue)->signal(
                this,
                iWaitFenceValue);
        }

        /*
        **
        */
        void CFence::waitCPU(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t iFenceValue)
        {
            vkWaitForFences(*mpNativeDevice, 1, &mNativeFence, VK_TRUE, UINT64_MAX);
            vkResetFences(*mpNativeDevice, 1, &mNativeFence);
        }

        /*
        **
        */
        void CFence::waitGPU()
        {
            
        }
    }
}