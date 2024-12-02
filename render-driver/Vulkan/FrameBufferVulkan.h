#pragma once

#include <render-driver/Device.h>
#include <render-driver/FrameBuffer.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        class CFrameBuffer : public RenderDriver::Common::CFrameBuffer
        {
        public:
            CFrameBuffer() = default;
            virtual ~CFrameBuffer() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::FrameBufferDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            PLATFORM_OBJECT_HANDLE create(RenderDriver::Common::CDevice& device);

            virtual void create2(
                RenderDriver::Common::FrameBufferDescriptor2 const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void* getNativeFrameBuffer()
            {
                return &mNativeFrameBuffer;
            }

            void setNativeFrameBuffer(VkFramebuffer& frameBuffer)
            {
                mNativeFrameBuffer = frameBuffer;
            }

            void setHandle(PLATFORM_OBJECT_HANDLE handle)
            {
                mHandle = handle;
            }

            virtual void setID(std::string const& id);

        protected:
            VkFramebuffer               mNativeFrameBuffer;
            VkDevice*                   mpNativeDevice;
        };
    
    }   // Vulkan

}   // RenderDriver
