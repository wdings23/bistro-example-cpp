#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/PhysicalDeviceDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

#include <assert.h>

#include "dxgi1_6.h"
#include "d3d12.h"

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CDevice::create(CreateDesc& createDesc)
        {
            RenderDriver::Common::CDevice::create(createDesc);

            RenderDriver::DX12::CPhysicalDevice* pPhysicalDeviceDX12 = static_cast<RenderDriver::DX12::CPhysicalDevice*>(createDesc.mpPhyiscalDevice);

            Microsoft::WRL::ComPtr<IDXGIAdapter1> pAdapter = pPhysicalDeviceDX12->getNativeAdapter();
            HRESULT hr = D3D12CreateDevice(
                pAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                IID_PPV_ARGS(&mNativeDevice));

            assert(SUCCEEDED(hr));

            mHandle = 2;

            return mHandle;
        }

        /*
        **
        */
        void CDevice::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
            mNativeDevice->SetName(wName.c_str());
        }

        /*
        **
        */
        void* CDevice::getNativeDevice()
        {
            return mNativeDevice.Get();
        }

    }   // DX12

}   // RenderDriver