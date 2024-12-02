#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/DescriptorSetDX12.h>

#include <render-driver/DX12/UtilsDX12.h>
#include <LogPrint.h>
#include <sstream>

#define DEBUG_PRINT_OUT 1

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        PLATFORM_OBJECT_HANDLE CDescriptorSet::create(
            RenderDriver::Common::DescriptorSetDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CDescriptorSet::create(desc, device);

			RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
			ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

			// shader resource table
			uint32_t iNumTextures = 0;
			std::vector<D3D12_STATIC_SAMPLER_DESC> aSamplerDescriptor;
			std::vector<D3D12_DESCRIPTOR_RANGE> aTotalRanges;
			std::vector<D3D12_SHADER_VISIBILITY> aShaderVisibilities;
			for(auto const& shaderResourceInfo : *desc.mpaShaderResources)
			{
				uint32_t iRegisterSpace = 0;
				D3D12_DESCRIPTOR_RANGE_TYPE range = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				if(shaderResourceInfo.mViewType == RenderDriver::Common::ResourceViewType::ShaderResourceView)
				{
					range = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
				}
				else if(shaderResourceInfo.mViewType == RenderDriver::Common::ResourceViewType::UnorderedAccessView)
				{
					range = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
				}
				else if(shaderResourceInfo.mViewType == RenderDriver::Common::ResourceViewType::ConstantBufferView)
				{
					range = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
				}
				else if(shaderResourceInfo.mViewType == RenderDriver::Common::ResourceViewType::None)
				{
					continue;
				}
				else
				{
					assert(0);
				}

				if(desc.mPipelineType == PipelineType::GRAPHICS_PIPELINE_TYPE)
				{
					// sampler
					if(aSamplerDescriptor.size() <= 0 &&
						shaderResourceInfo.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
					{
						// TODO: serialize this
						D3D12_STATIC_SAMPLER_DESC samplerDescriptor = {};
						samplerDescriptor.Filter = D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
						samplerDescriptor.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
						samplerDescriptor.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
						samplerDescriptor.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
						samplerDescriptor.MipLODBias = 0;
						samplerDescriptor.MaxAnisotropy = 0;
						samplerDescriptor.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
						samplerDescriptor.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
						samplerDescriptor.MinLOD = 0.0f;
						samplerDescriptor.MaxLOD = 0.0f;
						samplerDescriptor.ShaderRegister = 0;
						samplerDescriptor.RegisterSpace = 0;
						samplerDescriptor.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

						aSamplerDescriptor.push_back(samplerDescriptor);
						aShaderVisibilities.push_back(D3D12_SHADER_VISIBILITY_PIXEL);

						samplerDescriptor.Filter = D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
						samplerDescriptor.ShaderRegister = 1;
						aSamplerDescriptor.push_back(samplerDescriptor);
						aShaderVisibilities.push_back(D3D12_SHADER_VISIBILITY_PIXEL);

						samplerDescriptor.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
						samplerDescriptor.ShaderRegister = 2;
						aSamplerDescriptor.push_back(samplerDescriptor);
						aShaderVisibilities.push_back(D3D12_SHADER_VISIBILITY_ALL);
					}
					else
					{
						aShaderVisibilities.push_back(D3D12_SHADER_VISIBILITY_ALL);
					}
				}
				else
				{
					// sampler
					if(aSamplerDescriptor.size() <= 0 &&
						shaderResourceInfo.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
					{
						D3D12_STATIC_SAMPLER_DESC samplerDescriptor = {};
						samplerDescriptor.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
						samplerDescriptor.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
						samplerDescriptor.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
						samplerDescriptor.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
						samplerDescriptor.MipLODBias = 0;
						samplerDescriptor.MaxAnisotropy = 0;
						samplerDescriptor.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
						samplerDescriptor.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
						samplerDescriptor.MinLOD = 0.0f;
						samplerDescriptor.MaxLOD = 0.0f;
						samplerDescriptor.ShaderRegister = 0;
						samplerDescriptor.RegisterSpace = 0;
						samplerDescriptor.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
						aSamplerDescriptor.push_back(samplerDescriptor);
						aShaderVisibilities.push_back(D3D12_SHADER_VISIBILITY_ALL);

						samplerDescriptor.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
						samplerDescriptor.ShaderRegister = 1;
						aSamplerDescriptor.push_back(samplerDescriptor);
						aShaderVisibilities.push_back(D3D12_SHADER_VISIBILITY_ALL);

						samplerDescriptor.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
						samplerDescriptor.ShaderRegister = 2;
						aSamplerDescriptor.push_back(samplerDescriptor);
						aShaderVisibilities.push_back(D3D12_SHADER_VISIBILITY_ALL);
					}
					else
					{
						aShaderVisibilities.push_back(D3D12_SHADER_VISIBILITY_ALL);
					}
				}
				
				addShaderRange(
					aTotalRanges, 
					range, 
					shaderResourceInfo.mDesc.miCount,
					shaderResourceInfo.mName,
					iRegisterSpace);
			}

			// root parameters given the ranges
			std::vector<D3D12_ROOT_PARAMETER> aRootParameters;
			std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources = *desc.mpaShaderResources;
			for(uint32_t i = 0; i < static_cast<uint32_t>(aTotalRanges.size()); i++)
			{
				D3D12_ROOT_PARAMETER rootParameter = {};
				rootParameter.ShaderVisibility = aShaderVisibilities[i];

#if defined(DEBUG_PRINT_OUT)
				std::string parameterType = "";
#endif // DEBUG_PRINT_OUT
				
				if(aTotalRanges[i].RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SRV)
				{
					rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
					
					bool bIsFiller = (aShaderResources[i].mName.find("fillerShaderResource") != std::string::npos || aShaderResources[i].mName.find("fillerUnorderedAccessResource") != std::string::npos);
					ShaderResourceType shaderResourceType = (bIsFiller == true) ? ShaderResourceType::RESOURCE_TYPE_BUFFER_IN : aShaderResources[i].mType;

#if defined(DEBUG_PRINT_OUT)
					std::ostringstream oss;
					oss << "SRV t(" << aTotalRanges[i].BaseShaderRegister << ")";
					parameterType = oss.str();
#endif // DEBUG_PRINT_OUT

					if(desc.mPipelineType == PipelineType::GRAPHICS_PIPELINE_TYPE &&
						shaderResourceType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
					{
						// descriptor table for textures
						initRootParameterAsDescriptorTable(rootParameter, &aTotalRanges[i], 1, aShaderVisibilities[i]);

#if defined(DEBUG_PRINT_OUT)
						parameterType += "-DescriptorTable";
#endif // DEBUG_PRINT_OUT
					}
					else if(desc.mPipelineType == PipelineType::COMPUTE_PIPELINE_TYPE &&
						shaderResourceType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
					{
						// descriptor table for textures
						initRootParameterAsDescriptorTable(rootParameter, &aTotalRanges[i], 1, aShaderVisibilities[i]);

#if defined(DEBUG_PRINT_OUT)
						parameterType += "-DescriptorTable";
#endif // DEBUG_PRINT_OUT
					}
					else
					{
						rootParameter.Descriptor.ShaderRegister = aTotalRanges[i].BaseShaderRegister;
						rootParameter.Descriptor.RegisterSpace = aTotalRanges[i].RegisterSpace;
					}
				}
				else if(aTotalRanges[i].RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_UAV)
				{
					rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
					
#if defined(DEBUG_PRINT_OUT)
					std::ostringstream oss;
					oss << "UAV u(" << aTotalRanges[i].BaseShaderRegister << ")";
					parameterType = oss.str();
#endif // DEBUG_PRINT_OUT
					
					if(desc.mPipelineType == PipelineType::COMPUTE_PIPELINE_TYPE)
					{
						if(aShaderResources[i].mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN || 
						   aShaderResources[i].mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT || 
						   aShaderResources[i].mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT)
						{
							// descriptor table for textures
							initRootParameterAsDescriptorTable(rootParameter, &aTotalRanges[i], 1, aShaderVisibilities[i]);

#if defined(DEBUG_PRINT_OUT)
							parameterType += "-DescriptorTable";
#endif // DEBUG_PRINT_OUT
						}
						else
						{
							rootParameter.Descriptor.ShaderRegister = aTotalRanges[i].BaseShaderRegister;
							rootParameter.Descriptor.RegisterSpace = aTotalRanges[i].RegisterSpace;
						}
					}
					else
					{
						rootParameter.Descriptor.ShaderRegister = aTotalRanges[i].BaseShaderRegister;
						rootParameter.Descriptor.RegisterSpace = aTotalRanges[i].RegisterSpace;
					}
				}
				else if(aTotalRanges[i].RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
				{
					rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
					rootParameter.Constants.RegisterSpace = aTotalRanges[i].RegisterSpace;
					rootParameter.Constants.ShaderRegister = aTotalRanges[i].BaseShaderRegister;

#if defined(DEBUG_PRINT_OUT)
					std::ostringstream oss;
					oss << "CBV b(" << rootParameter.Descriptor.ShaderRegister << ")";
					parameterType = oss.str();
#endif // DEBUG_PRINT_OUT
				}
				
				aRootParameters.push_back(rootParameter);
				
#if defined(DEBUG_PRINT_OUT)
				if(rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
				{
					DEBUG_PRINTF("%s root parameter descriptor table %s visibility: %d register space %d shader register %d\n",
						parameterType.c_str(),
						aShaderResources[i].mName.c_str(),
						rootParameter.ShaderVisibility,
						rootParameter.DescriptorTable.pDescriptorRanges[0].RegisterSpace,
						rootParameter.DescriptorTable.pDescriptorRanges[0].BaseShaderRegister);
				}
				else
				{
					DEBUG_PRINTF("%s root parameter %s visibility: %d register space %d shader register %d\n",
						parameterType.c_str(),
						aShaderResources[i].mName.c_str(),
						rootParameter.ShaderVisibility,
						rootParameter.Descriptor.RegisterSpace,
						rootParameter.Descriptor.ShaderRegister);
				}
#endif // DEBUG_PRINT_OUT
			}

			// set root constants to the end of the list
			if(desc.miNumRootConstants > 0)
			{
				int32_t iLastConstantRegister = -1;
				for(int32_t iRootParameter = 0; iRootParameter < static_cast<int32_t>(aRootParameters.size()); iRootParameter++)
				{
					auto const& rootParameter = aRootParameters[iRootParameter];
					if(rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV)
					{
						iLastConstantRegister = iRootParameter;
					}
				}
				
				D3D12_ROOT_PARAMETER rootParameter = {};
				rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
				rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
				rootParameter.Constants.Num32BitValues = desc.miNumRootConstants;
				rootParameter.Constants.RegisterSpace = 0;
				rootParameter.Constants.ShaderRegister = iLastConstantRegister + 1;
				aRootParameters.push_back(rootParameter);
			}

			// root signature descriptor
			D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
			if(desc.mPipelineType == PipelineType::GRAPHICS_PIPELINE_TYPE)
			{
				rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			}

			initRootSignatureDesc(
				rootSignatureDesc,
				aRootParameters.data(),
				static_cast<uint32_t>(aRootParameters.size()),
				aSamplerDescriptor.data(),
				static_cast<uint32_t>(aSamplerDescriptor.size()),
				rootSignatureFlags);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;

			// create root signature
			HRESULT hr = D3D12SerializeRootSignature(
				&rootSignatureDesc,
				D3D_ROOT_SIGNATURE_VERSION_1_0,
				&signature,
				&error);
			assert(SUCCEEDED(hr));

			hr = nativeDevice->CreateRootSignature(
				0,
				signature->GetBufferPointer(),
				signature->GetBufferSize(),
				IID_PPV_ARGS(&mRootSignature));
			assert(SUCCEEDED(hr));

            return mHandle;
        }

		/*
		**
		*/
		void CDescriptorSet::addShaderRange(
			std::vector<D3D12_DESCRIPTOR_RANGE>& aRanges,
			D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType,
			uint32_t iNumDescriptors,
			std::string const& name,
			uint32_t iRegisterSpace)
		{
			uint32_t iNumRegisters = 0;
			uint32_t iLastBaseRegister = 0;
			uint32_t iRangeIndex = UINT32_MAX;
			for(uint32_t i = 0; i < aRanges.size(); i++)
			{
				if(aRanges[i].RangeType == descriptorRangeType)
				{
					if(aRanges[i].BaseShaderRegister > iLastBaseRegister)
					{
						iLastBaseRegister = aRanges[i].BaseShaderRegister;
					}

					iRangeIndex = i;
					++iNumRegisters;
				}
			}

			D3D12_DESCRIPTOR_RANGE descriptorRange = {};
			size_t iLastIndex = aRanges.size();
			aRanges.push_back(descriptorRange);

			if(iRangeIndex == UINT32_MAX)
			{
				initDescriptorRange(
					aRanges[iLastIndex], 
					descriptorRangeType, 
					iNumDescriptors, 
					0,
					iRegisterSpace);
			}
			else
			{
				initDescriptorRange(
					aRanges[iLastIndex], 
					descriptorRangeType, 
					iNumDescriptors,
					iLastBaseRegister + 1,
					iRegisterSpace);
			}
		}

		/*
		**
		*/
		void CDescriptorSet::initDescriptorRange(
			D3D12_DESCRIPTOR_RANGE& descriptorRange,
			D3D12_DESCRIPTOR_RANGE_TYPE type,
			uint32_t iNumDescriptors,
			uint32_t iBaseRegister,
			uint32_t iRegisterSpace)
		{
			descriptorRange.RegisterSpace = iRegisterSpace;
			descriptorRange.NumDescriptors = iNumDescriptors;
			descriptorRange.BaseShaderRegister = iBaseRegister;
			descriptorRange.RangeType = type;
			descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		}

		/*
		**
		*/
		void CDescriptorSet::initRootParameterAsDescriptorTable(
			D3D12_ROOT_PARAMETER& rootParameter,
			D3D12_DESCRIPTOR_RANGE const* aDescriptorRanges,
			uint32_t iNumDescriptorRanges,
			D3D12_SHADER_VISIBILITY shaderVisibility)
		{
			rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameter.ShaderVisibility = shaderVisibility;
			rootParameter.DescriptorTable.NumDescriptorRanges = iNumDescriptorRanges;
			rootParameter.DescriptorTable.pDescriptorRanges = aDescriptorRanges;
		}

		/*
		**
		*/
		void CDescriptorSet::initRootSignatureDesc(
			D3D12_ROOT_SIGNATURE_DESC& rootSignatureDesc,
			D3D12_ROOT_PARAMETER const* pRootParameter,
			uint32_t iNumRootParameters,
			D3D12_STATIC_SAMPLER_DESC const* pSamplerDesc,
			uint32_t iNumSamplerDescs,
			D3D12_ROOT_SIGNATURE_FLAGS flags)
		{
			rootSignatureDesc.pParameters = pRootParameter;
			rootSignatureDesc.NumParameters = iNumRootParameters;
			rootSignatureDesc.pStaticSamplers = pSamplerDesc;
			rootSignatureDesc.NumStaticSamplers = iNumSamplerDescs;
			rootSignatureDesc.Flags = flags;
		}

		/*
		**
		*/
		void CDescriptorSet::setID(std::string const& id)
		{
			RenderDriver::Common::CObject::setID(id);
			std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
			mRootSignature->SetName(wName.c_str());
		}

		/*
		**
		*/
		void* CDescriptorSet::getNativeDescriptorSet()
		{
			return mRootSignature.Get();
		}

    }   // DX12

}   // RenderDriver