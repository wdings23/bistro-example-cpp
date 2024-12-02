#include <render-driver/DX12/BufferViewDX12.h>
#include <render-driver/DX12/BufferDX12.h>
#include <render-driver/DX12/DescriptorHeapDX12.h>
#include <render-driver/DX12/UtilsDX12.h>
#include <LogPrint.h>

namespace RenderDriver
{
    namespace DX12
    {
        PLATFORM_OBJECT_HANDLE CBufferView::create(
            RenderDriver::Common::BufferViewDescriptor& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CBufferView::create(desc, device);
        
            RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
            ID3D12Device* pDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

            RenderDriver::DX12::CBuffer* pDX12Buffer = static_cast<RenderDriver::DX12::CBuffer*>(desc.mpBuffer);
            RenderDriver::DX12::CBuffer* pDX12CounterBuffer = nullptr;
            if(desc.mpCounterBuffer != nullptr)
            {
                pDX12CounterBuffer = static_cast<RenderDriver::DX12::CBuffer*>(desc.mpCounterBuffer);
            }

            RenderDriver::DX12::CDescriptorHeap* pDX12DescriptorHeap = static_cast<RenderDriver::DX12::CDescriptorHeap*>(mDesc.mpDescriptorHeap);
            ID3D12DescriptorHeap* nativeHeap = static_cast<ID3D12DescriptorHeap*>(pDX12DescriptorHeap->getNativeDescriptorHeap());
            D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = nativeHeap->GetCPUDescriptorHandleForHeapStart();
            uint32_t iDescriptorOffset = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            cpuDescriptorHandle.ptr += (iDescriptorOffset * mDesc.miDescriptorOffset);

            if(desc.mViewType == RenderDriver::Common::ResourceViewType::UnorderedAccessView)
            {
DEBUG_PRINTF("\ttype: UAV\n");
                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                uavDesc.Format = SerializeUtils::DX12::convert(desc.mFormat);
                uavDesc.Buffer.FirstElement = 0;
                uavDesc.Buffer.Flags = SerializeUtils::DX12::convert(desc.mFlags);
                assert(desc.miStructStrideSize > 0);
                uavDesc.Buffer.NumElements = desc.miSize / desc.miStructStrideSize;
                uavDesc.Buffer.StructureByteStride = desc.miStructStrideSize;
                uavDesc.Buffer.CounterOffsetInBytes = 0;
                //uavDesc.Buffer.CounterOffsetInBytes = RenderDriver::DX12::Utils::alignForUavCounter(desc.miSize);

                pDevice->CreateUnorderedAccessView(
                    static_cast<ID3D12Resource*>(pDX12Buffer->getNativeBuffer()),
                    desc.mpCounterBuffer ? static_cast<ID3D12Resource*>(pDX12CounterBuffer->getNativeBuffer()) : nullptr,
                    &uavDesc,
                    cpuDescriptorHandle);
            }
            else if(desc.mViewType == RenderDriver::Common::ResourceViewType::ShaderResourceView)
            {
DEBUG_PRINTF("\ttype: SRV\n");
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = SerializeUtils::DX12::convert(desc.mFormat);
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Buffer.FirstElement = 0;
                assert(desc.miStructStrideSize > 0);
                srvDesc.Buffer.NumElements = desc.miSize / desc.miStructStrideSize;
                srvDesc.Buffer.StructureByteStride = desc.miStructStrideSize;
                srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

                pDevice->CreateShaderResourceView(
                    static_cast<ID3D12Resource*>(pDX12Buffer->getNativeBuffer()),
                    &srvDesc,
                    cpuDescriptorHandle);
            }
            else if(desc.mViewType == RenderDriver::Common::ResourceViewType::ConstantBufferView)
            {
DEBUG_PRINTF("\ttype: CBV\n");
                D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc;
                viewDesc.BufferLocation = static_cast<ID3D12Resource*>(pDX12Buffer->getNativeBuffer())->GetGPUVirtualAddress();
                viewDesc.SizeInBytes = UINT(desc.miSize);

                pDevice->CreateConstantBufferView(
                    &viewDesc,
                    cpuDescriptorHandle);
            }

            return mHandle;
        }

    }   // DX12

}   // RenderDriver