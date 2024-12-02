#include <render-driver/DX12/PipelineStateDX12.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/DescriptorSetDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

#include <serialize_utils.h>

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPipelineState::create(
            RenderDriver::Common::GraphicsPipelineStateDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CPipelineState::create(desc, device);

			RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
			ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

			D3D12_RASTERIZER_DESC rasterDesc = {};
			rasterDesc.FillMode = SerializeUtils::DX12::convert(desc.mRasterState.mFillMode);
			rasterDesc.CullMode = SerializeUtils::DX12::convert(desc.mRasterState.mCullMode);
			rasterDesc.FrontCounterClockwise = desc.mRasterState.mbFrontCounterClockwise;
			rasterDesc.DepthBias = desc.mRasterState.miDepthBias;
			rasterDesc.DepthBiasClamp = desc.mRasterState.mfDepthBiasClamp;
			rasterDesc.SlopeScaledDepthBias = desc.mRasterState.mfSlopeScaledDepthBias;
			rasterDesc.DepthClipEnable = desc.mRasterState.mbDepthClipEnable;
			rasterDesc.MultisampleEnable = desc.mRasterState.mbMultisampleEnable;
			rasterDesc.AntialiasedLineEnable = desc.mRasterState.mbAntialiasedLineEnable;
			rasterDesc.ForcedSampleCount = desc.mRasterState.miForcedSampleCount;
			rasterDesc.ConservativeRaster = SerializeUtils::DX12::convert(desc.mRasterState.mConservativeRaster);

			// blend states for all the render targets
			D3D12_BLEND_DESC blendDesc = {};
			blendDesc.IndependentBlendEnable = true;
			uint32_t iBlendIndex = 0;
			for(uint32_t iRenderTarget = 0; iRenderTarget < desc.miNumRenderTarget; iRenderTarget++)
			{
				RenderDriver::Common::BlendState const& blendState = desc.maRenderTargetBlendStates[iRenderTarget];

				if((uint32_t const&)blendState == UINT32_MAX)
				{
					continue;
				}

				blendDesc.RenderTarget[iBlendIndex].BlendEnable = blendState.mbEnabled;
				blendDesc.RenderTarget[iBlendIndex].LogicOpEnable = blendState.mbLogicOpEnabled;
				blendDesc.RenderTarget[iBlendIndex].SrcBlend = SerializeUtils::DX12::convert(blendState.mSrcColor);
				blendDesc.RenderTarget[iBlendIndex].DestBlend = SerializeUtils::DX12::convert(blendState.mDestColor);
				blendDesc.RenderTarget[iBlendIndex].BlendOp = SerializeUtils::DX12::convert(blendState.mColorOp);
				blendDesc.RenderTarget[iBlendIndex].SrcBlendAlpha = SerializeUtils::DX12::convert(blendState.mSrcAlpha);
				blendDesc.RenderTarget[iBlendIndex].DestBlendAlpha = SerializeUtils::DX12::convert(blendState.mDestAlpha);
				blendDesc.RenderTarget[iBlendIndex].BlendOpAlpha = SerializeUtils::DX12::convert(blendState.mAlphaOp);
				blendDesc.RenderTarget[iBlendIndex].LogicOp = SerializeUtils::DX12::convert(blendState.mLogicOp);
				blendDesc.RenderTarget[iBlendIndex].RenderTargetWriteMask = SerializeUtils::DX12::convert(blendState.mWriteMask);

				++iBlendIndex;
			}

			// depth stencil
			D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
			depthStencilDesc.DepthEnable = desc.mDepthStencilState.mbDepthEnabled;
			depthStencilDesc.DepthWriteMask = SerializeUtils::DX12::convert(desc.mDepthStencilState.mDepthWriteMask);
			depthStencilDesc.DepthFunc = SerializeUtils::DX12::convert(desc.mDepthStencilState.mDepthFunc);
			depthStencilDesc.StencilEnable = desc.mDepthStencilState.mbStencilEnabled;
			depthStencilDesc.StencilReadMask = desc.mDepthStencilState.miStencilReadMask;
			depthStencilDesc.StencilWriteMask = desc.mDepthStencilState.miStencilWriteMask;
			depthStencilDesc.FrontFace = SerializeUtils::DX12::convert(desc.mDepthStencilState.mFrontFace);
			depthStencilDesc.BackFace = SerializeUtils::DX12::convert(desc.mDepthStencilState.mBackFace);

			std::vector<D3D12_INPUT_ELEMENT_DESC> aInputElementDescs;
			uint32_t iOffset = 0;
			for(uint32_t iMember = 0; iMember < desc.miNumVertexMembers; iMember++)
			{
				D3D12_INPUT_ELEMENT_DESC inputElementDesc = SerializeUtils::DX12::convert(desc.maVertexFormats[iMember], iOffset);
				aInputElementDescs.push_back(inputElementDesc);
				iOffset += SerializeUtils::DX12::getBaseComponentSize(inputElementDesc.Format) * sizeof(float);
			}

			RenderDriver::DX12::CDescriptorSet* pDX12DescriptorSet = static_cast<RenderDriver::DX12::CDescriptorSet*>(desc.mpDescriptor);
			ID3D12RootSignature* rootSignature = static_cast<ID3D12RootSignature*>(pDX12DescriptorSet->getNativeDescriptorSet());

			// create pipeline
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {};
			pipelineStateDesc.InputLayout = { aInputElementDescs.data(), desc.miNumVertexMembers };
			pipelineStateDesc.pRootSignature = rootSignature;
			pipelineStateDesc.VS.pShaderBytecode = desc.mpVertexShader;
			pipelineStateDesc.VS.BytecodeLength = desc.miVertexShaderSize;
			pipelineStateDesc.PS.pShaderBytecode = desc.mpPixelShader;
			pipelineStateDesc.PS.BytecodeLength = desc.miPixelShaderSize;
			pipelineStateDesc.RasterizerState = rasterDesc;
			pipelineStateDesc.BlendState = blendDesc;
			pipelineStateDesc.DepthStencilState = depthStencilDesc;
			pipelineStateDesc.SampleMask = UINT_MAX;
			pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			pipelineStateDesc.NumRenderTargets = desc.miNumRenderTarget;
			for(uint32_t i = 0; i < desc.miNumRenderTarget; i++)
			{
				pipelineStateDesc.RTVFormats[i] = SerializeUtils::DX12::convert(desc.maRenderTargetFormats[i]);
			}
			pipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			pipelineStateDesc.SampleDesc.Count = 1;

			HRESULT hr = nativeDevice->CreateGraphicsPipelineState(
				&pipelineStateDesc,
				IID_PPV_ARGS(&mNativePipelineState));
			assert(SUCCEEDED(hr));

            return mHandle;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CPipelineState::create(
            RenderDriver::Common::ComputePipelineStateDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CPipelineState::create(desc, device);

			RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
			ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

			RenderDriver::DX12::CDescriptorSet* pDX12DescriptorSet = static_cast<RenderDriver::DX12::CDescriptorSet*>(desc.mpDescriptor);
			ID3D12RootSignature* rootSignature = static_cast<ID3D12RootSignature*>(pDX12DescriptorSet->getNativeDescriptorSet());

			D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc = {};
			pipelineStateDesc.pRootSignature = rootSignature;
			pipelineStateDesc.CS.pShaderBytecode = desc.mpComputeShader;
			pipelineStateDesc.CS.BytecodeLength = desc.miComputeShaderSize;

			HRESULT hr = nativeDevice->CreateComputePipelineState(
				&pipelineStateDesc,
				IID_PPV_ARGS(&mNativePipelineState));
			assert(SUCCEEDED(hr));

            return mHandle;
        }

		/*
		**
		*/
		void CPipelineState::setID(std::string const& id)
		{
			RenderDriver::Common::CObject::setID(id);
			std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
			mNativePipelineState->SetName(wName.c_str());
		}

		/*
		**
		*/
		void* CPipelineState::getNativePipelineState()
		{
			return mNativePipelineState.Get();
		}

    }   // DX12

} // RenderDriver