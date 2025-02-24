#include <render-driver/Metal/DeviceMetal.h>
#include <render-driver/Metal/CommandBufferMetal.h>
//#include <RenderDriver/Metal/CommandAllocatorMetal.h>
//#include <RenderDriver/Metal/DescriptorSetMetal.h>
#include <render-driver/Metal/PipelineStateMetal.h>

#include <utils/wtfassert.h>

namespace RenderDriver
{
    namespace Metal
    {
        PLATFORM_OBJECT_HANDLE CCommandBuffer::create(
            RenderDriver::Common::CommandBufferDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CCommandBuffer::create(desc, device);

            RenderDriver::Metal::CDevice& deviceMetal = static_cast<RenderDriver::Metal::CDevice&>(device);
            mNativeDevice = (__bridge id<MTLDevice>)deviceMetal.getNativeDevice();

            RenderDriver::Metal::CommandBufferDescriptor const& descMetal = static_cast<RenderDriver::Metal::CommandBufferDescriptor const&>(desc);
            WTFASSERT(descMetal.mpCommandQueue != nil, "Need to supply command queue");
            mNativeQueue = descMetal.mpCommandQueue;
            
            mType = desc.mType;
     
            // encoder creation is done on begin render pass or begin compute pass
#if 0
            mNativeCommandBuffer = [mNativeQueue commandBuffer];
            
            if(mType == RenderDriver::Common::CommandBufferType::Compute)
            {
                mNativeComputeCommandEncoder = [mNativeCommandBuffer computeCommandEncoder];
            }
            else if(mType == RenderDriver::Common::CommandBufferType::Copy)
            {
                mNativeBlitCommandEncoder = [mNativeCommandBuffer blitCommandEncoder];
            }
            else if(mType == RenderDriver::Common::CommandBufferType::Graphics)
            {
                WTFASSERT(descMetal.mpRenderPassDescriptor != nil, "Need to supply render pass descriptor");
                mNativeRenderCommandEncoder = [mNativeCommandBuffer renderCommandEncoderWithDescriptor: descMetal.mpRenderPassDescriptor];
            }
#endif // #if 0
            
#if defined(_DEBUG)
            miNumCommands = 0;
#endif // _DEBUG
            
            return mHandle;
        }

        /*
        **
        */
        void CCommandBuffer::setID(std::string const& name)
        {
            RenderDriver::Common::CObject::setID(name);

            
        }

        /*
        **
        */
        void CCommandBuffer::setPipelineState(
            RenderDriver::Common::CPipelineState& pipelineState,
            RenderDriver::Common::CDevice& device)
        {
            if(mType == RenderDriver::Common::CommandBufferType::Graphics)
            {
                id<MTLRenderPipelineState> nativePipelineState = (__bridge id<MTLRenderPipelineState>)pipelineState.getNativePipelineState();
                WTFASSERT(nativePipelineState != nil, "No native graphics pipeline state for \"%s\"", pipelineState.getID().c_str());
                [mNativeRenderCommandEncoder setRenderPipelineState: nativePipelineState];
            }
            else if(mType == RenderDriver::Common::CommandBufferType::Compute)
            {
                id<MTLComputePipelineState> nativePipelineState = (__bridge id<MTLComputePipelineState>)pipelineState.getNativePipelineState();
                WTFASSERT(nativePipelineState != nil, "No native graphics pipeline state for \"%s\"", pipelineState.getID().c_str());
                [mNativeComputeCommandEncoder setComputePipelineState: nativePipelineState];
            }
            else
            {
                WTFASSERT(0, "Invalid command buffer type: %d", mType);
            }

#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_PIPELINE_STATE);
#endif // _DEBUG
        }

        /*
        **
        */
        void CCommandBuffer::setDescriptor(
            PipelineInfo const& pipelineInfo,
            PLATFORM_OBJECT_HANDLE descriptorHeap,
            RenderDriver::Common::CDevice& device)
        {
            WTFASSERT(0, "%s : %d Implement me", __FILE__, __LINE__);
        }

        /*
        **
        */
        void CCommandBuffer::dispatch(
            uint32_t iGroupX,
            uint32_t iGroupY,
            uint32_t iGroupZ,
            uint32_t iLocalX,
            uint32_t iLocalY,
            uint32_t iLocalZ)
        {
            MTLSize threadGroupSize = MTLSizeMake(iGroupX, iGroupY, iGroupZ);
            MTLSize localThreadSize = MTLSizeMake(iLocalX, iLocalY, iLocalZ);
            
            [mNativeComputeCommandEncoder
                dispatchThreadgroups: threadGroupSize
                threadsPerThreadgroup: localThreadSize];
            
#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_DISPATCH);
#endif // _DEBUG
        }

        /*
        **
        */
        void CCommandBuffer::drawIndexInstanced(
            uint32_t iIndexCountPerInstance,
            uint32_t iNumInstances,
            uint32_t iStartIndexLocation,
            uint32_t iBaseVertexLocation,
            uint32_t iStartInstanceLocation,
            RenderDriver::Common::CBuffer* pIndexBuffer)
        {
            mState = RenderDriver::Common::CommandBufferState::Executing;
            
            id<MTLBuffer> nativeIndexBuffer = (__bridge id<MTLBuffer>)pIndexBuffer->getNativeBuffer();
            [mNativeRenderCommandEncoder 
                drawIndexedPrimitives: MTLPrimitiveTypeTriangle
                indexCount: iIndexCountPerInstance
                indexType: MTLIndexTypeUInt32
                indexBuffer: nativeIndexBuffer
                indexBufferOffset: iStartIndexLocation];
            
#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_DRAW_INDEXED_INSTANCE);
#endif // _DEBUG
        }

