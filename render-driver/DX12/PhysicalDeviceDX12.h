#pragma once

#include <render-driver/PhysicalDevice.h>
#include <render-driver/Device.h>

#include "wrl/client.h"
#include "dxgi1_6.h"

namespace RenderDriver
{
    namespace DX12
    {
        class CPhysicalDevice : public RenderDriver::Common::CPhysicalDevice
        {
        public:
            
            CPhysicalDevice() = default;
            virtual ~CPhysicalDevice() = default;

            virtual PLATFORM_OBJECT_HANDLE create(Descriptor const& desc);

            inline Microsoft::WRL::ComPtr<IDXGIAdapter1> getNativeAdapter()
            {
                return mAdapter;
            }

            inline Microsoft::WRL::ComPtr<IDXGIFactory4> getNativeFactory()
            {
                return mFactory;
            }

            inline HWND getHWND()
            {
                return mHWND;
            }

        protected:

            Microsoft::WRL::ComPtr<IDXGIAdapter1>                       mAdapter;
            Microsoft::WRL::ComPtr<IDXGIFactory4>                       mFactory;
            HWND                                                        mHWND;

            

        protected:
            void getHardwareAdapter(
                IDXGIFactory4* pFactory,
                IDXGIAdapter1** pHardwareAdapter);

            void getSoftwareAdapter(
                IDXGIFactory4* pFactory);

        };

    }   // DX12

}   // RenderDriver