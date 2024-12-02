#include <render-driver/DX12/CommandAllocatorDX12.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandAllocator::create(
            RenderDriver::Common::CommandAllocatorDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CCommandAllocator::create(desc, device);

            D3D12_COMMAND_LIST_TYPE commandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
            if(desc.mType == RenderDriver::Common::CommandBufferType::Compute)
            {
                commandListType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
            }
            else if(desc.mType == RenderDriver::Common::CommandBufferType::Copy)
            {
                commandListType = D3D12_COMMAND_LIST_TYPE_COPY;
            }

            RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
            ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

            // graphics command list
            HRESULT hr = nativeDevice->CreateCommandAllocator(
                commandListType,
                IID_PPV_ARGS(&mNativeCommandAllocator));
            assert(SUCCEEDED(hr));

            return mHandle;
        }

        /*
        **
        */
        void CCommandAllocator::reset()
        {
            mNativeCommandAllocator->Reset();
        }

        /*
        **
        */
        void CCommandAllocator::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
            mNativeCommandAllocator->SetName(wName.c_str());
        }

        /*
        **
        */
        void* CCommandAllocator::getNativeCommandAllocator()
        {
            return mNativeCommandAllocator.Get();
        }

    }   // Common

}   // RenderDriver