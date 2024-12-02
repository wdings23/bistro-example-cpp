#pragma once

#include <render-driver/Device.h>
#include <render-driver/DX12/PhysicalDeviceDX12.h>

#include <memory>

#include "wrl/client.h"
#include "d3d12.h"
#include <pix3.h>

namespace RenderDriver
{
    namespace DX12
    {
        class CDevice : public RenderDriver::Common::CDevice
        {
        public:
            CDevice() = default;
            virtual ~CDevice() = default;

            virtual PLATFORM_OBJECT_HANDLE create(RenderDriver::Common::CDevice::CreateDesc& createDesc);
            
            virtual void* getNativeDevice();

            virtual void setID(std::string const& name);

        protected:
            Microsoft::WRL::ComPtr<ID3D12Device>                        mNativeDevice;

        };

    }   // DX12

}   // RenderDriver