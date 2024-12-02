#include <render-driver/DX12/DescriptorHeapDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

#include <serialize_utils.h>
#include <assert.h>

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        PLATFORM_OBJECT_HANDLE CDescriptorHeap::create(
            RenderDriver::Common::DescriptorHeapDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CDescriptorHeap::create(desc, device);

            RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
            ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

            //assert(desc.miNumDescriptors > 0);

            // descriptor heap
            D3D12_DESCRIPTOR_HEAP_DESC shaderResourceViewHeapDesc = {};
            shaderResourceViewHeapDesc.NumDescriptors = (UINT)desc.miNumDescriptors;
            shaderResourceViewHeapDesc.Type = SerializeUtils::DX12::convert(desc.mType);
            shaderResourceViewHeapDesc.Flags = SerializeUtils::DX12::convert(desc.mFlag);
            HRESULT hr = nativeDevice->CreateDescriptorHeap(&shaderResourceViewHeapDesc, IID_PPV_ARGS(&mNativeDescriptorHeap));
            assert(SUCCEEDED(hr));
            
            return mHandle;
        }

        /*
        **
        */
        void CDescriptorHeap::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
            mNativeDescriptorHeap->SetName(wName.c_str());
        }

        /*
        **
        */
        void* CDescriptorHeap::getNativeDescriptorHeap()
        {
            return mNativeDescriptorHeap.Get();
        }

        /*
        **
        */
        uint64_t CDescriptorHeap::getCPUHandle(uint32_t iIndex, RenderDriver::Common::CDevice& device)
        {
            ID3D12Device* pDX12Device = reinterpret_cast<ID3D12Device*>(device.getNativeDevice());
            uint32_t iDescriptorOffsetSize = pDX12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            
            D3D12_CPU_DESCRIPTOR_HANDLE handle = mNativeDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += iDescriptorOffsetSize * iIndex;

            return handle.ptr;
        }

        /*
        **
        */
        uint64_t CDescriptorHeap::getGPUHandle(uint32_t iIndex, RenderDriver::Common::CDevice& device)
        {
            ID3D12Device* pDX12Device = reinterpret_cast<ID3D12Device*>(device.getNativeDevice());
            uint32_t iDescriptorOffsetSize = pDX12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_GPU_DESCRIPTOR_HANDLE handle = mNativeDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
            handle.ptr += iDescriptorOffsetSize * iIndex;

            return handle.ptr;
        }

        /*
        **
        */
        void CDescriptorHeap::setImageView(
            RenderDriver::Common::CImage* pImage, 
            uint32_t iIndex,
            RenderDriver::Common::CDevice& device)
        {
            ID3D12Device* pDX12Device = reinterpret_cast<ID3D12Device*>(device.getNativeDevice());
            uint32_t iDescriptorOffsetSize = pDX12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            D3D12_CPU_DESCRIPTOR_HANDLE handle = mNativeDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            handle.ptr += iDescriptorOffsetSize * iIndex;

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = SerializeUtils::DX12::convert(pImage->getFormat());
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

            pDX12Device->CreateShaderResourceView(
                reinterpret_cast<ID3D12Resource*>(pImage->getNativeImage()),
                &srvDesc,
                handle);
        }

    }   // DX12

}   // RenderDriver