#include <render/DX12/RenderJobSerializerDX12.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/ImageDX12.h>
#include <render-driver/DX12/ImageViewDX12.h>
#include <render-driver/DX12/BufferDX12.h>
#include <render-driver/DX12/BufferViewDX12.h>
#include <render-driver/DX12/PipelineStateDX12.h>
#include <render-driver/DX12/UtilsDX12.h>
#include <render-driver/DX12/FenceDX12.h>

#include <LogPrint.h>
#include <wtfassert.h>

#include "image_utils.h"

#include <sstream>

namespace Render
{
    namespace DX12
    {
		/*
		**
		*/
		void Serializer::initCommandBuffer(
            PLATFORM_OBJECT_HANDLE& commandListHandle,
            PLATFORM_OBJECT_HANDLE* aCommandAllocatorHandles,
			PLATFORM_OBJECT_HANDLE* aFenceHandles,
            PLATFORM_OBJECT_HANDLE pipelineStateHandle,
            PipelineType const& type,
            std::string const& renderJobName,
            RenderDriver::Common::CDevice& device)
        {
			RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
			ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

			D3D12_COMMAND_LIST_TYPE listType = D3D12_COMMAND_LIST_TYPE_DIRECT;
			if(type == PipelineType::COMPUTE_PIPELINE_TYPE)
			{
				listType = D3D12_COMMAND_LIST_TYPE_COMPUTE;
			}
			else if(type == PipelineType::COPY_PIPELINE_TYPE)
			{
				listType = D3D12_COMMAND_LIST_TYPE_COPY;
			}
			
			for(uint32_t iTripleBufferIndex = 0; iTripleBufferIndex < NUM_BUFFER_OBJECTS; iTripleBufferIndex++)
			{
				RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc;
				commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
				if(type == PipelineType::COMPUTE_PIPELINE_TYPE)
				{
					commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
				}
				else if(type == PipelineType::COPY_PIPELINE_TYPE)
				{
					commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
				}

				std::string allocatorName = renderJobName + " Command Allocator";
				std::unique_ptr<RenderDriver::DX12::CCommandAllocator> pCommandAllocator = std::make_unique<RenderDriver::DX12::CCommandAllocator>();
				aCommandAllocatorHandles[iTripleBufferIndex] = pCommandAllocator->create(commandAllocatorDesc, dx12Device);
				PLATFORM_OBJECT_HANDLE commandAllocatorHandle = aCommandAllocatorHandles[iTripleBufferIndex];
				mapCommandAllocators[commandAllocatorHandle] = std::move(pCommandAllocator);
				mapCommandAllocators[commandAllocatorHandle]->setID(allocatorName);
			}

			RenderDriver::Common::CommandBufferDescriptor commandBufferDesc;
			commandBufferDesc.mpCommandAllocator = mapCommandAllocators[aCommandAllocatorHandles[0]].get();
			if(type == PipelineType::COMPUTE_PIPELINE_TYPE)
			{
				commandBufferDesc.mpPipelineState = mapComputePipelineStates[pipelineStateHandle].get();
				commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Compute;
			}
			else if(type == PipelineType::COPY_PIPELINE_TYPE)
			{
				commandBufferDesc.mpPipelineState = nullptr;
				commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
			}
			else
			{
				commandBufferDesc.mpPipelineState = mapGraphicsPipelineStates[pipelineStateHandle].get();
				commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Graphics;
			}
			
			std::unique_ptr<RenderDriver::DX12::CCommandBuffer> pCommandBuffer = std::make_unique<RenderDriver::DX12::CCommandBuffer>();
			commandListHandle = pCommandBuffer->create(commandBufferDesc, dx12Device);
			std::string commandBufferName = renderJobName + " Command List";
			pCommandBuffer->setID(commandBufferName);
			mapCommandBuffers[commandListHandle] = std::move(pCommandBuffer);
			//mapCommandBuffers[commandListHandle]->setID(commandBufferName);

			//for(uint32_t iFence = 0; iFence < 3; iFence++)
			//{
			//	RenderDriver::Common::FenceDescriptor fenceDesc;
			//	std::unique_ptr<RenderDriver::DX12::CFence> pFence = std::make_unique<RenderDriver::DX12::CFence>();
			//	aFenceHandles[iFence] = pFence->create(fenceDesc, dx12Device);
			//	std::string fenceName = renderJobName + " Fence";
			//	mapFences[aFenceHandles[iFence]] = std::move(pFence);
			//	mapFences[aFenceHandles[iFence]]->setID(fenceName);
			//}
        }

		/*
		**
		*/
		RenderDriver::Common::CDescriptorHeap* Serializer::registerObject(
			std::unique_ptr<RenderDriver::DX12::CDescriptorHeap>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapDescriptorHeaps[handle] = std::move(pObject);
			return mapDescriptorHeaps[handle].get();
		}

		/*
		**
		*/
		RenderDriver::Common::CRenderTarget* Serializer::registerObject(
			std::unique_ptr<RenderDriver::DX12::CRenderTarget>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapRenderTargets[handle] = std::move(pObject);
			return mapRenderTargets[handle].get();
		}

		/*
		**
		*/
		RenderDriver::Common::CCommandAllocator* Serializer::registerObject(
			std::unique_ptr<RenderDriver::DX12::CCommandAllocator>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapCommandAllocators[handle] = std::move(pObject);
			return mapCommandAllocators[handle].get();
		}

		/*
		**
		*/
		RenderDriver::Common::CCommandBuffer* Serializer::registerObject(
			std::unique_ptr<RenderDriver::DX12::CCommandBuffer>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapCommandBuffers[handle] = std::move(pObject);
			return mapCommandBuffers[handle].get();
		}

