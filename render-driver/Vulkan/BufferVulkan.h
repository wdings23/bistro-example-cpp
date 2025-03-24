#pragma once

#include <render-driver/Buffer.h>
#include <render-driver/Vulkan/DeviceVulkan.h>

namespace RenderDriver
{
    namespace Vulkan
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
            virtual void* getNativeDeviceMemory();

            virtual void setID(std::string const& name);

            virtual void copy(
                RenderDriver::Common::CBuffer& buffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iDestOffset,
                uint32_t iSrcOffset,
                uint32_t iDataSize);

            virtual void copyToImage(
                RenderDriver::Common::CBuffer& buffer,
                RenderDriver::Common::CImage& image,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iImageWidth,
                uint32_t iImageHeight);

            virtual void setData(
                void* pSrcData,
                uint32_t iDataSize);

            virtual void setDataOpen(
                void* pSrcData,
                uint32_t iDataSize);

            virtual uint64_t getGPUVirtualAddress();

            void* getMemoryOpen(uint32_t iDataSize);
            virtual void closeMemory();

            virtual void releaseNativeBuffer();

        protected:
            VkBuffer                mNativeBuffer;
            VkDeviceMemory          mNativeDeviceMemory;
            VkDevice*               mpNativeDevice;
        };

    }   // Vulkan

}   // RenderDriver