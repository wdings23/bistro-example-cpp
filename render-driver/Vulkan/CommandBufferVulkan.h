#pragma once

#include <render-driver/CommandBuffer.h>
#include <render-driver/Device.h>
#include <serialize_utils.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        struct CCommandBuffer : public RenderDriver::Common::CCommandBuffer
        {
        public:
            CCommandBuffer() = default;
            virtual ~CCommandBuffer() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::CommandBufferDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            void* getNativeCommandList();

            virtual void setID(std::string const& name);

            virtual void setPipelineState(
                RenderDriver::Common::CPipelineState& pipelineState,
                RenderDriver::Common::CDevice& device);

            virtual void setDescriptor(
                PipelineInfo const& pipelineInfo,
                PLATFORM_OBJECT_HANDLE descriptorHeap,
                RenderDriver::Common::CDevice& device);

            virtual void dispatch(
                uint32_t iX,
                uint32_t iY,
                uint32_t iZ);

            virtual void drawIndexInstanced(
                uint32_t iIndexCountPerInstance,
                uint32_t iNumInstances,
                uint32_t iStartIndexLocation,
                uint32_t iBaseVertexLocation,
                uint32_t iStartInstanceLocation);

            virtual void setGraphicsConstantBufferView(
                uint64_t iConstantBufferAddress,
                uint32_t iDataSize);

            virtual void setGraphicsConstantBufferView2(
                RenderDriver::Common::CBuffer& buffer,
                uint32_t iDataSize);

            void setGraphicsConstantBufferView3(
                RenderDriver::Common::CBuffer& buffer,
                RenderDriver::Common::CDescriptorSet& descriptorSet,
                uint32_t iSetIndex,
                uint32_t iBindingIndex,
                uint32_t iDataSize);

            virtual void setIndexAndVertexBufferView(
                uint64_t iIndexBufferAddress,
                uint32_t iIndexBufferSize,
                uint32_t iFormat,
                uint64_t iVertexBufferAddress,
                uint32_t iVertexBufferSize,
                uint32_t iVertexStrideSizeInByte,
                RenderDriver::Common::CBuffer& indexBuffer,
                RenderDriver::Common::CBuffer& vertexBuffer);

            virtual void setIndexAndMultipleVertexBufferViews(
                uint64_t iIndexBufferAddress,
                uint32_t iIndexBufferSize,
                uint32_t iFormat,
                uint64_t iVertexPositionBufferAddress,
                uint32_t iVertexPositionBufferSize,
                uint32_t iVertexPositionStrideSizeInBytes,
                uint64_t iVertexNormalBufferAddress,
                uint32_t iVertexNormalBufferSize,
                uint32_t iVertexNormalStrideSizeInBytes,
                uint64_t iVertexUVBufferAddress,
                uint32_t iVertexUVBufferSize,
                uint32_t iVertexUVStrideSizeInBytes);

            virtual void clearRenderTarget(PLATFORM_OBJECT_HANDLE renderTarget);

            virtual void drawIndirectCount(
                RenderDriver::Common::CBuffer& drawIndexBuffer,
                RenderDriver::Common::CBuffer& drawCountBuffer,
                uint32_t iCountBufferOffset
            );

            virtual void reset();
            virtual void close();

        protected:
            VkCommandBuffer                         mNativeCommandBuffer;
            VkDevice*                               mpNativeDevice;
        };

    }   // Common

}   // RenderDriver