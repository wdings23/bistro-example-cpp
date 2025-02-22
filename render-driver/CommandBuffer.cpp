#include "Device.h"
#include "CommandBuffer.h"

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandBuffer::create(
            CommandBufferDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            mHandle = device.assignHandleAndUpdate();
            mDesc = desc;
            mState = RenderDriver::Common::CommandBufferState::Initialized;

            return mHandle;
        }

        /*
        **
        */
        void CCommandBuffer::close()
        {
            mState = RenderDriver::Common::CommandBufferState::Closed;
        }

        /*
        **
        */
        void CCommandBuffer::reset()
        {
            mState = RenderDriver::Common::CommandBufferState::Initialized;
        }

        /*
        **
        */
        void* CCommandBuffer::getNativeCommandList()
        {
            return nullptr;
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
        }

        /*
        **
        */
        void CCommandBuffer::drawIndexInstanced(
            uint32_t iIndexCountPerInstance,
            uint32_t iNumInstances,
            uint32_t iStartIndexLocation,
            uint32_t iBaseVertexLocation,
            uint32_t iStartInstanceLocation)
        {
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
        }

        /*
        **
        */
        void CCommandBuffer::setGraphicsConstantBufferView(
            uint64_t iConstantBufferAddress,
            uint32_t iDataSize)
        {
        }

        /*
        **
        */
        void CCommandBuffer::setGraphicsConstantBufferView2(
            RenderDriver::Common::CBuffer& buffer,
            uint32_t iDataSize)
        {
        }

        /*
        **
        */
        void CCommandBuffer::clearRenderTarget(PLATFORM_OBJECT_HANDLE renderTarget)
        {
        }

        /*
        **
        */
        void CCommandBuffer::drawIndirectCount(
            RenderDriver::Common::CBuffer& drawIndexBuffer,
            RenderDriver::Common::CBuffer& drawCountBuffer,
            uint32_t iCountBufferOffset
        )
        {

        }

#if defined(_DEBUG)
        /*
        **
        */
        void CCommandBuffer::addCommand(uint32_t iCommandType)
        {
            maiCommands[miNumCommands] = iCommandType;
            ++miNumCommands;
        }
#endif // _DEBUG


    }   // Common

}   // RenderDriver
