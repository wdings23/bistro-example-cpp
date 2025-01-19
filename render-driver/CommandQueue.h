#pragma once

#include <render-driver/Device.h>
#include <render-driver/CommandBuffer.h>
#include <render-driver/Fence.h>

namespace RenderDriver
{
    namespace Common
    {
        struct CCommandQueue : public CObject
        {
        public:
            enum Type
            {
                None = -1,
                Graphics = 0,
                Compute,
                Copy,
                CopyGPU,

                NumTypes,
            };

            struct CreateDesc
            {
                RenderDriver::Common::CDevice* mpDevice;
                Type mType;
            };

        public:
            CCommandQueue() = default;
            virtual ~CCommandQueue() = default;

            virtual PLATFORM_OBJECT_HANDLE create(CreateDesc& desc);
            virtual void execCommandBuffer(
                CCommandBuffer& commandBuffer,
                CDevice& device);

            virtual void execCommandBuffer2(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::CFence& fence,
                uint64_t iFenceValue,
                RenderDriver::Common::CDevice& device) = 0;

            virtual void execCommandBuffer3(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint64_t* piWaitValue,
                uint64_t* piSignalValue,
                RenderDriver::Common::CFence* pWaitFence,
                RenderDriver::Common::CFence* pSignalFence
            );

            virtual void execCommandBufferSynchronized(
                CCommandBuffer& commandBuffer,
                CDevice& device,
                bool bWait = true);

            virtual void* getNativeCommandQueue();

            inline Type getType()
            {
                return mType;
            }

        protected:
            Type                        mType;

        };

    }   // Common

}   // RenderDriver