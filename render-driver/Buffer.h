#pragma once

#include <render-driver/Device.h>
#include <render-driver/Format.h>
#include <render-driver/CommandBuffer.h>
#include <utils/serialize_utils.h>

namespace RenderDriver
{
    namespace Common
    {
        class CBuffer;
        struct BufferDescriptor
        {
            uint64_t                    miSize;
            Format                      mFormat;
            ResourceFlagBits            mFlags;
            HeapType                    mHeapType = HeapType::Default;
            ResourceStateFlagBits       mOverrideState = ResourceStateFlagBits::None;
            CBuffer*                    mpCounterBuffer = nullptr;

            uint32_t                    miImageViewWidth;
            uint32_t                    miImageViewHeight;
            Format                      mImageViewFormat;

            BufferUsage                 mBufferUsage;
        };

        class CBuffer : public CObject
        {
        public:
            CBuffer() = default;
            virtual ~CBuffer() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                BufferDescriptor const& desc,
                CDevice& device);
        
            inline BufferDescriptor& getDescriptor()
            {
                return mDesc;
            }

            RenderDriver::Common::ResourceStateFlagBits         getState() { return mResourceState; }
            RenderDriver::Common::ResourceStateFlagBits         getState(PLATFORM_OBJECT_HANDLE queueHandle);
            void                                                setState(RenderDriver::Common::ResourceStateFlagBits const& state) { mResourceState = state; }
            void                                                setState(PLATFORM_OBJECT_HANDLE queueHandle, RenderDriver::Common::ResourceStateFlagBits const& state);
            
            virtual void                                                copy(
                RenderDriver::Common::CBuffer& buffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iDestOffset,
                uint32_t iSrcOffset,
                uint32_t iDataSize) = 0;

            virtual void setData(
                void* pSrcData,
                uint32_t iDataSize);

            virtual void setDataOpen(
                void* pSrcData,
                uint32_t iDataSize);

            virtual void* getNativeBuffer();

            virtual uint64_t getGPUVirtualAddress();

            virtual void releaseNativeBuffer();

            virtual void* getMemoryOpen(uint32_t iDataSize);

            virtual void closeMemory() {};

        protected:
            BufferDescriptor                                    mDesc;

            RenderDriver::Common::ResourceStateFlagBits         mResourceState;
            std::map<PLATFORM_OBJECT_HANDLE, RenderDriver::Common::ResourceStateFlagBits>       mQueueResourceState;

        };

    }   // Common

}   // RenderDriver
