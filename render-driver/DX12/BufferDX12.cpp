#include <render-driver/DX12/BufferDX12.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/UtilsDX12.h>
#include <render-driver/DX12/CommandBufferDX12.h>
#include <serialize_utils.h>

#include <LogPrint.h>

#include <assert.h>

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        PLATFORM_OBJECT_HANDLE CBuffer::create(
            RenderDriver::Common::BufferDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CBuffer::create(desc, device);

            RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
            ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

            // descriptor heap properties
            D3D12_HEAP_PROPERTIES heapProperties;
            heapProperties.Type = SerializeUtils::DX12::convert(desc.mHeapType);
            heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProperties.CreationNodeMask = 1;
            heapProperties.VisibleNodeMask = 1;
            heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

            mResourceDesc.Width = (uint64_t)desc.miSize;
            mResourceDesc.Height = 1;
            mResourceDesc.DepthOrArraySize = 1;
            mResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            mResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            mResourceDesc.MipLevels = 1;
            mResourceDesc.SampleDesc.Count = 1;
            mResourceDesc.SampleDesc.Quality = 0;
            mResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            mResourceDesc.Flags = SerializeUtils::DX12::convert(desc.mFlags);

            D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
            mResourceState = RenderDriver::Common::ResourceStateFlagBits::Common;
            if((mResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) > 0)
            {
                mResourceDesc.Width = desc.miSize;
            }

            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE | D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
            if(desc.mHeapType == RenderDriver::Common::HeapType::Default)
            {
                heapFlags |= D3D12_HEAP_FLAG_SHARED;
            }

            if(desc.mHeapType == RenderDriver::Common::HeapType::Custom)
            {
                heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_COMBINE;
                heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
            }
            else if(desc.mHeapType == RenderDriver::Common::HeapType::Upload)
            {
                resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
                mResourceState = RenderDriver::Common::ResourceStateFlagBits::GenericRead;
            }

            if(desc.mOverrideState != RenderDriver::Common::ResourceStateFlagBits::None)
            {
                resourceState = SerializeUtils::DX12::convert(desc.mOverrideState);
                mResourceState = desc.mOverrideState;
            }

            // create resource 
            HRESULT hr = nativeDevice->CreateCommittedResource(
                &heapProperties,
                heapFlags,
                &mResourceDesc,
                resourceState,
                nullptr,
                IID_PPV_ARGS(&mNativeBuffer));

            assert(hr == S_OK);

            mpDestData = nullptr;

            return mHandle;
        }

        /*
        **
        */
        void CBuffer::copy(
            RenderDriver::Common::CBuffer& buffer,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iDestOffset,
            uint32_t iSrcOffset,
            uint32_t iDataSize)
        {
            RenderDriver::DX12::CBuffer& dx12Buffer = static_cast<RenderDriver::DX12::CBuffer&>(buffer);
            RenderDriver::DX12::CCommandBuffer& dx12CommandBuffer = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);

            static_cast<ID3D12GraphicsCommandList*>(dx12CommandBuffer.getNativeCommandList())->CopyBufferRegion(
                static_cast<ID3D12Resource*>(dx12Buffer.getNativeBuffer()),
                iDestOffset,
                mNativeBuffer.Get(),
                iSrcOffset,
                iDataSize);


#if defined(_DEBUG)
            commandBuffer.addCommand(RenderDriver::Common::CCommandBuffer::COMMAND_COPY);
#endif // DEBUG
        }

        /*
        **
        */
        void CBuffer::setDataOpen(
            void* pSrcData,
            uint32_t iDataSize)
        {
            if(mpDestData == nullptr)
            {
                HRESULT hr = mNativeBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mpDestData));
                assert(SUCCEEDED(hr));
            }

            memcpy(mpDestData, pSrcData, iDataSize);
        }

        /*
        **
        */
        void CBuffer::setData(
            void* pSrcData,
            uint32_t iDataSize)
        {
            HRESULT hr = mNativeBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mpDestData));
            assert(SUCCEEDED(hr));
            
            memcpy(mpDestData, pSrcData, iDataSize);

            mNativeBuffer->Unmap(0, nullptr);
        }

        /*
        **
        */
        void CBuffer::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
            mNativeBuffer->SetName(wName.c_str());
        }

        /*
        **
        */
        void* CBuffer::getNativeBuffer()
        {
            return mNativeBuffer.Get();
        }

        /*
        **
        */
        uint64_t CBuffer::getGPUVirtualAddress()
        {
            return mNativeBuffer.Get()->GetGPUVirtualAddress();
        }

        /*
        **
        */
        void CBuffer::releaseNativeBuffer()
        {
            mNativeBuffer.Reset();
            mNativeBuffer = nullptr;
        }

    }   // DX12

}   // RenderDriver