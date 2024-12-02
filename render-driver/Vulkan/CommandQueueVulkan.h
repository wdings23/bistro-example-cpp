#pragma once

#include <render-driver/CommandQueue.h>
#include <render-driver/CommandBuffer.h>
#include <render-driver/Device.h>
#include <render-driver/Vulkan/PhysicalDeviceVulkan.h>

#include <atomic>

#define MAX_SEMAPHORE_PLACEMENTS        128

namespace RenderDriver
{
    namespace Vulkan
    {
        class CCommandQueue : public RenderDriver::Common::CCommandQueue
        {
        public:
            CCommandQueue() = default;
            virtual ~CCommandQueue() = default;

            PLATFORM_OBJECT_HANDLE create(RenderDriver::Common::CCommandQueue::CreateDesc& desc);

            virtual void execCommandBuffer3(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint64_t* piWaitValue,
                uint64_t* piSignalValue,
                RenderDriver::Common::CFence* pWaitFence,
                RenderDriver::Common::CFence* pSignalFence
            );

            virtual void execCommandBuffer2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CFence& fence,
                uint64_t iFenceValue,
                RenderDriver::Common::CDevice& device);

            virtual void execCommandBuffer(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& id);

            virtual void* getNativeCommandQueue();

            void placeFence(
                RenderDriver::Common::CFence* pWaitFence,
                uint64_t iWaitValue);

            void signal(
                RenderDriver::Common::CFence* pSignalFence,
                uint64_t iSignalValue);

            VkSemaphore* getNativeSignalSemaphores(uint32_t& iRetSignalValue);

            inline uint32_t getNumSignalSemaphores()
            {
                return miNumSignalSemaphores;
            }

            inline uint64_t* getSignalValues()
            {
                return maiSignalValues;
            }

            void resetSignals();

        protected:

            VkQueue                             mNativeQueue;

            uint64_t                            miSignalValue;
            uint64_t                            maiWaitValues[MAX_SEMAPHORE_PLACEMENTS];
            uint64_t                            maiSignalValues[MAX_SEMAPHORE_PLACEMENTS];


            VkSemaphore*                        mapNativeWaitSemaphores[MAX_SEMAPHORE_PLACEMENTS];
            VkSemaphore                         maNativeWaitSemaphores[MAX_SEMAPHORE_PLACEMENTS];
            VkSemaphore                         maNativeSignalSemaphores[MAX_SEMAPHORE_PLACEMENTS];

            std::atomic<uint32_t>               miSemaphorePlaceIndex;
            VkDevice*                           mpNativeDevice;

            std::atomic<uint32_t>               miNumSignalSemaphores;
        };

    }   // Common

}   // RenderDriver