#include <render-driver/DX12/PhysicalDeviceDX12.h>
#include <LogPrint.h>

#include <assert.h>

#include <wtfassert.h>

#include "dxgi1_6.h"
#include "d3d12.h"

using namespace Microsoft::WRL;

/*
**
*/
namespace RenderDriver
{
    namespace DX12
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPhysicalDevice::create(Descriptor const& desc)
        {
            RenderDriver::Common::CPhysicalDevice::create(desc);

            uint32_t iDXGIFactoryFlags = 0;
            HRESULT hr = CreateDXGIFactory2(iDXGIFactoryFlags, IID_PPV_ARGS(&mFactory));
            assert(SUCCEEDED(hr));

            if(desc.mAdapterType == AdapterType::Software)
            {
                getSoftwareAdapter(mFactory.Get());
            }
            else
            {
                ComPtr<IDXGIAdapter1> hardwareAdapter;
                getHardwareAdapter(mFactory.Get(), &hardwareAdapter);
            }

            return mHandle;
        }

        /*
        **
        */
        void CPhysicalDevice::getHardwareAdapter(
            IDXGIFactory4* pFactory,
            IDXGIAdapter1** pHardwareAdapter)
        {
            Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

            for(uint32_t i = 0;; i++)
            {
                HRESULT hr = pFactory->EnumAdapters1(i, &adapter);
                if(hr == DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }

                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                char szBuffer[2048];
                wcstombs(szBuffer, desc.Description, sizeof(szBuffer));
                DEBUG_PRINTF("adapter : %s\n", szBuffer);
                
                // don't want software emulation adapter
                if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                //if(desc.Flags == 0)
                {
                    continue;
                }

                hr = D3D12CreateDevice(
                    adapter.Get(),
                    D3D_FEATURE_LEVEL_11_0,
                    _uuidof(ID3D12Device),
                    nullptr);

                if(SUCCEEDED(hr))
                {
                    break;
                }
            }

            mAdapter = adapter.Detach();

        }

        /*
        **
        */
        void CPhysicalDevice::getSoftwareAdapter(
            IDXGIFactory4* pFactory)
        {
            mFactory->EnumWarpAdapter(IID_PPV_ARGS(&mAdapter));
            DXGI_ADAPTER_DESC warpAdapterDesc;
            mAdapter->GetDesc(&warpAdapterDesc);
            char szBuffer[2048];
            wcstombs(szBuffer, warpAdapterDesc.Description, sizeof(szBuffer));
            DEBUG_PRINTF("software adapter : %s\n", szBuffer);

            HRESULT hr = D3D12CreateDevice(
                mAdapter.Get(),
                D3D_FEATURE_LEVEL_11_0,
                __uuidof(ID3D12Device),
                nullptr);

            WTFASSERT(SUCCEEDED(hr), "Can\'t create software adapter");
        }

    }   // DX12

}   // RenderDriver