        /*
        **
        */
        void CCommandBuffer::setGraphicsConstantBufferView(
            uint64_t iConstantBufferAddress,
            uint32_t iDataSize)
        {
            
#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_CONSTANT_BUFFER_VIEW);
#endif // _DEBUG
        }

        /*
        **
        */
        void CCommandBuffer::setGraphicsConstantBufferView2(
            RenderDriver::Common::CBuffer& buffer,
            uint32_t iDataSize)
        {
            id<MTLBuffer> nativeBuffer = (__bridge id<MTLBuffer>)buffer.getNativeBuffer();
            [mNativeRenderCommandEncoder
                 setVertexBuffer: nativeBuffer
                 offset: 0
                 atIndex: 0];
        }

        /*
        **
        */
        void CCommandBuffer::setIndexAndVertexBufferView(
            uint64_t iIndexBufferAddress,
            uint32_t iIndexBufferSize,
            uint32_t iFormat,
            uint64_t iVertexBufferAddress,
            uint32_t iVertexBufferSize,
            uint32_t iVertexStrideSizeInBytes,
            RenderDriver::Common::CBuffer& indexBuffer,
            RenderDriver::Common::CBuffer& vertexBuffer)
        {
            id<MTLBuffer> nativeVertexBuffer = (__bridge id<MTLBuffer>)vertexBuffer.getNativeBuffer();
            [mNativeRenderCommandEncoder
                setVertexBuffer: nativeVertexBuffer
                offset: iVertexBufferAddress
                atIndex: RenderDriver::Metal::CPipelineState::kiVertexBufferIndex];
            
#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_DRAW_BUFFERS);
#endif // _DEBUG

        }

        /*
        **
        */
        void CCommandBuffer::setIndexAndMultipleVertexBufferViews(
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
            uint32_t iVertexUVStrideSizeInBytes)
        {
            WTFASSERT(0, "%s : %d Implement me", __FILE__, __LINE__);
            
#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_DRAW_BUFFERS);
#endif // _DEBUG

        }

        /*
        **
        */
        void CCommandBuffer::clearRenderTarget(PLATFORM_OBJECT_HANDLE renderTarget)
        {
            mState = RenderDriver::Common::CommandBufferState::Executing;
        }

        /*
        **
        */
        void CCommandBuffer::reset()
        {
            mState = RenderDriver::Common::CommandBufferState::Initialized;
            //WTFASSERT(0, "%s : %d Implement me", __FILE__, __LINE__);

            mNativeRenderCommandEncoder = nil;
            mNativeComputeCommandEncoder = nil;
            mNativeBlitCommandEncoder = nil;
            
            mNativeCommandBuffer = nil;
            
#if defined(_DEBUG)
            miNumCommands = 0;
#endif // _DEBUG

        }

        /*
        **
        */
        void CCommandBuffer::close()
        {
            mState = RenderDriver::Common::CommandBufferState::Closed;
            
            if(mType == RenderDriver::Common::CommandBufferType::Graphics && mNativeRenderCommandEncoder != nil)
            {
                [mNativeRenderCommandEncoder endEncoding];
            }
            else if(mType == RenderDriver::Common::CommandBufferType::Compute && mNativeComputeCommandEncoder != nil)
            {
                [mNativeComputeCommandEncoder endEncoding];
            }
            else if(mType == RenderDriver::Common::CommandBufferType::Copy && mNativeBlitCommandEncoder != nil)
            {
                [mNativeBlitCommandEncoder endEncoding];
            }
            else
            {
                WTFASSERT(0, "no native command buffer");
            }
            
        }

        /*
        **
        */
        void* CCommandBuffer::getNativeCommandList()
        {
            return (__bridge void*)mNativeCommandBuffer;
        }
    
        /*
        **
        */
        void CCommandBuffer::beginRenderPass(MTLRenderPassDescriptor* pRenderPassDescriptor)
        {
            mNativeCommandBuffer = [mNativeQueue commandBuffer];
            mNativeRenderCommandEncoder = [mNativeCommandBuffer renderCommandEncoderWithDescriptor: pRenderPassDescriptor];
        }
    
        /*
        **
        */
        void CCommandBuffer::beginComputePass(MTLComputePassDescriptor* pComputePassDescriptor)
        {
            mNativeCommandBuffer = [mNativeQueue commandBuffer];
            mNativeComputeCommandEncoder = [mNativeCommandBuffer computeCommandEncoder];
        }
    
        /*
        **
        */
        void CCommandBuffer::beginCopy()
        {
            mNativeCommandBuffer = [mNativeQueue commandBuffer];
            mNativeBlitCommandEncoder = [mNativeCommandBuffer blitCommandEncoder];
        }
    
    }   // Common

}   // RenderDriver
