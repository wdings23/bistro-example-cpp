#pragma once

#include <render-driver/Buffer.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

namespace RenderDriver
{
    namespace DX12
    {
        class CBuffer : public RenderDriver::Common::CBuffer
        {
        public:
            CBuffer() = default;
            virtual ~CBuffer() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::BufferDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void* getNativeBuffer();

            virtual void setID(std::string const& name);

            void copy(
                RenderDriver::Common::CBuffer& buffer,
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                uint32_t iDestOffset,
                uint32_t iSrcOffset,
                uint32_t iDataSize);

            virtual void setData(
                void* pSrcData,
                uint32_t iDataSize);

            virtual void setDataOpen(
                void* pSrcData,
                uint32_t iDataSize);

            virtual uint64_t getGPUVirtualAddress();

            void* getMemoryOpen(uint32_t iDataSize);

            virtual void releaseNativeBuffer();

        protected:
            
            Microsoft::WRL::ComPtr<ID3D12Resource>              mNativeBuffer;
            D3D12_RESOURCE_DESC                                 mResourceDesc;
            uint8_t*                                            mpDestData = nullptr;
        };

    }   // DX12

}   // RenderDriver