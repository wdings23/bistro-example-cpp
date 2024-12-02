#include <render-driver/DX12/ImageViewDX12.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/BufferDX12.h>
#include <render-driver/DX12/ImageDX12.h>
#include <render-driver/DX12/DescriptorHeapDX12.h>

#include <LogPrint.h>
#include <wtfassert.h>

namespace RenderDriver
{
    namespace DX12
    {
        PLATFORM_OBJECT_HANDLE CImageView::create(
            RenderDriver::Common::ImageViewDescriptor const& desc,
			RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CImageView::create(desc, device);

			RenderDriver::DX12::CImage* pDX12Image = nullptr; 
			if(desc.mpImage)
			{
				pDX12Image = static_cast<RenderDriver::DX12::CImage*>(desc.mpImage);
				assert(pDX12Image);
			}

			RenderDriver::DX12::CBuffer* pDX12Buffer = nullptr;
			if(desc.mpBuffer)
			{
				pDX12Buffer = static_cast<RenderDriver::DX12::CBuffer*>(desc.mpBuffer);
				assert(pDX12Buffer);
			}

			RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
			ID3D12Device* pDeviceDX12 = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());
			RenderDriver::DX12::CDescriptorHeap* pDX12DescriptorHeap = static_cast<RenderDriver::DX12::CDescriptorHeap*>(desc.mpDescriptorHeap);
			assert(pDX12DescriptorHeap);

			ID3D12DescriptorHeap* nativeDescriptorHeap = static_cast<ID3D12DescriptorHeap*>(pDX12DescriptorHeap->getNativeDescriptorHeap());

			RenderDriver::Common::DescriptorHeapType descriptorHeapType = pDX12DescriptorHeap->getType();
			D3D12_DESCRIPTOR_HEAP_TYPE d3d12DescriptorHeapType = SerializeUtils::DX12::convert(descriptorHeapType);

			D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = nativeDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			uint32_t iDescriptorIncrementSize = pDeviceDX12->GetDescriptorHandleIncrementSize(d3d12DescriptorHeapType);
			assert(desc.miShaderResourceIndex < pDX12DescriptorHeap->getDescriptor().miNumDescriptors);
			cpuHandle.ptr += (static_cast<uint64_t>(desc.miShaderResourceIndex) * iDescriptorIncrementSize);

