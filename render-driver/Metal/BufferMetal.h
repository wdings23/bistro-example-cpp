#pragma once

#include <RenderDriver/Buffer.h>
#include <RenderDriver/Metal/DeviceMetal.h>

namespace RenderDriver
{
    namespace Metal
    {
        class CBuffer : public RenderDriver::Common::CBuffer
        {
        public:
            CBuffer() = default;
            virtual ~CBuffer() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::BufferDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void* getNativeBuffer();

            virtual void setID(std::string const& name);

            virtual void copy(
                RenderDriver::Common::CBuffer& buffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iDestOffset,
                uint32_t iSrcOffset,
                uint32_t iDataSize);

            virtual void setData(
                void* pSrcData,
                uint32_t iDataSize);

            virtual void setDataOpen(
                void* pSrcData,
                uint32_t iDataSize);

            virtual uint64_t getGPUVirtualAddress();
            
        protected:
            id<MTLBuffer>       mNativeBuffer;
            id<MTLDevice>       mNativeDevice;
        };

    }   // Metal

}   // RenderDriver
