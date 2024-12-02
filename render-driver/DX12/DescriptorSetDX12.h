#pragma once

#include <render-driver/Device.h>
#include <render-driver/DescriptorSet.h>

namespace RenderDriver
{
    namespace DX12
    {
        class CDescriptorSet : public RenderDriver::Common::CDescriptorSet
        {
        public:

            CDescriptorSet() = default;
            virtual ~CDescriptorSet() = default;

            PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::DescriptorSetDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& name);

            virtual void* getNativeDescriptorSet();

        protected:
            ComPtr<ID3D12RootSignature>         mRootSignature;

        protected:

            void addShaderRange(
                std::vector<D3D12_DESCRIPTOR_RANGE>& aRanges,
                D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType,
                uint32_t iNumDescriptors,
                std::string const& name,
                uint32_t iRegisterSpace = 0);

            void initDescriptorRange(
                D3D12_DESCRIPTOR_RANGE& descriptorRange,
                D3D12_DESCRIPTOR_RANGE_TYPE type,
                uint32_t iNumDescriptors,
                uint32_t iBaseRegister,
                uint32_t iRegisterSpace = 0);

            void initRootParameterAsDescriptorTable(
                D3D12_ROOT_PARAMETER& rootParameter,
                D3D12_DESCRIPTOR_RANGE const* aDescriptorRanges,
                uint32_t iNumDescriptorRanges,
                D3D12_SHADER_VISIBILITY shaderVisibility);

            void initRootSignatureDesc(
                D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
                D3D12_ROOT_PARAMETER const* pRootParameter,
                uint32_t iNumRootParameters,
                D3D12_STATIC_SAMPLER_DESC const* pSamplerDesc,
                uint32_t iNumSamplerDescs,
                D3D12_ROOT_SIGNATURE_FLAGS flags);

        };

    }   // Common

}   // RenderDriver