			if(mDesc.mViewType == RenderDriver::Common::ResourceViewType::RenderTargetView)
			{
DEBUG_PRINTF("\ttype: Render Target View\n");

				if(desc.mFormat == RenderDriver::Common::Format::D16_UNORM ||
					desc.mFormat == RenderDriver::Common::Format::D24_UNORM_S8_UINT ||
					desc.mFormat == RenderDriver::Common::Format::D32_FLOAT ||
					desc.mFormat == RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT)
				{
					D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
					depthStencilViewDesc.Texture2D.MipSlice = 0;
					depthStencilViewDesc.Format = SerializeUtils::DX12::convert(desc.mFormat);
					
					RenderDriver::Common::DepthStencilDimension depthStencilDimension = RenderDriver::Common::DepthStencilDimension::Unknown;
					
					switch(desc.mDimension)
					{
					case RenderDriver::Common::Dimension::Texture1D:
						depthStencilDimension = RenderDriver::Common::DepthStencilDimension::Texture1D;
						break;
					
					case RenderDriver::Common::Dimension::Texture2D:
						depthStencilDimension = RenderDriver::Common::DepthStencilDimension::Texture2D;
						break;
					}
					depthStencilViewDesc.ViewDimension = SerializeUtils::DX12::convert(depthStencilDimension);

					if(pDX12Image)
					{
						pDeviceDX12->CreateDepthStencilView(
							static_cast<ID3D12Resource*>(pDX12Image->getNativeImage()),
							&depthStencilViewDesc,
							cpuHandle);
					}
					else if(pDX12Buffer)
					{
						pDeviceDX12->CreateDepthStencilView(
							static_cast<ID3D12Resource*>(pDX12Buffer->getNativeBuffer()),
							&depthStencilViewDesc,
							cpuHandle);
					}
					else
					{
						WTFASSERT(0, "image and buffer are null");
					}
				}
				else
				{
					D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc = {};
					renderTargetViewDesc.Texture2D.MipSlice = 0;
					renderTargetViewDesc.Texture2D.PlaneSlice = 0;
					renderTargetViewDesc.Format = SerializeUtils::DX12::convert(desc.mFormat);
					renderTargetViewDesc.ViewDimension = SerializeUtils::DX12::convert(desc.mDimension);

					if(pDX12Image)
					{
						pDeviceDX12->CreateRenderTargetView(
							static_cast<ID3D12Resource*>(pDX12Image->getNativeImage()),
							&renderTargetViewDesc,
							cpuHandle);
					}
					else if(pDX12Buffer)
					{
						pDeviceDX12->CreateRenderTargetView(
							static_cast<ID3D12Resource*>(pDX12Buffer->getNativeBuffer()),
							&renderTargetViewDesc,
							cpuHandle);
					}
					else
					{
						WTFASSERT(0, "image and buffer are null");
					}
				}
			}
			else if(mDesc.mViewType == RenderDriver::Common::ResourceViewType::ShaderResourceView)
			{
DEBUG_PRINTF("\ttype: Shader Resource View cpu handle: 0x%llX\n", cpuHandle.ptr);
				D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
				if(pDX12Image)
				{
					shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					shaderResourceViewDesc.Format = SerializeUtils::DX12::convert(desc.mFormat);
					shaderResourceViewDesc.ViewDimension = SerializeUtils::DX12::convert(desc.mShaderResourceViewDimension);
					shaderResourceViewDesc.Texture2DArray.ArraySize = desc.miNumImages;
					shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
					shaderResourceViewDesc.Texture2DArray.MipLevels = 1;
					shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
					shaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
					shaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0;

					pDeviceDX12->CreateShaderResourceView(
						static_cast<ID3D12Resource*>(pDX12Image->getNativeImage()),
						&shaderResourceViewDesc,
						cpuHandle);
				}
				else if(pDX12Buffer)
				{
					RenderDriver::Common::BufferDescriptor const& bufferDesc = pDX12Buffer->getDescriptor();

					shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
					shaderResourceViewDesc.Format = DXGI_FORMAT_UNKNOWN;
					shaderResourceViewDesc.ViewDimension = SerializeUtils::DX12::convert(RenderDriver::Common::ShaderResourceViewDimension::Buffer);
					shaderResourceViewDesc.Buffer.FirstElement = 0;
					shaderResourceViewDesc.Buffer.NumElements = static_cast<uint32_t>(bufferDesc.miSize) / (SerializeUtils::Common::getBaseComponentSize(bufferDesc.mImageViewFormat) * 4);
					shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
					shaderResourceViewDesc.Buffer.StructureByteStride = SerializeUtils::Common::getBaseComponentSize(bufferDesc.mImageViewFormat) * 4;

					pDeviceDX12->CreateShaderResourceView(
						static_cast<ID3D12Resource*>(pDX12Buffer->getNativeBuffer()),
						&shaderResourceViewDesc,
						cpuHandle);
				}
				else
				{
					WTFASSERT(0, "image and buffer are null");
				}
			}
			else
			{
DEBUG_PRINTF("\ttype: Unordered Access View\n");
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

				uavDesc.Format = SerializeUtils::DX12::convert(mDesc.mFormat);
				//uavDesc.Buffer.FirstElement = 0;
				//uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				//uavDesc.Buffer.NumElements = (UINT)mDesc.miTotalSize / mDesc.miElementSize;
				//assert(mDesc.miTotalSize % mDesc.miElementSize == 0);
				//uavDesc.Buffer.StructureByteStride = mDesc.miElementSize;
				//uavDesc.Buffer.CounterOffsetInBytes = 0;
				
				// uav view dimension needs to be buffer
				//uavDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_TEXTURE2D;
				//uavDesc.Format = DXGI_FORMAT_UNKNOWN;

				if(pDX12Image)
				{
					uavDesc.Texture2D.MipSlice = 0;

					pDeviceDX12->CreateUnorderedAccessView(
						static_cast<ID3D12Resource*>(pDX12Image->getNativeImage()),
						nullptr,
						&uavDesc,
						cpuHandle);
				}
				else if(pDX12Buffer)
				{
					pDeviceDX12->CreateUnorderedAccessView(
						static_cast<ID3D12Resource*>(pDX12Buffer->getNativeBuffer()),
						nullptr,
						&uavDesc,
						cpuHandle);
				}
				else
				{
					WTFASSERT(0, "image and buffer are null");
				}
			}

			return mHandle;
        }

    }   // Common

}   // RenderDriver