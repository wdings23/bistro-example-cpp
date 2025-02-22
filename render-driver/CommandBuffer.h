#pragma once

#include <render-driver/Object.h>
#include <render-driver/Device.h>
#include <render-driver/PipelineInfo.h>
#include <render-driver/CommandAllocator.h>
#include <render-driver/Buffer.h>
#include <render-driver/PipelineState.h>

#include <utils/serialize_utils.h>

namespace RenderDriver
{
    namespace Common
    {
        struct CommandBufferDescriptor
        {
            CommandBufferType                mType;
            CPipelineState*                  mpPipelineState;
            CCommandAllocator*               mpCommandAllocator;
        };

        struct CCommandBuffer : public CObject
        {
            enum
            {
                COMMAND_COPY = 1,
                COMMAND_DRAW,
                COMMAND_DISPATCH,
                COMMAND_SET_DRAW_BUFFERS,
                COMMAND_SET_CONSTANT_BUFFER_VIEW,
                COMMAND_DRAW_INDEXED_INSTANCE,
                COMMAND_SET_PIPELINE_STATE,

                NUM_COMMAND_TYPES,
            };
        
        public:
            CCommandBuffer() = default;
            virtual ~CCommandBuffer() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                CommandBufferDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void reset();

            virtual void* getNativeCommandList();

#if 0
            virtual void copyBuffer(
                RenderDriver::Common::CBuffer& destination,
                RenderDriver::Common::CBuffer& source,
                uint32_t iDestOffset,
                uint32_t iSrcOffset,
                uint32_t iSize);
            
            
    
            virtual void setDescriptor(
                PipelineInfo const& pipelineInfo, 
                PLATFORM_OBJECT_HANDLE descriptorHeap,
                CDevice& device) = 0;
#endif // #if 0

            virtual void setPipelineState(
                RenderDriver::Common::CPipelineState&,
                CDevice& device) = 0;

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
                uint32_t iStartInstanceLocation);

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
                uint32_t iVertexStrideSizeInBytes,
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

            virtual void close();

            inline CommandBufferType getType()
            {
                return mDesc.mType;
            }

            inline void setState(CommandBufferState const& state)
            {
                mState = state;
            }

            inline CommandBufferState getState()
            {
                return mState;
            }

#if defined(_DEBUG)
            void addCommand(uint32_t iCommandType);
            inline uint32_t getNumCommands()
            {
                return miNumCommands;
            }
#endif // DEBUG 

        protected:
            CommandBufferDescriptor         mDesc;

            CommandBufferState              mState;


#if defined(_DEBUG)
            uint32_t                        miNumCommands;
            uint32_t                        maiCommands[512];
#endif // DEBUG 
        };

    }   // Common

}   // RenderDriver
