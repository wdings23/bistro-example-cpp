#pragma once

#include <render-driver/CommandBuffer.h>
#include <render-driver/Device.h>
#include <utils/serialize_utils.h>

#import <Metal/Metal.h>

namespace RenderDriver
{
    namespace Metal
    {
        struct CommandBufferDescriptor : public RenderDriver::Common::CommandBufferDescriptor
        {
            id<MTLCommandQueue>             mpCommandQueue = nil;
            MTLRenderPassDescriptor*        mpRenderPassDescriptor = nil;
        };
    
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
                uint32_t iGroupX,
                uint32_t iGroupY,
                uint32_t iGroupZ,
                uint32_t iLocalX = 256,
                uint32_t iLocalY = 1,
                uint32_t iLocalZ = 1);

            virtual void drawIndexInstanced(
                uint32_t iIndexCountPerInstance,
                uint32_t iNumInstances,
                uint32_t iStartIndexLocation,
                uint32_t iBaseVertexLocation,
                uint32_t iStartInstanceLocation,
                RenderDriver::Common::CBuffer* pIndexBuffer);

            virtual void setGraphicsConstantBufferView(
                uint64_t iConstantBufferAddress,
                uint32_t iDataSize);

            virtual void setGraphicsConstantBufferView2(
                RenderDriver::Common::CBuffer& buffer,
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

            virtual void reset();
            virtual void close();

            inline id<MTLRenderCommandEncoder> getNativeRenderCommandEncoder()
            {
                return mNativeRenderCommandEncoder;
            }
            
            inline id<MTLComputeCommandEncoder> getNativeComputeCommandEncoder()
            {
                return mNativeComputeCommandEncoder;
            }
            
            inline id<MTLBlitCommandEncoder> getNativeBlitCommandEncoder()
            {
                return mNativeBlitCommandEncoder;
            }
            
            void createBlitComandEncoder();
            void createComputeCommandEncoder();
            void createRenderCommandEncoder();
            
            void beginRenderPass(MTLRenderPassDescriptor* pRenderPassDescriptor);
            void beginComputePass(MTLComputePassDescriptor* pComputePassDescriptor);
            void beginCopy();
            
            virtual void drawIndirectCount(
                RenderDriver::Common::CBuffer& drawIndexBuffer,
                RenderDriver::Common::CBuffer& drawCountBuffer,
                RenderDriver::Common::CBuffer& meshIndexBuffer,
                uint32_t iCountBufferOffset
            );
            
        protected:
            id<MTLCommandBuffer>                mNativeCommandBuffer = nil;
            id<MTLCommandQueue>                 mNativeQueue;
            id<MTLDevice>                       mNativeDevice;
        
            id<MTLBlitCommandEncoder>           mNativeBlitCommandEncoder;
            id<MTLComputeCommandEncoder>        mNativeComputeCommandEncoder;
            id<MTLRenderCommandEncoder>         mNativeRenderCommandEncoder;
            
            RenderDriver::Common::CommandBufferType                   mType;
        };

    }   // Metal

}   // RenderDriver
