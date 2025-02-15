#pragma once

#include <render-driver/Device.h>
#import <Metal/Metal.h>

#include <memory>

namespace RenderDriver
{
    namespace Metal
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
            id<MTLDevice>                        mNativeDevice;

        };

    }   // DX12

}   // RenderDriver
