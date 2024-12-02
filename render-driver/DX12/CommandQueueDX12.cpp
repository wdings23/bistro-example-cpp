#include <render-driver/DX12/CommandQueueDX12.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/CommandBufferDX12.h>
#include <render-driver/DX12/UtilsDX12.h>
#include <render-driver/DX12/FenceDX12.h>

#include <assert.h>

namespace RenderDriver
{
    namespace DX12
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandQueue::create(RenderDriver::Common::CCommandQueue::CreateDesc& desc)
        {
            // compute command queue
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

            if(desc.mType == Type::Compute)
            {
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            }
            else if(desc.mType == Type::Graphics)
            {
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            }
            else if(desc.mType == Type::Copy)
            {
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            }
            else if(desc.mType == Type::CopyGPU)
            {
                queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            }

            RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(*desc.mpDevice);
            ID3D12Device* pDeviceDX12 = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

            HRESULT hr = pDeviceDX12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mNativeCommandQueue));
            assert(SUCCEEDED(hr));

            mType = desc.mType;

            mHandle = desc.mpDevice->registerCommandQueue(this);

            return mHandle;
        }

        /*
        **
        */
        void CCommandQueue::execCommandBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CDevice& device)
        {
            commandBuffer.setState(RenderDriver::Common::CommandBufferState::Executing);
            RenderDriver::DX12::CCommandBuffer& dx12CommandBuffer = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);

            ID3D12CommandList* aCommandLists[] = { static_cast<ID3D12GraphicsCommandList*>(dx12CommandBuffer.getNativeCommandList()) };
            mNativeCommandQueue->ExecuteCommandLists(1, aCommandLists);
        }

        /*
        **
        */
        void CCommandQueue::execCommandBuffer2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CFence& fence,
            uint64_t iFenceValue,
            RenderDriver::Common::CDevice& device)
        {
            commandBuffer.setState(RenderDriver::Common::CommandBufferState::Executing);
            RenderDriver::DX12::CCommandBuffer& dx12CommandBuffer = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);

            ID3D12CommandList* aCommandLists[] = { static_cast<ID3D12GraphicsCommandList*>(dx12CommandBuffer.getNativeCommandList()) };
            mNativeCommandQueue->ExecuteCommandLists(1, aCommandLists);

            if(iFenceValue != UINT64_MAX)
            {
                ID3D12Fence* pFence = static_cast<ID3D12Fence*>(fence.getNativeFence());
                mNativeCommandQueue->Signal(pFence, iFenceValue);
            }
        }

        /*
        **
        */
        void CCommandQueue::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
            mNativeCommandQueue->SetName(wName.c_str());
        }

        /*
        **
        */
        void* CCommandQueue::getNativeCommandQueue()
        {
            return mNativeCommandQueue.Get();
        }

    }   // DX12 

}   // RenderDriver