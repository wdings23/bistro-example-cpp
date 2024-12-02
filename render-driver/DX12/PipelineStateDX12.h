#pragma once

#include <render-driver/PipelineState.h>

namespace RenderDriver
{
    namespace DX12
    {
        class CPipelineState : public RenderDriver::Common::CPipelineState
        {
        public:
            CPipelineState() = default;
            virtual ~CPipelineState() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::GraphicsPipelineStateDescriptor const& desc,
                RenderDriver::Common::CDevice & device);

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::ComputePipelineStateDescriptor const& desc,
                RenderDriver::Common::CDevice & device);

            virtual void setID(std::string const& name);

            virtual void* getNativePipelineState();

        protected:
            ComPtr<ID3D12PipelineState>     mNativePipelineState;

        };


    }   // DX12

}   // RenderDriver