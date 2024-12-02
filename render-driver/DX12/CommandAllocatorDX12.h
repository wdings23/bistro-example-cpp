#pragma once

#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/CommandAllocator.h>


namespace RenderDriver
{
    namespace DX12
    {
        class CCommandAllocator : public RenderDriver::Common::CCommandAllocator
        {
        public:
            CCommandAllocator() = default;
            virtual ~CCommandAllocator() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::CommandAllocatorDescriptor const& desc,
                RenderDriver::Common::CDevice& device);
        
            virtual void reset();

            virtual void setID(std::string const& id);

            virtual void* getNativeCommandAllocator();

        protected:
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  mNativeCommandAllocator;
        };


    }   // Common

}   // RenderDriver
