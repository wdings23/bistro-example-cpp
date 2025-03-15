#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/CommandBufferDX12.h>
#include <render-driver/DX12/CommandAllocatorDX12.h>
#include <render-driver/DX12/PipelineStateDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        PLATFORM_OBJECT_HANDLE CCommandBuffer::create(
            RenderDriver::Common::CommandBufferDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
            if(desc.mType == RenderDriver::Common::CommandBufferType::Compute)
            {
                commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            }
            else if(desc.mType == RenderDriver::Common::CommandBufferType::Copy)
            {
                commandListType = D3D12_COMMAND_LIST_TYPE_COPY;
            }

            RenderDriver::Common::CCommandBuffer::create(desc, device);

            RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
            ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

            RenderDriver::DX12::CCommandAllocator* pDX12CommandAllocator = static_cast<RenderDriver::DX12::CCommandAllocator*>(desc.mpCommandAllocator);

            ID3D12PipelineState* pNativePipelineState = nullptr;
            if(desc.mType != RenderDriver::Common::CommandBufferType::Copy && desc.mpPipelineState != nullptr)
            {
                RenderDriver::DX12::CPipelineState* pPipelineState = static_cast<RenderDriver::DX12::CPipelineState*>(desc.mpPipelineState);
                pNativePipelineState = static_cast<ID3D12PipelineState*>(pPipelineState->getNativePipelineState());
            }

            HRESULT hr = nativeDevice->CreateCommandList(
                0,
                commandListType,
                static_cast<ID3D12CommandAllocator*>(pDX12CommandAllocator->getNativeCommandAllocator()),
                pNativePipelineState,
                IID_PPV_ARGS(&mNativeCommandList));
            assert(SUCCEEDED(hr));

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
            std::wstring wCommandListName = RenderDriver::DX12::Utils::convertToLPTSTR(name);
            mNativeCommandList->SetName(wCommandListName.c_str());
        }

        /*
        **
        */
        void CCommandBuffer::setPipelineState(
            RenderDriver::Common::CPipelineState& pipelineState,
            RenderDriver::Common::CDevice& device)
        {
            mNativeCommandList->SetPipelineState(reinterpret_cast<ID3D12PipelineState*>(pipelineState.getNativePipelineState()));

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

        }

        /*
        **
        */
        void CCommandBuffer::dispatch(
            uint32_t iX,
            uint32_t iY,
            uint32_t iZ)
        {
            mState = RenderDriver::Common::CommandBufferState::Executing;
            mNativeCommandList->Dispatch(iX, iY, iZ);
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
            mNativeCommandList->DrawIndexedInstanced(
                iIndexCountPerInstance,
                iNumInstances,
                iStartIndexLocation,
                iBaseVertexLocation,
                iStartInstanceLocation);
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
            mNativeCommandList->SetGraphicsRootConstantBufferView(
                0,
                iConstantBufferAddress);
#if defined(_DEBUG)
            addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_SET_CONSTANT_BUFFER_VIEW);
#endif // _DEBUG 
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
            D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
            vertexBufferView.BufferLocation = iVertexBufferAddress;
            vertexBufferView.SizeInBytes = iVertexBufferSize;
            vertexBufferView.StrideInBytes = iVertexStrideSizeInBytes;

            D3D12_INDEX_BUFFER_VIEW indexBufferView;
            indexBufferView.BufferLocation = iIndexBufferAddress;
            indexBufferView.Format = DXGI_FORMAT_R32_UINT;
            indexBufferView.SizeInBytes = iIndexBufferSize;

            mNativeCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            mNativeCommandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            mNativeCommandList->IASetIndexBuffer(&indexBufferView);

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
            D3D12_VERTEX_BUFFER_VIEW aVertexBufferViews[3];

            // position
            aVertexBufferViews[0].BufferLocation =  iVertexPositionBufferAddress;
            aVertexBufferViews[0].SizeInBytes =     iVertexPositionBufferSize;
            aVertexBufferViews[0].StrideInBytes =   iVertexPositionStrideSizeInBytes;

            // normal
            aVertexBufferViews[1].BufferLocation =  iVertexNormalBufferAddress;
            aVertexBufferViews[1].SizeInBytes =     iVertexNormalBufferSize;
            aVertexBufferViews[1].StrideInBytes =   iVertexNormalStrideSizeInBytes;

            // uv
            aVertexBufferViews[2].BufferLocation =  iVertexUVBufferAddress;
            aVertexBufferViews[2].SizeInBytes =     iVertexUVBufferSize;
            aVertexBufferViews[2].StrideInBytes =   iVertexUVStrideSizeInBytes;

            D3D12_INDEX_BUFFER_VIEW indexBufferView;
            indexBufferView.BufferLocation = iIndexBufferAddress;
            indexBufferView.Format = DXGI_FORMAT(iFormat); // DXGI_FORMAT_R32_UINT;
            indexBufferView.SizeInBytes = iIndexBufferSize;

            mNativeCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            mNativeCommandList->IASetVertexBuffers(0, 3, aVertexBufferViews);
            mNativeCommandList->IASetIndexBuffer(&indexBufferView);

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
            RenderDriver::DX12::CCommandAllocator* pDX12CommandAllocator = 
                static_cast<RenderDriver::DX12::CCommandAllocator*>(mDesc.mpCommandAllocator);
            RenderDriver::DX12::CPipelineState* pDX12PipelineState =
                static_cast<RenderDriver::DX12::CPipelineState*>(mDesc.mpPipelineState);

            ID3D12PipelineState* pD3D12PipelineState = nullptr;
            if(pDX12PipelineState != nullptr)
            {
                pD3D12PipelineState = static_cast<ID3D12PipelineState*>(pDX12PipelineState->getNativePipelineState());
            }

            mNativeCommandList->Reset(static_cast<ID3D12CommandAllocator*>(pDX12CommandAllocator->getNativeCommandAllocator()), pD3D12PipelineState);

            mState = RenderDriver::Common::CommandBufferState::Initialized;

#if defined(_DEBUG)
            miNumCommands = 0;
#endif // _DEBUG 

        }

        /*
        **
        */
        void CCommandBuffer::close()
        {
            HRESULT hr = mNativeCommandList->Close();
            assert(hr == S_OK);
            mState = RenderDriver::Common::CommandBufferState::Closed;
        }

        /*
        **
        */
        void* CCommandBuffer::getNativeCommandList()
        {
            return mNativeCommandList.Get();
        }

    }   // Common

}   // RenderDriver