		/*
		**
		*/
		RenderDriver::Common::CFence* Serializer::registerObject(
			std::unique_ptr<RenderDriver::DX12::CFence>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapFences[handle] = std::move(pObject);
			return mapFences[handle].get();
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::serializeShaderDescriptors(
			std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResourceInfo,
			PipelineType pipelineType,
			uint32_t iNumRootConstants,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::DX12::CDescriptorSet descriptorSet;
			RenderDriver::Common::DescriptorSetDescriptor desc;
			desc.mpaShaderResources = &aShaderResourceInfo;
			desc.mPipelineType = pipelineType;
			desc.miNumRootConstants = iNumRootConstants;

			std::unique_ptr<RenderDriver::DX12::CDescriptorSet> pDescriptorSet = std::make_unique<RenderDriver::DX12::CDescriptorSet>();
			PLATFORM_OBJECT_HANDLE handle = pDescriptorSet->create(desc, device);
			mapDescriptorSets[handle] = std::move(pDescriptorSet);

			return handle;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::serializeComputePipeline(
			PLATFORM_OBJECT_HANDLE descriptorHandle,
			ShaderInfo const& shaderInfo,
			Render::Common::RenderJobInfo const& renderJob,
			RenderDriver::Common::CDevice& device)
		{
			ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(static_cast<RenderDriver::DX12::CDevice&>(device).getNativeDevice());

			FILE* fp = fopen(shaderInfo.mFileOutputPath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			std::vector<char> acShaderBuffer(iFileSize + 1);
			acShaderBuffer[iFileSize] = '\0';
			fread(acShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			std::unique_ptr<RenderDriver::DX12::CPipelineState> pPipelineState = std::make_unique<RenderDriver::DX12::CPipelineState>();
			RenderDriver::Common::ComputePipelineStateDescriptor pipelineStateDesc;
			pipelineStateDesc.miFlags = 0;
			pipelineStateDesc.miComputeShaderSize = (uint32_t)iFileSize;
			pipelineStateDesc.mpComputeShader = (uint8_t*)acShaderBuffer.data();
			pipelineStateDesc.mpDescriptor = mapDescriptorSets[descriptorHandle].get();
			PLATFORM_OBJECT_HANDLE handle = pPipelineState->create(pipelineStateDesc, device);
			mapComputePipelineStates[handle] = std::move(pPipelineState);

			return handle;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::serializeGraphicsPipeline(
			PLATFORM_OBJECT_HANDLE descriptorHandle,
			std::vector<RenderDriver::Common::BlendState> const& aBlendStates,
			RenderDriver::Common::DepthStencilState const& depthStencilState,
			RenderDriver::Common::RasterState const& rasterState,
			std::vector<RenderDriver::Common::Format> const& aRenderTargetFormats,
			rapidjson::Document const& doc,
			ShaderInfo const& vertexShaderInfo,
			ShaderInfo const& fragmentShaderInfo,
			Render::Common::RenderJobInfo const& renderJob,
			RenderDriver::Common::CDevice& device)
		{
			ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(static_cast<RenderDriver::DX12::CDevice&>(device).getNativeDevice());

			std::vector<RenderDriver::Common::VertexFormat> aInputFormat;
			SerializeUtils::Common::getVertexFormat(aInputFormat, doc);

			// vertex shader
			FILE* fp = fopen(vertexShaderInfo.mFileOutputPath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			std::vector<char> acShaderBufferVS(iFileSize);
			fread(acShaderBufferVS.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			// fragment shader
			fp = fopen(fragmentShaderInfo.mFileOutputPath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			std::vector<char> acShaderBufferPS(iFileSize);
			fread(acShaderBufferPS.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			DEBUG_PRINTF("\nvertex shader: %s\n", vertexShaderInfo.mFileOutputPath.c_str());
			DEBUG_PRINTF("fragment shader: %s\n\n", fragmentShaderInfo.mFileOutputPath.c_str());

			RenderDriver::Common::GraphicsPipelineStateDescriptor pipelineStateDesc;
			pipelineStateDesc.mpDescriptor = mapDescriptorSets[descriptorHandle].get();
			pipelineStateDesc.mRasterState = rasterState;
			pipelineStateDesc.mDepthStencilState = depthStencilState;
			pipelineStateDesc.maInputFormat = aInputFormat;
			
			for(uint32_t i = 0; i < aBlendStates.size(); i++)
			{
				pipelineStateDesc.maRenderTargetBlendStates[i] = aBlendStates[i];
				pipelineStateDesc.maRenderTargetFormats[i] = aRenderTargetFormats[i];
			}

			pipelineStateDesc.miNumRenderTarget = (uint32_t)aBlendStates.size();
			pipelineStateDesc.miNumVertexMembers = (uint32_t)aInputFormat.size();
			pipelineStateDesc.mSample.miCount = 1;
			pipelineStateDesc.mSample.miQuality = 0;
			pipelineStateDesc.mDepthStencilFormat = RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT;
			pipelineStateDesc.miFlags = 0;
			pipelineStateDesc.miPixelShaderSize = (uint32_t)acShaderBufferPS.size();
			pipelineStateDesc.mpPixelShader = (uint8_t*)acShaderBufferPS.data();
			pipelineStateDesc.miVertexShaderSize = (uint32_t)acShaderBufferVS.size();
			pipelineStateDesc.mpVertexShader = (uint8_t*)acShaderBufferVS.data();
			pipelineStateDesc.maVertexFormats = pipelineStateDesc.maInputFormat.data();

			std::unique_ptr<RenderDriver::DX12::CPipelineState> pPipelineState = std::make_unique<RenderDriver::DX12::CPipelineState>();
			PLATFORM_OBJECT_HANDLE handle = pPipelineState->create(pipelineStateDesc, device);
			mapGraphicsPipelineStates[handle] = std::move(pPipelineState);

			return handle;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::createResourceView(
			SerializeUtils::Common::ShaderResourceInfo& shaderResource,
			SerializeUtils::Common::ShaderResourceDataInfo& shaderData,
			uint32_t iDataSize,
			RenderDriver::Common::CDevice& device)
		{
			ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(static_cast<RenderDriver::DX12::CDevice&>(device).getNativeDevice());
			SerializeUtils::DX12::ShaderResourceInfo& dx12ShaderResource = reinterpret_cast<SerializeUtils::DX12::ShaderResourceInfo&>(shaderResource);
			D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = dx12ShaderResource.mCPUDescriptorHandle;

			ComPtr<ID3D12Resource> resource = mResourcesDX12[shaderData.mHandle];

			if(shaderResource.mViewType == RenderDriver::Common::ResourceViewType::ShaderResourceView)
			{
				nativeDevice->CreateShaderResourceView(
					resource.Get(),
					nullptr,
					cpuDescriptorHandle);
			}
			else if(shaderResource.mViewType == RenderDriver::Common::ResourceViewType::UnorderedAccessView)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				D3D12_UAV_DIMENSION uavDimension = D3D12_UAV_DIMENSION_UNKNOWN;
				if(shaderResource.mDesc.mDimension == RenderDriver::Common::Dimension::Buffer)
				{
					uavDimension = D3D12_UAV_DIMENSION_BUFFER;
				}
				else if(shaderResource.mDesc.mDimension == RenderDriver::Common::Dimension::Texture1D)
				{
					uavDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
				}
				else if(shaderResource.mDesc.mDimension == RenderDriver::Common::Dimension::Texture2D)
				{
					uavDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				}
				else if(shaderResource.mDesc.mDimension == RenderDriver::Common::Dimension::Texture3D)
				{
					uavDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				}

				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = uavDimension;
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				uavDesc.Buffer.NumElements = (UINT)shaderResource.mDesc.miWidth / shaderResource.miStructByteStride;
				assert(shaderResource.mDesc.miWidth % shaderResource.miStructByteStride == 0);
				uavDesc.Buffer.StructureByteStride = shaderResource.miStructByteStride;
				uavDesc.Buffer.CounterOffsetInBytes = 0;

				nativeDevice->CreateUnorderedAccessView(
					resource.Get(),
					nullptr,
					&uavDesc,
					cpuDescriptorHandle);
			}
			else if(shaderResource.mViewType == RenderDriver::Common::ResourceViewType::ConstantBufferView)
			{
				D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc;
				for(uint32_t iBuffer = 0; iBuffer < 3; iBuffer++)
				{
					shaderResource.maDataInfo[iBuffer].miDataSize = (uint32_t)shaderResource.mDesc.miWidth;
				}

				viewDesc.BufferLocation = resource->GetGPUVirtualAddress(); // parentResource->GetGPUVirtualAddress();
				viewDesc.SizeInBytes = UINT(iDataSize);

				nativeDevice->CreateConstantBufferView(
					&viewDesc,
					cpuDescriptorHandle);
			}

			return 0;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::createDescriptorHeap(
			uint32_t iNumShaderResources,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::DX12::CDescriptorHeap descriptorHeap;

			RenderDriver::Common::DescriptorHeapDescriptor desc;
			desc.mFlag = RenderDriver::Common::DescriptorHeapFlag::ShaderVisible;
			desc.mType = RenderDriver::Common::DescriptorHeapType::General;
			desc.miNumDescriptors = iNumShaderResources;
			//PLATFORM_OBJECT_HANDLE handle = descriptorHeap.create(desc, device);
			//mDescriptorHeaps[handle] = descriptorHeap;
			
			std::unique_ptr<RenderDriver::DX12::CDescriptorHeap> pDescriptorHeap = std::make_unique<RenderDriver::DX12::CDescriptorHeap>();
			PLATFORM_OBJECT_HANDLE handle = pDescriptorHeap->create(desc, device);
			mapDescriptorHeaps[handle] = std::move(pDescriptorHeap);

			return handle;
		}

		/*
		**
		*/
		void Serializer::createResourceViewOnly(
			PLATFORM_OBJECT_HANDLE descriptorHeapHandle,
			SerializeUtils::Common::ShaderResourceInfo& shaderResource,
			uint32_t iResourceIndex,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Common::CDescriptorHeap& dx12DescriptorHeap = *mapDescriptorHeaps[descriptorHeapHandle].get();
			SerializeUtils::DX12::ShaderResourceInfo& dx12ShaderResource = reinterpret_cast<SerializeUtils::DX12::ShaderResourceInfo&>(shaderResource);
			ID3D12Device* pNativeDevice = static_cast<ID3D12Device*>(static_cast<RenderDriver::DX12::CDevice&>(device).getNativeDevice());

			ID3D12DescriptorHeap* nativeDescriptorHeap = static_cast<ID3D12DescriptorHeap*>(dx12DescriptorHeap.getNativeDescriptorHeap());

			uint64_t iSerializeDescriptorHeapOffset = (uint64_t)(pNativeDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = nativeDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			cpuDescriptorHandle.ptr += SIZE_T((uint64_t)iResourceIndex * iSerializeDescriptorHeapOffset);

			bool bIsTexture = (dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN ||
				dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT ||
				dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT);

			bool bIsBuffer = (dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN ||
				dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT ||
				dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT);

			uint32_t iTotalSize = static_cast<uint32_t>(shaderResource.maDataInfo[0].miDataSize);

			// buffer view
			if(bIsBuffer)
			{
				RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
				{
					/* .mFormat				*/  dx12ShaderResource.mDesc.mViewFormat,
					/* .mpBuffer			*/  dx12ShaderResource.mExternalResource.mpBuffer,
					/* .mpCounterBuffer		*/  nullptr,
					/* .miSize				*/  (iTotalSize == 0) ? 1024 : iTotalSize,
					/* .miStructStrideSize	*/  dx12ShaderResource.miStructByteStride,
					/* .mFlags				*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
					/* .mViewType			*/  dx12ShaderResource.mViewType,
					/* .mpDescriptorHeap	*/  &dx12DescriptorHeap,
					/* .miDescriptorOffset	*/  iResourceIndex,
				};
				
				std::unique_ptr<RenderDriver::DX12::CBufferView> pBufferView = std::make_unique<RenderDriver::DX12::CBufferView>();
				PLATFORM_OBJECT_HANDLE bufferViewHandle = pBufferView->create(bufferViewDesc, device);
				mapBufferViews[bufferViewHandle] = std::move(pBufferView);
			}
			else if(bIsTexture)
			{
				RenderDriver::Common::ImageViewDescriptor imageViewDesc =
				{
					/* .mViewType				*/				dx12ShaderResource.mViewType,
					/* .mDimension				*/				dx12ShaderResource.mExternalResource.mDimension,
					/* .mShaderResourceViewDimension */			dx12ShaderResource.mExternalResource.mShaderResourceViewDimension,
					/* .mFormat					*/				dx12ShaderResource.mDesc.mFormat,
					/* .miTotalSize				*/				iTotalSize,
					/* .miElementSize			*/				dx12ShaderResource.miStructByteStride,
					/* .mpDescriptorHeap		*/				&dx12DescriptorHeap,
					/* .mpImage					*/				dx12ShaderResource.mExternalResource.mpImage,
					/* .mpBuffer				*/				nullptr,
					/* .miShaderResourceIndex	*/				iResourceIndex,
					/* .miNumImages				*/				dx12ShaderResource.mExternalResource.miNumImages,
				};
				RenderDriver::DX12::CImageView imageView;
				PLATFORM_OBJECT_HANDLE imageViewHandle = imageView.create(imageViewDesc, device);
			}
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::createResource(
			PLATFORM_OBJECT_HANDLE descriptorHeapHandle,
			SerializeUtils::Common::ShaderResourceInfo& shaderResource,
			uint32_t iResourceIndex,
			uint32_t iImageWidth,
			uint32_t iImageHeight,
			uint32_t iTripleBufferIndex,
			Render::Common::JobType jobType,
			RenderDriver::Common::CDevice& device)
		{
			std::ostringstream oss;
			oss << shaderResource.mName << iTripleBufferIndex;
			std::string const& shaderResourceName = oss.str();

			PLATFORM_OBJECT_HANDLE ret = 0;

			RenderDriver::Common::CDescriptorHeap& dx12DescriptorHeap = *mapDescriptorHeaps[descriptorHeapHandle].get();
			SerializeUtils::DX12::ShaderResourceInfo& dx12ShaderResource = reinterpret_cast<SerializeUtils::DX12::ShaderResourceInfo&>(shaderResource);
			ID3D12Device* pNativeDevice = static_cast<ID3D12Device*>(static_cast<RenderDriver::DX12::CDevice&>(device).getNativeDevice());

			ID3D12DescriptorHeap* nativeDescriptorHeap = static_cast<ID3D12DescriptorHeap*>(dx12DescriptorHeap.getNativeDescriptorHeap());

			uint64_t iSerializeDescriptorHeapOffset = (uint64_t)(pNativeDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
			D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = nativeDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			cpuDescriptorHandle.ptr += SIZE_T((uint64_t)iResourceIndex * iSerializeDescriptorHeapOffset);

			bool bIsTexture = (dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN ||
				dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT ||
				dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT);

			bool bIsBuffer = (dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN ||
				dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT ||
				dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT);

			dx12ShaderResource.mCPUDescriptorHandle = cpuDescriptorHandle;
			
			// image or render target, depeending on the parent render job index
			if(dx12ShaderResource.miParentRenderJobIndex == UINT32_MAX)
			{
				RenderDriver::Common::ImageDescriptor imageDesc = {};
				RenderDriver::Common::BufferDescriptor bufferDesc = {};

				++miCurrHandle;
				if(dx12ShaderResource.mImageFilePath.length() > 0)
				{
					std::vector<unsigned char> aImageBuffer;
					uint32_t iImageWidth = 0, iImageHeight = 0, iNumComp = 0;
					ImageUtils::getImageDimensions(
						iImageWidth,
						iImageHeight,
						iNumComp,
						dx12ShaderResource.mImageFilePath);

					if(dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN ||
						dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT)
					{
						dx12ShaderResource.mDesc.miWidth = iImageWidth;
						dx12ShaderResource.mDesc.miHeight = iImageHeight;
						dx12ShaderResource.mDesc.miDepthOrArraySize = 1;

						imageDesc.miWidth = iImageWidth;
						imageDesc.miHeight = iImageHeight;
					}
					else if(
						dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN ||
						dx12ShaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT)
					{
						dx12ShaderResource.mDesc.miWidth = (uint64_t)iImageWidth * (uint64_t)iImageHeight * (uint64_t)iNumComp;
						dx12ShaderResource.mDesc.miHeight = 1;
						dx12ShaderResource.mDesc.miDepthOrArraySize = 1;

						bufferDesc.miSize = iImageWidth * iImageHeight * iNumComp;
						
					}
				}
				else
				{
					if(bIsBuffer)
					{
						if(dx12ShaderResource.mDesc.miWidth == 0)
						{
							// screen size buffer
							dx12ShaderResource.mDesc.miWidth = (uint64_t)iImageWidth * (uint64_t)iImageHeight * 4;
							dx12ShaderResource.mDesc.miHeight = 1;
							dx12ShaderResource.mDesc.miDepthOrArraySize = 1;
							bufferDesc.miSize = iImageWidth * iImageHeight * 4;
						}
						else
						{
							bufferDesc.miSize = dx12ShaderResource.mDesc.miWidth;
						}
					}
					else
					{
						// texture needs to be created, only create writable texture for now
						// SRV or UAV root descriptors can only be Raw or Structured buffers. [ STATE_CREATION ERROR #690: CREATEGRAPHICSPIPELINESTATE_PS_ROOT_SIGNATURE_MISMATCH]
						// this usually happens for missing, mis-spelled, or non-matching shader resource name, ie. "shaderResource0" in HLSL file and "srg0" in json file 
						// - OR - ShaderResourceName entry for TextureInput in the pipeline JSON file is not given
						// - OR - wrong shader file name in the pipeline JSON file
						WTFASSERT(dx12ShaderResource.mViewType == RenderDriver::Common::ResourceViewType::UnorderedAccessView,
							"Invalid view, needs unordered access view for \"%s\" in file \"%s\", this usually means the shader resource name is spelled wrong or not given in the pipeline file",
							dx12ShaderResource.mName.c_str());

						// assume RGBA for now
						//dx12ShaderResource.mDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
						//
						//dx12ShaderResource.mDesc.miWidth = (uint64_t)iImageWidth * (uint64_t)iImageHeight * 1;
						//dx12ShaderResource.mDesc.miHeight = 1;
						//dx12ShaderResource.mDesc.miDepthOrArraySize = 1;
						//uint32_t iFlags = static_cast<uint32_t>(dx12ShaderResource.mDesc.mFlags);
						//iFlags |= static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess);
						//dx12ShaderResource.mDesc.mFlags = static_cast<RenderDriver::Common::ResourceFlagBits>(iFlags);
						//
						//bufferDesc.miSize = iImageWidth * iImageHeight * 1;
						//
						//bIsTexture = false;
						//bIsBuffer = true;

						dx12ShaderResource.mDesc.mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
						dx12ShaderResource.mDesc.miWidth = (uint64_t)iImageWidth;
						dx12ShaderResource.mDesc.miHeight = (uint64_t)iImageHeight;
						dx12ShaderResource.mDesc.miDepthOrArraySize = 1;
						uint32_t iFlags = static_cast<uint32_t>(dx12ShaderResource.mDesc.mFlags);
						iFlags |= static_cast<uint32_t>(RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess);
						dx12ShaderResource.mDesc.mFlags = static_cast<RenderDriver::Common::ResourceFlagBits>(iFlags);
					}
				}

				// needs to set the size first due to being dynamic
				assert(dx12ShaderResource.mDesc.miWidth != UINT32_MAX && dx12ShaderResource.mDesc.miWidth != 0);

				ID3D12Resource* pResource = nullptr;

				// create and register image or buffer
				if(bIsTexture)
				{
					// image 
					imageDesc.mFormat = dx12ShaderResource.mDesc.mFormat;
					imageDesc.mStateFlags = RenderDriver::Common::ResourceStateFlagBits::Common;
					if(dx12ShaderResource.mViewType == RenderDriver::Common::ResourceViewType::RenderTargetView)
					{
						imageDesc.mStateFlags = RenderDriver::Common::ResourceStateFlagBits::RenderTarget;
					}
					else if(dx12ShaderResource.mViewType == RenderDriver::Common::ResourceViewType::UnorderedAccessView)
					{
						// read/write image

						// needs to be created like a writable buffer
						//imageDesc.miWidth = (uint64_t)iImageWidth * (uint64_t)iImageHeight * 4;
						//imageDesc.miHeight = 1;
						imageDesc.miWidth = (uint64_t)iImageWidth;
						imageDesc.miHeight = (uint64_t)iImageHeight;
						imageDesc.mFormat = shaderResource.mDesc.mFormat;
						imageDesc.miNumImages = 1;
						imageDesc.mStateFlags = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;
						imageDesc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess;
					}
					else
					{
						imageDesc.miWidth = shaderResource.mDesc.miWidth;
						imageDesc.miHeight = shaderResource.mDesc.miHeight;
						imageDesc.mFormat = shaderResource.mDesc.mFormat;
						imageDesc.miNumImages = shaderResource.mDesc.miDepthOrArraySize;
					}

					// is texture array?
					RenderDriver::Common::ShaderResourceViewDimension shaderResourceViewDimension = RenderDriver::Common::ShaderResourceViewDimension::Texture2D;
					if(imageDesc.miNumImages > 1)
					{
						shaderResourceViewDimension = RenderDriver::Common::ShaderResourceViewDimension::Texture2DArray;
					}

					// create image
					float afDefaultClearColor[4] = { 0.0f, 0.0f, 0.3f, 0.0f };
					if(imageDesc.mafClearColor == nullptr)
					{
						imageDesc.mafClearColor = afDefaultClearColor;
					}

					std::unique_ptr<RenderDriver::DX12::CImage> pImage = std::make_unique<RenderDriver::DX12::CImage>();
					PLATFORM_OBJECT_HANDLE imageHandle = pImage->create(imageDesc, device);
					pImage->setID(shaderResourceName);
					
					// image view
					RenderDriver::DX12::CImageView imageView = {};
					RenderDriver::Common::ImageViewDescriptor imageViewDesc;
					imageViewDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
					imageViewDesc.mShaderResourceViewDimension = shaderResourceViewDimension;
					imageViewDesc.mFormat = dx12ShaderResource.mDesc.mFormat;
					imageViewDesc.miElementSize = dx12ShaderResource.miStructByteStride;
					imageViewDesc.miTotalSize = imageViewDesc.miElementSize * dx12ShaderResource.mDesc.miWidth * dx12ShaderResource.mDesc.miHeight;
					imageViewDesc.mpImage = pImage.get();
					imageViewDesc.mViewType = dx12ShaderResource.mViewType;
					imageViewDesc.mpDescriptorHeap = &dx12DescriptorHeap;
					imageViewDesc.miShaderResourceIndex = iResourceIndex;

					std::unique_ptr<RenderDriver::DX12::CImageView> pImageView = std::make_unique<RenderDriver::DX12::CImageView>();
					PLATFORM_OBJECT_HANDLE imageViewHandle = pImageView->create(imageViewDesc, device);
					mapImageViews[imageViewHandle] = std::move(pImageView);

					pResource = static_cast<ID3D12Resource*>(pImage->getNativeImage());

					shaderResource.maHandles[iTripleBufferIndex] = imageHandle;

					mapImages[imageHandle] = std::move(pImage);

					ret = imageHandle;
				}
				else if(bIsBuffer)
				{
					// buffer
					bufferDesc.mpCounterBuffer = reinterpret_cast<RenderDriver::DX12::CBuffer*>(dx12ShaderResource.mpCounterBuffer);
					bufferDesc.mFormat = dx12ShaderResource.mDesc.mFormat;
					bufferDesc.miSize = dx12ShaderResource.mDesc.miWidth;
					bufferDesc.mFlags = dx12ShaderResource.mDesc.mFlags;

					// enforce dx12 minimum requirement for constant buffer size
					uint32_t const DX12ConstantBufferSizeRequirement = 256;
					if(bufferDesc.miSize < DX12ConstantBufferSizeRequirement)
					{
						bufferDesc.miSize = DX12ConstantBufferSizeRequirement;
					}

					// want the initial to for copy
					if(dx12ShaderResource.maDataInfo[iTripleBufferIndex].mpData != nullptr)
					{
						bufferDesc.mOverrideState = RenderDriver::Common::ResourceStateFlagBits::CopyDestination;
					}

					std::unique_ptr<RenderDriver::DX12::CBuffer> pBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();
					PLATFORM_OBJECT_HANDLE bufferHandle = pBuffer->create(bufferDesc, device);
					pBuffer->setID(shaderResourceName);
					mapBuffers[bufferHandle] = std::move(pBuffer);

					// buffer view
					RenderDriver::DX12::CBufferView bufferView;
					RenderDriver::Common::BufferViewDescriptor bufferViewDesc;
					bufferViewDesc.mpBuffer = mapBuffers[bufferHandle].get();
					bufferViewDesc.mFlags = RenderDriver::Common::UnorderedAccessViewFlags::None;
					bufferViewDesc.miDescriptorOffset = iResourceIndex;
					bufferViewDesc.miSize = static_cast<uint32_t>(bufferDesc.miSize);
					bufferViewDesc.miStructStrideSize = dx12ShaderResource.miStructByteStride;
					bufferViewDesc.mpDescriptorHeap = &dx12DescriptorHeap;
					bufferViewDesc.mViewType = dx12ShaderResource.mViewType;
					bufferViewDesc.mFormat = dx12ShaderResource.mDesc.mViewFormat;
					bufferViewDesc.mpCounterBuffer = bufferDesc.mpCounterBuffer;

					std::unique_ptr<RenderDriver::DX12::CBufferView> pBufferView = std::make_unique<RenderDriver::DX12::CBufferView>();
					PLATFORM_OBJECT_HANDLE bufferViewHandle = pBufferView->create(bufferViewDesc, device);
					
					
					mapBufferViews[bufferViewHandle] = std::move(pBufferView);
					mResourcesDX12[bufferHandle] = static_cast<ID3D12Resource*>(mapBuffers[bufferHandle]->getNativeBuffer());

					//pResource = static_cast<ID3D12Resource*>(buffer.getNativeBuffer());
					pResource = mResourcesDX12[bufferHandle];

					shaderResource.maHandles[iTripleBufferIndex] = bufferHandle;

					ret = bufferHandle;
				}

				bool bCopyData = false;
				if(bIsBuffer)
				{
					bCopyData = true;
					dx12ShaderResource.maDataInfo[iTripleBufferIndex].miDataSize = dx12ShaderResource.mDesc.miWidth;
				}
				else if(bIsTexture)
				{
					//bCopyData = (((uint32_t)imageDesc.mStateFlags & (uint32_t)RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess) != 0);
					//if(bCopyData)
					//{
					//	dx12ShaderResource.maDataInfo[iTripleBufferIndex].miDataSize = dx12ShaderResource.mDesc.miWidth;
					//}
				}

				// copy data over if there's any
				if(bCopyData)
				{
#if 0
					HRESULT hr = pResource->Map(
						0,
						&range,
						&dx12ShaderResource.maDataInfo[iTripleBufferIndex].mpMappedData);
					assert(hr == S_OK);

					if(dx12ShaderResource.maDataInfo[iTripleBufferIndex].mpData != nullptr)
					{
						memcpy(
							dx12ShaderResource.maDataInfo[iTripleBufferIndex].mpMappedData,
							dx12ShaderResource.maDataInfo[iTripleBufferIndex].mpData,
							dx12ShaderResource.maDataInfo[iTripleBufferIndex].miDataSize);
					}
#endif // #if 0
					if(dx12ShaderResource.maDataInfo[iTripleBufferIndex].mpData != nullptr)
					{
						updateShaderResourceData(
							shaderResource,
							dx12ShaderResource.maDataInfo[iTripleBufferIndex].mpData, 
							iTripleBufferIndex, device);
					}

				}	// if copy data

				std::wstring name = RenderDriver::DX12::Utils::convertToLPTSTR(shaderResourceName);
				pResource->SetName(name.c_str());

				mCPUDescriptorsDX12[ret] = cpuDescriptorHandle;

			}	// if not linked to parent

			return ret;
		}

		/*
		**
		*/
		void Serializer::addShaderRange(
			std::vector<D3D12_DESCRIPTOR_RANGE>& aRanges,
			D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType,
			uint32_t iNumDescriptors,
			std::string const& name)
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
				initDescriptorRange(aRanges[iLastIndex], descriptorRangeType, 1, 0);
			}
			else
			{
				initDescriptorRange(aRanges[iLastIndex], descriptorRangeType, 1, iLastBaseRegister + 1);
			}
		}

		/*
		**
		*/
		void Serializer::initDescriptorRange(
			D3D12_DESCRIPTOR_RANGE& descriptorRange,
			D3D12_DESCRIPTOR_RANGE_TYPE type,
			uint32_t iNumDescriptors,
			uint32_t iBaseRegister)
		{
			descriptorRange.RegisterSpace = 0;
			descriptorRange.NumDescriptors = iNumDescriptors;
			descriptorRange.BaseShaderRegister = iBaseRegister;
			descriptorRange.RangeType = type;
			descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		}

		/*
		**
		*/
		void Serializer::initRootParameterAsDescriptorTable(
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
		void Serializer::initRootSignatureDesc(
			D3D12_VERSIONED_ROOT_SIGNATURE_DESC& rootSignatureDesc,
			D3D12_ROOT_PARAMETER const* pRootParameter,
			uint32_t iNumRootParameters,
			D3D12_STATIC_SAMPLER_DESC const* pSamplerDesc,
			uint32_t iNumSamplerDescs,
			D3D12_ROOT_SIGNATURE_FLAGS flags)
		{
			rootSignatureDesc.Desc_1_0.pParameters = pRootParameter;
			rootSignatureDesc.Desc_1_0.NumParameters = iNumRootParameters;
			rootSignatureDesc.Desc_1_0.pStaticSamplers = pSamplerDesc;
			rootSignatureDesc.Desc_1_0.NumStaticSamplers = iNumSamplerDescs;
			rootSignatureDesc.Desc_1_0.Flags = flags;
		}

		/*
		**
		*/
		void Serializer::updateShaderResourceData(
			SerializeUtils::Common::ShaderResourceInfo& shaderResource, 
			void const* pNewData, 
			uint32_t iTripleBufferIndex,
			RenderDriver::Common::CDevice& device)
		{
			SerializeUtils::DX12::ShaderResourceInfo& dx12ShaderResource = reinterpret_cast<SerializeUtils::DX12::ShaderResourceInfo&>(shaderResource);

			maUploadBuffers.resize(maUploadBuffers.size() + 1);
			RenderDriver::DX12::CBuffer& uploadBuffer = maUploadBuffers.back();
			RenderDriver::Common::BufferDescriptor uploadBufferDesc = {};
			uploadBufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
			uploadBufferDesc.miSize = dx12ShaderResource.maDataInfo[iTripleBufferIndex].miDataSize;
			uploadBuffer.create(uploadBufferDesc, device);

			std::ostringstream oss;
			oss << "UploadBuffer" << maUploadBuffers.size() - 1;
			uploadBuffer.setID(oss.str());

			uint8_t* pData = nullptr;
			HRESULT hr = static_cast<ID3D12Resource*>(uploadBuffer.getNativeBuffer())->Map(0, nullptr, reinterpret_cast<void**>(&pData));
			assert(SUCCEEDED(hr));
			memcpy(
				pData,
				dx12ShaderResource.maDataInfo[iTripleBufferIndex].mpData,
				dx12ShaderResource.maDataInfo[iTripleBufferIndex].miDataSize);
			static_cast<ID3D12Resource*>(uploadBuffer.getNativeBuffer())->Unmap(0, nullptr);

			RenderDriver::Common::CBuffer& destBuffer = *mapBuffers[dx12ShaderResource.maHandles[iTripleBufferIndex]].get();
		}

		/*
		**
		*/
		void Serializer::initRenderTarget(
			PLATFORM_OBJECT_HANDLE& renderTargetHandle,
			PLATFORM_OBJECT_HANDLE& colorRenderTargetHeapHandle,
			PLATFORM_OBJECT_HANDLE& depthStencilRenderTargetHeapHandle,
			AttachmentInfo const& attachmentInfo,
			uint32_t iWidth,
			uint32_t iHeight,
			float const* afClearColor,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Common::RenderTargetDescriptor desc =
			{
				/* uint32_t                                        miWidth									*/		iWidth,
				/* uint32_t                                        miHeight									*/		iHeight,
				/* Format                                          mColorFormat								*/		attachmentInfo.mFormat,
				/* Format                                          mDepthStencilFormat						*/		RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT,
				/* float                                           mafClearColor[4]							*/		{afClearColor[0], afClearColor[1], afClearColor[2], afClearColor[3]},
				/* float                                           mafClearDepthStencil[4]					*/		{0.0f, 0.0f, 0.0f, 1.0f},
				/* RenderDriver::Common::CDescriptorHeap* mpColorDescriptorHeap								*/		nullptr,
				/* RenderDriver::Common::CDescriptorHeap* mpDepthStencilDescriptorHeap						*/		nullptr,
				/* mbComputeShaderWritable																	*/		attachmentInfo.mbComputeShaderWritable,
				/* miShaderResourceIndex																	*/		0,
			};

			if(!attachmentInfo.mbHasDepthStencil)
			{
				desc.mDepthStencilFormat = RenderDriver::Common::Format::UNKNOWN;
			}

			if(colorRenderTargetHeapHandle == 0)
			{
				RenderDriver::Common::DescriptorHeapDescriptor renderTargetDescriptorHeapDesc = {};
				renderTargetDescriptorHeapDesc.miNumDescriptors = 1;
				renderTargetDescriptorHeapDesc.mType = RenderDriver::Common::DescriptorHeapType::RenderTarget;
				renderTargetDescriptorHeapDesc.mFlag = RenderDriver::Common::DescriptorHeapFlag::None;

				if(attachmentInfo.mbComputeShaderWritable)
				{
					renderTargetDescriptorHeapDesc.mType = RenderDriver::Common::DescriptorHeapType::General;
					renderTargetDescriptorHeapDesc.mFlag = RenderDriver::Common::DescriptorHeapFlag::ShaderVisible;
				}

				std::unique_ptr<RenderDriver::DX12::CDescriptorHeap> pColorDescriptorHeap = std::make_unique<RenderDriver::DX12::CDescriptorHeap>();
				colorRenderTargetHeapHandle = pColorDescriptorHeap->create(renderTargetDescriptorHeapDesc, device);
				pColorDescriptorHeap->setID(attachmentInfo.mName + " Color Render Target Descriptor Heap");
				mapDescriptorHeaps[colorRenderTargetHeapHandle] = std::move(pColorDescriptorHeap);

				desc.mpColorDescriptorHeap = mapDescriptorHeaps[colorRenderTargetHeapHandle].get();

				if(attachmentInfo.mbComputeShaderWritable == false && attachmentInfo.mbHasDepthStencil)
				{
					std::unique_ptr<RenderDriver::DX12::CDescriptorHeap> pDepthStencilDescriptorHeap = std::make_unique<RenderDriver::DX12::CDescriptorHeap>();
					renderTargetDescriptorHeapDesc.mType = RenderDriver::Common::DescriptorHeapType::DepthStencil;
					depthStencilRenderTargetHeapHandle = pDepthStencilDescriptorHeap->create(renderTargetDescriptorHeapDesc, device);
					pDepthStencilDescriptorHeap->setID(attachmentInfo.mName + " Depth Stencil Render Target Descriptor Heap");
					mapDescriptorHeaps[depthStencilRenderTargetHeapHandle] = std::move(pDepthStencilDescriptorHeap);

					desc.mpDepthStencilDescriptorHeap = mapDescriptorHeaps[depthStencilRenderTargetHeapHandle].get();
				}
			}
			else
			{
				// !!!! PASS IN SHADER RESOURCE INDEX FOR DESCRIPTOR HEAP INDEX !!!!!

				desc.mpColorDescriptorHeap = mapDescriptorHeaps[colorRenderTargetHeapHandle].get();
				desc.miShaderResourceIndex = attachmentInfo.miShaderResourceIndex;
			}

			std::unique_ptr<RenderDriver::DX12::CRenderTarget> pRenderTarget = std::make_unique<RenderDriver::DX12::CRenderTarget>();
			pRenderTarget->setID(attachmentInfo.mName);
			renderTargetHandle = pRenderTarget->create(desc, device);
			mapRenderTargets[renderTargetHandle] = std::move(pRenderTarget);
			mapRenderTargets[renderTargetHandle]->setID(attachmentInfo.mName);
		}

		/*
		**
		*/
		void Serializer::initRenderTargetBuffer(
			PLATFORM_OBJECT_HANDLE& renderTargetBuffer,
			AttachmentInfo const& attachmentInfo,
			uint32_t iWidth,
			uint32_t iHeight,
			uint32_t iPipelineStateIndex,
			RenderDriver::Common::CDevice& device)
		{
			std::unique_ptr<RenderDriver::DX12::CBuffer> pBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();

			RenderDriver::Common::BufferDescriptor desc;
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(attachmentInfo.mFormat);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(attachmentInfo.mFormat);

			if(attachmentInfo.miDataSize == UINT32_MAX || attachmentInfo.miDataSize == 0)
			{
				desc.miSize = iWidth * iHeight * iNumChannels * iChannelSize;
			}
			else
			{
				desc.miSize = attachmentInfo.miDataSize;
			}

			desc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
			if(attachmentInfo.mbComputeShaderWritable)
			{
				desc.mFlags = RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess;
			}

			renderTargetBuffer = pBuffer->create(desc, device);
			pBuffer->setID(attachmentInfo.mName);
			pBuffer->getDescriptor().miImageViewWidth = iWidth;
			pBuffer->getDescriptor().miImageViewHeight = iHeight;
			pBuffer->getDescriptor().mImageViewFormat = attachmentInfo.mFormat;
			mapBuffers[renderTargetBuffer] = std::move(pBuffer);
		}

		/*
		**
		*/
		void Serializer::initAttachmentImage(
			PLATFORM_OBJECT_HANDLE& attachmentImageHandle,
			AttachmentInfo const& attachmentInfo,
			uint32_t iWidth,
			uint32_t iHeight,
			RenderDriver::Common::CDevice& device)
		{
			std::unique_ptr<RenderDriver::DX12::CImage> pImage = std::make_unique<RenderDriver::DX12::CImage>();

			float afDefaultClearColor[4] = { 0.0f, 0.0f, 0.3f, 0.0f };

			RenderDriver::Common::ImageDescriptor desc = {};
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(attachmentInfo.mFormat);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(attachmentInfo.mFormat);
			desc.miWidth = iWidth;
			desc.miHeight = iHeight;
			desc.miNumImages = 1;
			desc.mFormat = attachmentInfo.mFormat;
			desc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess;
			desc.mafClearColor = afDefaultClearColor;

			attachmentImageHandle = pImage->create(desc, device);
			pImage->setID(attachmentInfo.mName);
			pImage->getDescriptor().miWidth = iWidth;
			pImage->getDescriptor().miHeight = iHeight;
			pImage->getDescriptor().miNumImages = 1;
			pImage->getDescriptor().mFormat = desc.mFormat;
			mapImages[attachmentImageHandle] = std::move(pImage);
		}

		/*
		**
		*/
		void Serializer::createAttachmentView(
			Render::Common::RenderJobInfo& renderJob,
			uint32_t iAttachmentIndex,
			uint32_t iShaderResourceIndex,
			uint32_t iTotalSize,
			bool bBufferAttachment,
			bool bExternalImageAttachment,
			Render::Common::AttachmentType const& viewType,
			RenderDriver::Common::CDevice& device)
		{
			DEBUG_PRINTF("\tcreate attachment image view: %s (%d)\n",
				renderJob.maShaderResourceInfo[iShaderResourceIndex].mName.c_str(),
				iShaderResourceIndex);

			for(uint32_t i = 0; i < NUM_BUFFER_OBJECTS; i++)
			{
				RenderDriver::Common::CDescriptorHeap& dx12DescriptorHeap = *getDescriptorHeap(renderJob.maDescriptorHeapHandles[i]).get();

				RenderDriver::Common::CBuffer* pBuffer = nullptr;
				RenderDriver::Common::CImage* pImage = nullptr;
				RenderDriver::Common::Format imageFormat;
				uint32_t iBaseElementSize = 0;

				PLATFORM_OBJECT_HANDLE renderTargetAttachmentHandle = renderJob.maInputRenderTargetAttachments[iAttachmentIndex];
				if(bBufferAttachment)
				{
					pBuffer = getBuffer(renderTargetAttachmentHandle).get();
					imageFormat = pBuffer->getDescriptor().mImageViewFormat;
					iBaseElementSize = SerializeUtils::Common::getBaseComponentSize(imageFormat);
				}
				else if(bExternalImageAttachment)
				{
					if(isImage(renderTargetAttachmentHandle))
					{
						pImage = getImage(renderTargetAttachmentHandle).get();
					}
					else
					{
						pImage = getRenderTarget(renderTargetAttachmentHandle)->getImage().get();
					}
					WTFASSERT(pImage, "Can\'t find image for handle %d\n", renderTargetAttachmentHandle);
					imageFormat = pImage->getFormat();
					iBaseElementSize = SerializeUtils::Common::getBaseComponentSize(imageFormat);
				}
				else
				{
					RenderDriver::Common::CRenderTarget& renderTarget = *getRenderTarget(renderTargetAttachmentHandle).get();
					pImage = renderTarget.getImage().get();
					assert(pImage);

					imageFormat = pImage->getFormat();
					iBaseElementSize = SerializeUtils::Common::getBaseComponentSize(imageFormat);
				}

				if(viewType == Render::Common::AttachmentType::BufferIn)
				{
					RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
					{
						/* .mFormat							*/	RenderDriver::Common::Format::UNKNOWN,
						/* .mpBuffer						*/	pBuffer,
						/* .mpCounterBuffer = nullptr		*/  nullptr,
						/* .miSize							*/	iTotalSize,
						/* .miStructStrideSize = 0			*/	iBaseElementSize,
						/* .mFlags;							*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
						/* .mViewType						*/	RenderDriver::Common::ResourceViewType::ShaderResourceView,
						/* .mDescriptorHeap					*/	&dx12DescriptorHeap,
						/* .miDescriptorOffset				*/	iShaderResourceIndex
					};

					RenderDriver::DX12::CBufferView bufferView;
					bufferView.create(bufferViewDesc, device);
				}
				else
				{
					RenderDriver::Common::ImageViewDescriptor imageViewDesc =
					{
						/* .mViewType				*/				RenderDriver::Common::ResourceViewType::ShaderResourceView,
						/* .mDimension				*/				(bBufferAttachment) ? RenderDriver::Common::Dimension::Buffer : RenderDriver::Common::Dimension::Texture2D,
						/* .mShaderResourceViewDimension */			(bBufferAttachment) ? RenderDriver::Common::ShaderResourceViewDimension::Buffer : RenderDriver::Common::ShaderResourceViewDimension::Texture2D,
						/* .mFormat					*/				imageFormat,
						/* .miTotalSize				*/				iTotalSize,
						/* .miElementSize			*/				iBaseElementSize,
						/* .mpDescriptorHeap		*/				&dx12DescriptorHeap,
						/* .mpImage					*/				pImage,
						/* .mpBuffer				*/				pBuffer,
						/* .miShaderResourceIndex	*/				iShaderResourceIndex,
						/* .miNumImages				*/				1,
					};

					RenderDriver::DX12::CImageView imageView;
					imageView.create(imageViewDesc, device);
				}
			}
		}

		/*
		**
		*/
		void Serializer::createWritableAttachmentView(
			Render::Common::RenderJobInfo& renderJob,
			uint32_t iAttachmentIndex,
			uint32_t iShaderResourceIndex,
			uint32_t iTotalSize,
			bool bBufferAttachment,
			bool bExternalImageAttachment,
			Render::Common::AttachmentType const& viewType,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Common::CDescriptorHeap& dx12DescriptorHeap = *getDescriptorHeap(renderJob.maDescriptorHeapHandles[0]).get();

			RenderDriver::Common::CImage* pImage = nullptr;
			RenderDriver::Common::Format imageFormat;
			uint32_t iBaseElementSize = 0;

			PLATFORM_OBJECT_HANDLE renderTargetAttachmentHandle = renderJob.maInputRenderTargetAttachments[iAttachmentIndex];
			if(isImage(renderTargetAttachmentHandle))
			{
				pImage = getImage(renderTargetAttachmentHandle).get();
			}
			else
			{
				pImage = getRenderTarget(renderTargetAttachmentHandle)->getImage().get();
			}
			WTFASSERT(pImage, "Can\'t find image for handle %d\n", renderTargetAttachmentHandle);
			imageFormat = pImage->getFormat();
			iBaseElementSize = SerializeUtils::Common::getBaseComponentSize(imageFormat);
			RenderDriver::Common::ImageViewDescriptor imageViewDesc =
			{
				/* .mViewType				*/				RenderDriver::Common::ResourceViewType::UnorderedAccessView,
				/* .mDimension				*/				(bBufferAttachment) ? RenderDriver::Common::Dimension::Buffer : RenderDriver::Common::Dimension::Texture2D,
				/* .mShaderResourceViewDimension */			(bBufferAttachment) ? RenderDriver::Common::ShaderResourceViewDimension::Buffer : RenderDriver::Common::ShaderResourceViewDimension::Texture2D,
				/* .mFormat					*/				imageFormat,
				/* .miTotalSize				*/				iTotalSize,
				/* .miElementSize			*/				iBaseElementSize,
				/* .mpDescriptorHeap		*/				&dx12DescriptorHeap,
				/* .mpImage					*/				pImage,
				/* .mpBuffer				*/				nullptr,
				/* .miShaderResourceIndex	*/				iShaderResourceIndex,
				/* .miNumImages				*/				1,
			};

			RenderDriver::DX12::CImageView imageView;
			imageView.create(imageViewDesc, device);
		}

		/*
		**
		*/
		void Serializer::associateReadOnlyBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
			uint32_t iBufferSize = 1 << 24;

		}

		/*
		**
		*/
		void Serializer::associateReadWriteBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
			
		}

		/*
		**
		*/
		void Serializer::associateConstantBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo,
			std::string const& renderJobName)
		{
			std::vector<std::string> aRenderJobNames =
			{
				"Mesh Instance Visibility Compute",
				"Mesh Cluster Visibility Compute",
				"Pre Pass Mesh Cluster Visibility Compute",
				"Post Pass Mesh Cluster Visibility Compute",
				"Draw Pre Pass Mesh Cluster Graphics",
				"Draw Post Pass Mesh Cluster Graphics",
				"Over Draw Graphics"
			};

			if(std::find(aRenderJobNames.begin(), aRenderJobNames.end(), renderJobName) != aRenderJobNames.end())
			{
				std::unique_ptr<RenderDriver::DX12::CBuffer> pBuffer = std::make_unique<RenderDriver::DX12::CBuffer>();

				uint32_t iBufferSize = 4096;

				RenderDriver::Common::BufferDescriptor desc;
				desc.miSize = iBufferSize;
				desc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
				PLATFORM_OBJECT_HANDLE handle = pBuffer->create(desc, *mpDevice);

				std::string constantBufferName = renderJobName + ".render-pass-constant-buffer";
				pBuffer->setID(constantBufferName);
				pBuffer->getDescriptor().miImageViewWidth = static_cast<uint32_t>(desc.miSize);
				pBuffer->getDescriptor().miImageViewHeight = 1;
				pBuffer->getDescriptor().mImageViewFormat = desc.mFormat;
				mapBuffers[handle] = std::move(pBuffer);

				shaderResourceInfo.mDesc.mDimension = RenderDriver::Common::Dimension::Buffer;
				shaderResourceInfo.mDesc.miWidth = iBufferSize;
				shaderResourceInfo.mExternalResource.mDimension = RenderDriver::Common::Dimension::Buffer;
				shaderResourceInfo.mExternalResource.mpBuffer = mapBuffers[handle].get();
				shaderResourceInfo.mExternalResource.miGPUAddressOffset = 0;
				shaderResourceInfo.mExternalResource.mShaderResourceViewDimension = RenderDriver::Common::ShaderResourceViewDimension::Buffer;
				shaderResourceInfo.maDataInfo[0].miDataSize = iBufferSize;

				return;
			}

		}

		/*
		**
		*/
		void Serializer::associateRayTraceSceneBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
			
		}

		/*
		**
		*/
		void Serializer::associateExternalBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
			
		}

		/*
		**
		*/
		void Serializer::setBufferView(
			Render::Common::RenderJobInfo& renderJob,
			RenderDriver::Common::CBuffer* pBuffer,
			uint32_t iShaderResourceIndex,
			uint32_t iBufferSize,
			RenderDriver::Common::CDevice& device)
		{
			for(uint32_t i = 0; i < NUM_BUFFER_OBJECTS; i++)
			{
				RenderDriver::Common::CDescriptorHeap& dx12DescriptorHeap = *getDescriptorHeap(renderJob.maDescriptorHeapHandles[i]).get();
				RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
				{
					/* .mFormat							*/	RenderDriver::Common::Format::UNKNOWN,
					/* .mpBuffer						*/	pBuffer,
					/* .mpCounterBuffer = nullptr		*/  nullptr,
					/* .miSize							*/	iBufferSize,
					/* .miStructStrideSize = 0			*/	sizeof(float),
					/* .mFlags;							*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
					/* .mViewType						*/	RenderDriver::Common::ResourceViewType::ShaderResourceView,
					/* .mDescriptorHeap					*/	&dx12DescriptorHeap,
					/* .miDescriptorOffset				*/	iShaderResourceIndex
				};

				RenderDriver::DX12::CBufferView bufferView;
				bufferView.create(bufferViewDesc, device);
			}
		}

		/*
		**
		*/
		void Serializer::platformCreateFrameBuffer(
			Render::Common::RenderJobInfo& renderJobInfo)
		{

		}

		/*
		**
		*/
		void Serializer::platformUpdateSamplers(
			Render::Common::RenderJobInfo& renderJobInfo)
		{

		}

		/*
		**
		*/
		void Serializer::platformUpdateOutputAttachments(
			Render::Common::RenderJobInfo& renderJobInfo)
		{

		}

    }	// DX12

}	// RenderDriver