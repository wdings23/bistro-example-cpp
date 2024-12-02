#include <render-driver/Vulkan/DeviceVulkan.h>
#include <render-driver/Vulkan/ImageVulkan.h>
#include <render-driver/Vulkan/ImageViewVulkan.h>
#include <render-driver/Vulkan/BufferVulkan.h>
#include <render-driver/Vulkan/BufferViewVulkan.h>
#include <render-driver/Vulkan/PipelineStateVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>
#include <render-driver/Vulkan/FenceVulkan.h>
#include <render-driver/Vulkan/DescriptorHeapVulkan.h>
#include <render-driver/Vulkan/DescriptorSetVulkan.h>
#include <render-driver/Vulkan/RenderTargetVulkan.h>
#include <render-driver/Vulkan/FrameBufferVulkan.h>

#include <render/Vulkan/RenderJobSerializerVulkan.h>
#include <render/Vulkan/RendererVulkan.h>


#include <LogPrint.h>
#include <wtfassert.h>

#include "image_utils.h"

#include <sstream>

namespace Render
{
	namespace Vulkan
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
			RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
			VkDevice* pNativeDevice = static_cast<VkDevice*>(deviceVulkan.getNativeDevice());

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
				std::unique_ptr<RenderDriver::Vulkan::CCommandAllocator> pCommandAllocator = std::make_unique<RenderDriver::Vulkan::CCommandAllocator>();
				aCommandAllocatorHandles[iTripleBufferIndex] = pCommandAllocator->create(commandAllocatorDesc, deviceVulkan);
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

			std::unique_ptr<RenderDriver::Vulkan::CCommandBuffer> pCommandBuffer = std::make_unique<RenderDriver::Vulkan::CCommandBuffer>();
			commandListHandle = pCommandBuffer->create(commandBufferDesc, deviceVulkan);
			std::string commandBufferName = renderJobName + " Command List";
			pCommandBuffer->setID(commandBufferName);
			mapCommandBuffers[commandListHandle] = std::move(pCommandBuffer);
		}

		/*
		**
		*/
		//RenderDriver::Common::CDescriptorHeap* Serializer::registerObject(
		//	std::unique_ptr<RenderDriver::Vulkan::CDescriptorHeap>& pObject,
		//	PLATFORM_OBJECT_HANDLE handle)
		//
		//	mapDescriptorHeaps[handle] = std::move(pObject);
		//	return mapDescriptorHeaps[handle].get();
		//

		/*
		**
		*/
		//RenderDriver::Common::CRenderTarget* Serializer::registerObject(
		//	std::unique_ptr<RenderDriver::Vulkan::CRenderTarget>& pObject,
		//	PLATFORM_OBJECT_HANDLE handle)
		//{
		//	mapRenderTargets[handle] = std::move(pObject);
		//	return mapRenderTargets[handle].get();
		//}

		/*
		**
		*/
		RenderDriver::Common::CCommandAllocator* Serializer::registerObject(
			std::unique_ptr<RenderDriver::Vulkan::CCommandAllocator>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapCommandAllocators[handle] = std::move(pObject);
			return mapCommandAllocators[handle].get();
		}

		/*
		**
		*/
		RenderDriver::Common::CCommandBuffer* Serializer::registerObject(
			std::unique_ptr<RenderDriver::Vulkan::CCommandBuffer>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapCommandBuffers[handle] = std::move(pObject);
			return mapCommandBuffers[handle].get();
		}

		/*
		**
		*/
		RenderDriver::Common::CFence* Serializer::registerObject(
			std::unique_ptr<RenderDriver::Vulkan::CFence>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapFences[handle] = std::move(pObject);
			return mapFences[handle].get();
		}

		/*
		**
		*/
		RenderDriver::Common::CImageView* Serializer::registerObject(
			std::unique_ptr<RenderDriver::Vulkan::CImageView>& pObject,
			PLATFORM_OBJECT_HANDLE handle)
		{
			mapImageViews[handle] = std::move(pObject);
			return mapImageViews[handle].get();
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
			RenderDriver::Vulkan::CDescriptorSet descriptorSet;
			RenderDriver::Common::DescriptorSetDescriptor desc;
			desc.mpaShaderResources = &aShaderResourceInfo;
			desc.mPipelineType = pipelineType;
			desc.miNumRootConstants = iNumRootConstants;

			std::unique_ptr<RenderDriver::Vulkan::CDescriptorSet> pDescriptorSet = std::make_unique<RenderDriver::Vulkan::CDescriptorSet>();
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
			VkDevice* nativeDevice = static_cast<VkDevice*>(static_cast<RenderDriver::Vulkan::CDevice&>(device).getNativeDevice());

			FILE* fp = fopen(std::string(shaderInfo.mFileOutputPath + ".spv").c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			std::vector<char> acShaderBuffer(iFileSize + 1);
			acShaderBuffer[iFileSize] = '\0';
			fread(acShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			std::unique_ptr<RenderDriver::Vulkan::CPipelineState> pPipelineState = std::make_unique<RenderDriver::Vulkan::CPipelineState>();
			RenderDriver::Vulkan::CPipelineState::ComputePipelineStateDescriptor pipelineStateDesc;
			pipelineStateDesc.miFlags = 0;
			pipelineStateDesc.miComputeShaderSize = (uint32_t)iFileSize;
			pipelineStateDesc.mpComputeShader = (uint8_t*)acShaderBuffer.data();
			pipelineStateDesc.mpDescriptor = mapDescriptorSets[descriptorHandle].get();
			pipelineStateDesc.miNumRootConstants = renderJob.miNumRootConstants;
			memcpy(pipelineStateDesc.mafRootConstants, renderJob.mafRootConstants, sizeof(pipelineStateDesc.mafRootConstants));

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
			VkDevice* nativeDevice = static_cast<VkDevice*>(static_cast<RenderDriver::Vulkan::CDevice&>(device).getNativeDevice());

			std::vector<RenderDriver::Common::VertexFormat> aInputFormat;
			SerializeUtils::Common::getVertexFormat(aInputFormat, doc);

			// vertex shader
			FILE* fp = fopen(std::string(vertexShaderInfo.mFileOutputPath + ".spv").c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			std::vector<char> acShaderBufferVS(iFileSize);
			fread(acShaderBufferVS.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			// fragment shader
			fp = fopen(std::string(fragmentShaderInfo.mFileOutputPath + ".spv").c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			std::vector<char> acShaderBufferPS(iFileSize);
			fread(acShaderBufferPS.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			DEBUG_PRINTF("\nvertex shader: %s\n", vertexShaderInfo.mFileOutputPath.c_str());
			DEBUG_PRINTF("fragment shader: %s\n\n", fragmentShaderInfo.mFileOutputPath.c_str());

			RenderDriver::Vulkan::CPipelineState::GraphicsPipelineStateDescriptor pipelineStateDesc;
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
			pipelineStateDesc.mbOutputPresent = (renderJob.mPassType == Render::Common::PassType::SwapChain) ? true : false;
			pipelineStateDesc.mbFullTrianglePass = (renderJob.mPassType == Render::Common::PassType::FullTriangle) ? true : false;

			pipelineStateDesc.miImageWidth = 0;
			pipelineStateDesc.miImageHeight = 0;

			pipelineStateDesc.miNumRootConstants = renderJob.miNumRootConstants;
			memcpy(pipelineStateDesc.mafRootConstants, renderJob.mafRootConstants, sizeof(pipelineStateDesc.mafRootConstants));

			std::unique_ptr<RenderDriver::Vulkan::CPipelineState> pPipelineState = std::make_unique<RenderDriver::Vulkan::CPipelineState>();
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
			VkDevice* nativeDevice = static_cast<VkDevice*>(static_cast<RenderDriver::Vulkan::CDevice&>(device).getNativeDevice());

			return 0;
		}

		/*
		**
		*/
		PLATFORM_OBJECT_HANDLE Serializer::createDescriptorHeap(
			uint32_t iNumShaderResources,
			RenderDriver::Common::CDevice& device)
		{
			RenderDriver::Vulkan::CDescriptorHeap descriptorHeap;

			RenderDriver::Common::DescriptorHeapDescriptor desc;
			desc.mFlag = RenderDriver::Common::DescriptorHeapFlag::ShaderVisible;
			desc.mType = RenderDriver::Common::DescriptorHeapType::General;
			desc.miNumDescriptors = iNumShaderResources;
			
			std::unique_ptr<RenderDriver::Vulkan::CDescriptorHeap> pDescriptorHeap = std::make_unique<RenderDriver::Vulkan::CDescriptorHeap>();
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
#if 0
			VkDevice* nativeDevice = static_cast<VkDevice*>(static_cast<RenderDriver::Vulkan::CDevice&>(device).getNativeDevice());
			
			bool bIsTexture = (shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN ||
				shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT ||
				shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN_OUT);

			bool bIsBuffer = (shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN ||
				shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_OUT ||
				shaderResource.mType == ShaderResourceType::RESOURCE_TYPE_BUFFER_IN_OUT);

			uint32_t iTotalSize = static_cast<uint32_t>(shaderResource.maDataInfo[0].miDataSize);

			RenderDriver::Common::CDescriptorSet* pDescriptorSet = mapDescriptorSets[descriptorHeapHandle].get();

			// buffer view
			if(bIsBuffer)
			{
				RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
				{
					/* .mFormat				*/  shaderResource.mDesc.mViewFormat,
					/* .mpBuffer			*/  shaderResource.mExternalResource.mpBuffer,
					/* .mpCounterBuffer		*/  nullptr,
					/* .miSize				*/  (iTotalSize == 0) ? CONSTANT_BUFFER_SIZE : iTotalSize,
					/* .miStructStrideSize	*/  shaderResource.miStructByteStride,
					/* .mFlags				*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
					/* .mViewType			*/  shaderResource.mViewType,
					/* .mpDescriptorHeap	*/  nullptr,
					/* .miDescriptorOffset	*/  iResourceIndex,
					/* .mpDescriptorSet		*/  pDescriptorSet,
				};

				std::unique_ptr<RenderDriver::Vulkan::CBufferView> pBufferView = std::make_unique<RenderDriver::Vulkan::CBufferView>();
				PLATFORM_OBJECT_HANDLE bufferViewHandle = pBufferView->create(bufferViewDesc, device);
				mapBufferViews[bufferViewHandle] = std::move(pBufferView);
			}
			else if(bIsTexture)
			{
				RenderDriver::Common::ImageViewDescriptor imageViewDesc =
				{
					/* .mViewType				*/				shaderResource.mViewType,
					/* .mDimension				*/				shaderResource.mExternalResource.mDimension,
					/* .mShaderResourceViewDimension */			shaderResource.mExternalResource.mShaderResourceViewDimension,
					/* .mFormat					*/				shaderResource.mDesc.mFormat,
					/* .miTotalSize				*/				iTotalSize,
					/* .miElementSize			*/				shaderResource.miStructByteStride,
					/* .mpDescriptorHeap		*/				nullptr,
					/* .mpImage					*/				shaderResource.mExternalResource.mpImage,
					/* .mpBuffer				*/				nullptr,
					/* .miShaderResourceIndex	*/				iResourceIndex,
					/* .miNumImages				*/				shaderResource.mExternalResource.miNumImages,
					/* .mpDescriptorSet			*/				pDescriptorSet,
				};
				RenderDriver::Vulkan::CImageView imageView;
				PLATFORM_OBJECT_HANDLE imageViewHandle = imageView.create(imageViewDesc, device);
			}
#endif // #if 0

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
			WTFASSERT(0, "Implement me");

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
			WTFASSERT(0, "Implement me");
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
			WTFASSERT(0, "Implement me");
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

				std::unique_ptr<RenderDriver::Vulkan::CDescriptorHeap> pColorDescriptorHeap = std::make_unique<RenderDriver::Vulkan::CDescriptorHeap>();
				colorRenderTargetHeapHandle = pColorDescriptorHeap->create(renderTargetDescriptorHeapDesc, device);
				pColorDescriptorHeap->setID(attachmentInfo.mName + " Color Render Target Descriptor Heap");
				mapDescriptorHeaps[colorRenderTargetHeapHandle] = std::move(pColorDescriptorHeap);

				desc.mpColorDescriptorHeap = mapDescriptorHeaps[colorRenderTargetHeapHandle].get();

				if(attachmentInfo.mbComputeShaderWritable == false && attachmentInfo.mbHasDepthStencil)
				{
					std::unique_ptr<RenderDriver::Vulkan::CDescriptorHeap> pDepthStencilDescriptorHeap = std::make_unique<RenderDriver::Vulkan::CDescriptorHeap>();
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

			std::unique_ptr<RenderDriver::Vulkan::CRenderTarget> pRenderTarget = std::make_unique<RenderDriver::Vulkan::CRenderTarget>();
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
			std::unique_ptr<RenderDriver::Vulkan::CBuffer> pBuffer = std::make_unique<RenderDriver::Vulkan::CBuffer>();

			RenderDriver::Common::BufferDescriptor desc = {};
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(attachmentInfo.mFormat);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(attachmentInfo.mFormat);

			// data size
			if(attachmentInfo.miDataSize == UINT32_MAX || attachmentInfo.miDataSize == 0)
			{
				desc.miSize = iWidth * iHeight * iNumChannels * iChannelSize;
			}
			else
			{
				desc.miSize = attachmentInfo.miDataSize;
			}

			// usage
			desc.mBufferUsage = (desc.miSize > 65536 || attachmentInfo.mbComputeShaderWritable) ? RenderDriver::Common::BufferUsage::StorageBuffer : RenderDriver::Common::BufferUsage::UniformBuffer;
			desc.mBufferUsage = static_cast<RenderDriver::Common::BufferUsage>(
				static_cast<uint32_t>(desc.mBufferUsage) | static_cast<uint32_t>(RenderDriver::Common::BufferUsage::TransferDest));

			// format and writeable flag
			desc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
			if(attachmentInfo.mbComputeShaderWritable)
			{
				desc.mFlags = RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess;
			}

			// create and register to global buffer dictionary
			renderTargetBuffer = pBuffer->create(desc, device);
			pBuffer->setID(attachmentInfo.mName);
			pBuffer->getDescriptor().miImageViewWidth = iWidth;
			pBuffer->getDescriptor().miImageViewHeight = iHeight;
			pBuffer->getDescriptor().mImageViewFormat = attachmentInfo.mFormat;
			mapBuffers[renderTargetBuffer] = std::move(pBuffer);

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = *(static_cast<VkBuffer*>(mapBuffers[renderTargetBuffer]->getNativeBuffer()));
			bufferInfo.range = desc.miSize;

			if(attachmentInfo.miShaderResourceIndex != UINT32_MAX)
			{
				auto const& pipeline = maPipelines[iPipelineStateIndex];
				RenderDriver::Vulkan::CDescriptorSet* pDescriptorSet = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(getDescriptorSet(pipeline.mDescriptorHandle).get());
				std::vector<VkDescriptorSet>& aNativeDescriptorSets = pDescriptorSet->getNativeDescriptorSets();

				// get the binding index and set index from the layout info saved in the descriptor set
				uint32_t iDescriptorSetIndex = UINT32_MAX;
				uint32_t iBindingIndex = UINT32_MAX;
				auto const& aLayoutInfo = pDescriptorSet->getLayoutInfo();
				for(uint32_t i = 0; i < static_cast<uint32_t>(aLayoutInfo.size()); i++)
				{
					if(aLayoutInfo[i].miShaderResourceIndex == attachmentInfo.miShaderResourceIndex)
					{
						iDescriptorSetIndex = aLayoutInfo[i].miSetIndex;
						iBindingIndex = aLayoutInfo[i].miBindingIndex;
						break;
					}
				}

				WTFASSERT(iDescriptorSetIndex != UINT32_MAX, "Can\'t find descriptor set for attachment \"%s\"",
					attachmentInfo.mName.c_str());

				WTFASSERT(iBindingIndex != UINT32_MAX, "Can\'t find descriptor set for attachment \"%s\"",
					attachmentInfo.mName.c_str());

				// update the descriptor with this buffer
				VkWriteDescriptorSet writeDescriptorSet = {};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = aNativeDescriptorSets[iDescriptorSetIndex];			// 0 due output buffer can be created only in compute shader
				writeDescriptorSet.descriptorType = (desc.mBufferUsage == RenderDriver::Common::BufferUsage::UniformBuffer) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writeDescriptorSet.pBufferInfo = &bufferInfo;
				writeDescriptorSet.dstBinding = iBindingIndex;
				writeDescriptorSet.descriptorCount = 1;

				VkDevice* pNativeDevice = static_cast<VkDevice*>(static_cast<RenderDriver::Vulkan::CDevice&>(device).getNativeDevice());

				vkUpdateDescriptorSets(
					*pNativeDevice,
					1,
					&writeDescriptorSet,
					0,
					nullptr);
			}
			else
			{
				DEBUG_PRINTF("attachment: \"%s\" has no shader index\n", 
					attachmentInfo.mName.c_str());
			}

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
			std::unique_ptr<RenderDriver::Vulkan::CImage> pImage = std::make_unique<RenderDriver::Vulkan::CImage>();

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

			RenderDriver::Vulkan::CDevice& deviceVulkan = static_cast<RenderDriver::Vulkan::CDevice&>(device);
			VkDevice& nativeDevice = *(static_cast<VkDevice*>(deviceVulkan.getNativeDevice()));

			uint32_t iDescriptorSetIndex = (renderJob.maShaderResourceInfo[iShaderResourceIndex].mShaderType == RenderDriver::Common::ShaderType::Fragment) ? 1 : 0;

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

			Render::Common::AttachmentType viewTypeCopy = viewType;
			viewTypeCopy = (viewType == Render::Common::AttachmentType::None) ? Render::Common::AttachmentType::TextureIn : viewType;
			if(bBufferAttachment)
			{
				viewTypeCopy = Render::Common::AttachmentType::BufferIn;
			}

			// create new image and buffer views
			if(viewTypeCopy == Render::Common::AttachmentType::BufferIn)
			{
				RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
				{
					/* .mFormat							*/	RenderDriver::Common::Format::R32_FLOAT,
					/* .mpBuffer						*/	pBuffer,
					/* .mpCounterBuffer = nullptr		*/  nullptr,
					/* .miSize							*/	iTotalSize,
					/* .miStructStrideSize = 0			*/	iBaseElementSize,
					/* .mFlags;							*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
					/* .mViewType						*/	RenderDriver::Common::ResourceViewType::ShaderResourceView,
					/* .mDescriptorHeap					*/	nullptr,
					/* .miDescriptorOffset				*/	iShaderResourceIndex
				};

				RenderDriver::Vulkan::CBufferView bufferView;
				bufferView.create(bufferViewDesc, device);

				
				// fill out VkWriteDescriptorSet for image and buffers with image and buffer views
				// vkUpdateDescriptorSets

				Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(mpRenderer);
				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = *(static_cast<VkBuffer*>(pBuffer->getNativeBuffer()));
				bufferInfo.range = iTotalSize;

				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo = renderJob.maShaderResourceInfo[iShaderResourceIndex];
				uint32_t iDescriptorSetIndex = (shaderResourceInfo.mShaderType == RenderDriver::Common::ShaderType::Fragment) ? 1 : 0;

				auto const& pipeline = maPipelines[renderJob.miPipelineIndex];
				RenderDriver::Vulkan::CDescriptorSet* pDescriptorSet = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(getDescriptorSet(pipeline.mDescriptorHandle).get());
				std::vector<VkDescriptorSet>& aNativeDescriptorSets = pDescriptorSet->getNativeDescriptorSets();
				WTFASSERT(aNativeDescriptorSets.size() > iDescriptorSetIndex, "Descriptor set index out of bounds: %d", iDescriptorSetIndex);

				// get the binding index and set index from the layout info saved in the descriptor set
				uint32_t iBindingIndex = UINT32_MAX;
				auto const& aLayoutInfo = pDescriptorSet->getLayoutInfo();
				for(uint32_t i = 0; i < static_cast<uint32_t>(aLayoutInfo.size()); i++)
				{
					if(aLayoutInfo[i].miShaderResourceIndex == iShaderResourceIndex)
					{
						iDescriptorSetIndex = aLayoutInfo[i].miSetIndex;
						iBindingIndex = aLayoutInfo[i].miBindingIndex;
						break;
					}
				}

				WTFASSERT(iBindingIndex != UINT32_MAX, "Can\'t find binding index for shader resource \"%s\"",
					renderJob.maShaderResourceInfo[iShaderResourceIndex].mName.c_str());

				VkWriteDescriptorSet writeDescriptorSet = {};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.dstSet = aNativeDescriptorSets[iDescriptorSetIndex];
				writeDescriptorSet.descriptorType = (iTotalSize < 65536) ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				writeDescriptorSet.pBufferInfo = &bufferInfo;
				writeDescriptorSet.dstBinding = iBindingIndex;
				writeDescriptorSet.descriptorCount = 1;

				vkUpdateDescriptorSets(
					nativeDevice,
					1,
					&writeDescriptorSet,
					0,
					nullptr);
			}
			else if(viewTypeCopy == Render::Common::AttachmentType::TextureIn)
			{
				// fill out VkWriteDescriptorSet for image and buffers with image and buffer views
				// vkUpdateDescriptorSets

				RenderDriver::Vulkan::CImageView* pColorImageView = nullptr;
				if(isImage(renderTargetAttachmentHandle))
				{
					// create new image view

					std::unique_ptr<RenderDriver::Vulkan::CImageView> pImageView = std::make_unique<RenderDriver::Vulkan::CImageView>();
					RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
					imageViewDesc.mFormat = pImage->getFormat();
					imageViewDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
					imageViewDesc.mpImage = pImage;
					imageViewDesc.mViewType = RenderDriver::Common::ResourceViewType::RenderTargetView;
					PLATFORM_OBJECT_HANDLE imageViewHandle = pImageView->create(imageViewDesc, *mpDevice);
					mapImageViews[imageViewHandle] = std::move(pImageView);
					pColorImageView = static_cast<RenderDriver::Vulkan::CImageView*>(mapImageViews[imageViewHandle].get());
				}
				else
				{
					auto const& renderTarget = this->getRenderTarget(renderTargetAttachmentHandle);
					pColorImageView = static_cast<RenderDriver::Vulkan::CImageView*>(renderTarget->getColorImageView());
				}
				
				VkImageView& imageView = pColorImageView->getNativeImageView();

				// image info 
				Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(mpRenderer);
				VkDescriptorImageInfo imageInfo;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; // VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = imageView;
				imageInfo.sampler = pRendererVulkan->getNativeLinearSampler();

				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo = renderJob.maShaderResourceInfo[iShaderResourceIndex];
				uint32_t iDescriptorSetIndex = (shaderResourceInfo.mShaderType == RenderDriver::Common::ShaderType::Fragment) ? 1 : 0;

				// descriptor set
				auto const& pipeline = maPipelines[renderJob.miPipelineIndex];
				RenderDriver::Vulkan::CDescriptorSet* pDescriptorSet = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(getDescriptorSet(pipeline.mDescriptorHandle).get());
				std::vector<VkDescriptorSet>& aNativeDescriptorSets = pDescriptorSet->getNativeDescriptorSets();
				WTFASSERT(aNativeDescriptorSets.size() > iDescriptorSetIndex, "Descriptor set index out of bounds: %d", iDescriptorSetIndex);

				// expected image layout when this descriptor set is being used
				pDescriptorSet->setImageLayout(
					iShaderResourceIndex, 
					RenderDriver::Common::ImageLayout::GENERAL);

				// get the binding index and set index from the layout info saved in the descriptor set
				uint32_t iBindingIndex = UINT32_MAX;
				auto const& aLayoutInfo = pDescriptorSet->getLayoutInfo();
				for(uint32_t i = 0; i < static_cast<uint32_t>(aLayoutInfo.size()); i++)
				{
					if(aLayoutInfo[i].miShaderResourceIndex == iShaderResourceIndex)
					{
						iDescriptorSetIndex = aLayoutInfo[i].miSetIndex;
						iBindingIndex = aLayoutInfo[i].miBindingIndex;
						break;
					}
				}

				if(iBindingIndex != UINT32_MAX)
				{
					VkWriteDescriptorSet writeDescriptorSet = {};
					writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSet.dstSet = aNativeDescriptorSets[iDescriptorSetIndex];
					writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					writeDescriptorSet.dstBinding = iBindingIndex;
					writeDescriptorSet.pImageInfo = &imageInfo;
					writeDescriptorSet.descriptorCount = 1;

					vkUpdateDescriptorSets(
						nativeDevice,
						1,
						&writeDescriptorSet,
						0,
						nullptr);
				}
			}
			else
			{
				WTFASSERT(0, "Invalid view type");
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
			WTFASSERT(0, "Implement me");
		}

		/*
		**
		*/
		void Serializer::associateReadOnlyBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
			WTFASSERT(0, "Get rid of this");
		}

		/*
		**
		*/
		void Serializer::associateReadWriteBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
			WTFASSERT(0, "Get rid of this");
		}

		/*
		**
		*/
		void Serializer::associateConstantBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo,
			std::string const& renderJobName)
		{
			WTFASSERT(0, "Get rid of this");
		}

		/*
		**
		*/
		void Serializer::associateRayTraceSceneBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
			WTFASSERT(0, "Implement me");
		}

		/*
		**
		*/
		void Serializer::associateExternalBufferToShaderResource(
			SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo)
		{
			WTFASSERT(0, "Implement me");
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
				RenderDriver::Common::CDescriptorHeap& VulkanDescriptorHeap = *getDescriptorHeap(renderJob.maDescriptorHeapHandles[i]).get();
				RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
				{
					/* .mFormat							*/	RenderDriver::Common::Format::R32_FLOAT,
					/* .mpBuffer						*/	pBuffer,
					/* .mpCounterBuffer = nullptr		*/  nullptr,
					/* .miSize							*/	iBufferSize,
					/* .miStructStrideSize = 0			*/	sizeof(float),
					/* .mFlags;							*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
					/* .mViewType						*/	RenderDriver::Common::ResourceViewType::ShaderResourceView,
					/* .mDescriptorHeap					*/	&VulkanDescriptorHeap,
					/* .miDescriptorOffset				*/	iShaderResourceIndex
				};

				RenderDriver::Vulkan::CBufferView bufferView;
				bufferView.create(bufferViewDesc, device);
			}
		}

		/*
		**
		*/
		void Serializer::platformCreateFrameBuffer(
			Render::Common::RenderJobInfo& renderJob)
		{
			if(renderJob.mPassType == Render::Common::PassType::SwapChain)
			{
				auto const& pipelineInfo = maPipelines[renderJob.miPipelineIndex];
				RenderDriver::Vulkan::CPipelineState* pPipelineState = static_cast<RenderDriver::Vulkan::CPipelineState*>(mapGraphicsPipelineStates[pipelineInfo.mPipelineStateHandle].get());
				VkRenderPass& nativeRenderPass = *(static_cast<VkRenderPass*>(pPipelineState->getNativeRenderPass()));
				VkDevice& nativeDevice = *(static_cast<VkDevice*>(mpDevice->getNativeDevice()));

				RenderDriver::Vulkan::CSwapChain* pSwapChain = static_cast<RenderDriver::Vulkan::CSwapChain*>(mpRenderer->getSwapChain());
				pSwapChain->createFrameBuffers(nativeRenderPass, nativeDevice);
			}
			else if(renderJob.mPassType == Render::Common::PassType::Imgui)
			{
				uint32_t iFrameBufferWidth = 0, iFrameBufferHeight = 0;
				std::vector<RenderDriver::Common::CRenderTarget*> apRenderTargets;
				for(uint32_t i = 0; i < static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size()); i++)
				{
					PLATFORM_OBJECT_HANDLE handle = renderJob.maOutputRenderTargetAttachments[i];
					std::unique_ptr<RenderDriver::Common::CRenderTarget>& renderTarget = getRenderTarget(handle);
					apRenderTargets.push_back(renderTarget.get());
				}

				std::unique_ptr<RenderDriver::Vulkan::CFrameBuffer> pFrameBufferVulkan =
					std::make_unique<RenderDriver::Vulkan::CFrameBuffer>();
;
				PLATFORM_OBJECT_HANDLE handle = mpDevice->assignHandleAndUpdate();
				pFrameBufferVulkan->setHandle(handle);

				uint32_t iNumValidRenderTargets = 0;
				RenderDriver::Vulkan::CImageView* pSelectedDepthImageView = nullptr;
				std::vector<VkImageView> aNativeImageViews;
				for(uint32_t i = 0; i < static_cast<uint32_t>(apRenderTargets.size()); i++)
				{
					if(apRenderTargets[i])
					{
						RenderDriver::Vulkan::CRenderTarget* pRenderTargetVulkan = static_cast<RenderDriver::Vulkan::CRenderTarget*>(apRenderTargets[i]);
						RenderDriver::Vulkan::CImageView* pColorImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pRenderTargetVulkan->getColorImageView());
						aNativeImageViews.push_back(pColorImageView->getNativeImageView());

						RenderDriver::Vulkan::CImageView* pDepthImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pRenderTargetVulkan->getDepthStencilImageView());
						if(pDepthImageView && pSelectedDepthImageView == nullptr)
						{
							pSelectedDepthImageView = pDepthImageView;
						}

						iFrameBufferWidth = pRenderTargetVulkan->getDescriptor().miWidth;
						iFrameBufferHeight = pRenderTargetVulkan->getDescriptor().miHeight;

						++iNumValidRenderTargets;
					}
				}

				// add depth 
				if(pSelectedDepthImageView)
				{
					aNativeImageViews.push_back(pSelectedDepthImageView->getNativeImageView());
				}

				VkFramebufferCreateInfo frameBufferCreateInfo = {};
				frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				frameBufferCreateInfo.pNext = nullptr;
				frameBufferCreateInfo.renderPass = static_cast<Render::Vulkan::CRenderer*>(mpRenderer)->getImguiRenderPass();
				frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(aNativeImageViews.size());
				frameBufferCreateInfo.pAttachments = aNativeImageViews.data();
				frameBufferCreateInfo.width = iFrameBufferWidth;
				frameBufferCreateInfo.height = iFrameBufferHeight;
				frameBufferCreateInfo.layers = 1;

				VkDevice* pNativeDevice = static_cast<VkDevice*>(mpDevice->getNativeDevice());

				VkFramebuffer nativeFrameBuffer;
				VkResult ret = vkCreateFramebuffer(
					*pNativeDevice,
					&frameBufferCreateInfo,
					nullptr,
					&nativeFrameBuffer);
				WTFASSERT(ret == VK_SUCCESS, "Error creating frame buffer: %d", ret);
				pFrameBufferVulkan->setNativeFrameBuffer(nativeFrameBuffer);
				mapFrameBuffers[handle] = std::move(pFrameBufferVulkan);

				static_cast<Render::Vulkan::CRenderer*>(mpRenderer)->setImguiFrameBufferHandle(handle);
			}
			else
			{
				if(renderJob.mType != Render::Common::JobType::Compute && renderJob.mType != Render::Common::JobType::Copy)
				{
					auto const& pipelineInfo = maPipelines[renderJob.miPipelineIndex];
					RenderDriver::Vulkan::CPipelineState* pPipelineState = static_cast<RenderDriver::Vulkan::CPipelineState*>(mapGraphicsPipelineStates[pipelineInfo.mPipelineStateHandle].get());
					if(pPipelineState == nullptr)
					{
						pPipelineState = static_cast<RenderDriver::Vulkan::CPipelineState*>(mapComputePipelineStates[pipelineInfo.mPipelineStateHandle].get());
					}
					WTFASSERT(pPipelineState, "can\'t find pipeline for render job: %s\n", renderJob.mName.c_str());

					uint32_t iFrameBufferWidth = 0, iFrameBufferHeight = 0;
					std::vector<RenderDriver::Common::CRenderTarget*> apRenderTargets;
					for(uint32_t i = 0; i < static_cast<uint32_t>(renderJob.maOutputRenderTargetAttachments.size()); i++)
					{
						PLATFORM_OBJECT_HANDLE handle = renderJob.maOutputRenderTargetAttachments[i];
						std::unique_ptr<RenderDriver::Common::CRenderTarget>& renderTarget = getRenderTarget(handle);
						apRenderTargets.push_back(renderTarget.get());
					}

					std::unique_ptr<RenderDriver::Vulkan::CFrameBuffer> pFrameBufferVulkan =
						std::make_unique<RenderDriver::Vulkan::CFrameBuffer>();

					RenderDriver::Common::FrameBufferDescriptor createFrameBufferDesc = {};
					createFrameBufferDesc.mpaRenderTargets = &apRenderTargets;
					createFrameBufferDesc.mpPipelineState = pPipelineState;
					PLATFORM_OBJECT_HANDLE frameBufferHandle = pFrameBufferVulkan->create(createFrameBufferDesc, *mpDevice);
					mapFrameBuffers[frameBufferHandle] = std::move(pFrameBufferVulkan);

					mapFrameBuffers[frameBufferHandle]->setID(renderJob.mName + " Frame Buffer");

					renderJob.mFrameBuffer = frameBufferHandle;
				}
			}
			
		}

		/*
		**
		*/
		void Serializer::platformUpdateSamplers(
			Render::Common::RenderJobInfo& renderJob)
		{
			if(renderJob.mPassType == Render::Common::PassType::Imgui)
			{
				return;
			}

			if(renderJob.maInputRenderTargetAttachments.size() <= 0 || renderJob.mType == Render::Common::JobType::Copy)
			{
				return;
			}

			Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(mpRenderer);

			// descriptor set to store the expected image layout 
			auto const& pipeline = maPipelines[renderJob.miPipelineIndex];
			RenderDriver::Vulkan::CDescriptorSet* pDescriptorSet = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(getDescriptorSet(pipeline.mDescriptorHandle).get());
			
			// input attachment to shader index mapping
			std::vector<uint32_t> aiShaderAttachmentMapping(renderJob.maiInputAttachmentMapping.size());
			for(uint32_t iInputAttachment = 0; iInputAttachment < static_cast<uint32_t>(renderJob.maiInputAttachmentMapping.size()); iInputAttachment++)
			{
				auto const& attachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iInputAttachment]];
				auto iter = std::find_if(
					renderJob.maShaderResourceInfo.begin(),
					renderJob.maShaderResourceInfo.end(),
					[&](SerializeUtils::Common::ShaderResourceInfo const& info)
					{
						return info.mName == attachmentInfo.mShaderResourceName;
					});
				uint32_t iShaderIndex = static_cast<uint32_t>(std::distance(renderJob.maShaderResourceInfo.begin(), iter));
				aiShaderAttachmentMapping[iInputAttachment] = iShaderIndex;
			}

			// get the descriptor image info
			std::vector<VkDescriptorImageInfo> aImageInfo;
			for(uint32_t iAttachment = 0; iAttachment < static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size()); iAttachment++)
			{
				PLATFORM_OBJECT_HANDLE renderTargetAttachmentHandle = renderJob.maInputRenderTargetAttachments[iAttachment];
				auto iter = std::find_if(
					mapRenderTargets.begin(),
					mapRenderTargets.end(),
					[&](auto const& keyValue)
					{
						return keyValue.first == renderTargetAttachmentHandle;
					});
				if(iter != mapRenderTargets.end())
				{
					RenderDriver::Common::CRenderTarget& renderTarget = *getRenderTarget(renderTargetAttachmentHandle).get();
					RenderDriver::Vulkan::CImageView* pColorImageView = static_cast<RenderDriver::Vulkan::CImageView*>(renderTarget.getColorImageView());
					VkImageView& imageView = pColorImageView->getNativeImageView();

					VkDescriptorImageInfo imageInfo;
					imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
					imageInfo.imageView = imageView;
					imageInfo.sampler = pRendererVulkan->getNativeLinearSampler();

					aImageInfo.push_back(imageInfo);

#if 0
					// shader resource index for attachment
					auto const& attachmentInfo = renderJob.maAttachmentInfo[renderJob.maiInputAttachmentMapping[iAttachment]];
					auto shaderIter = std::find_if(
						renderJob.maShaderResourceInfo.begin(),
						renderJob.maShaderResourceInfo.end(),
						[&](auto const& shaderResourceInfo)
						{
							return attachmentInfo.mShaderResourceName == shaderResourceInfo.mName;
						});
					// register expected layout for non-filler shader resource 
					if(shaderIter != renderJob.maShaderResourceInfo.end())
					{
						uint32_t iShaderResourceIndex = static_cast<uint32_t>(std::distance(renderJob.maShaderResourceInfo.begin(), shaderIter));

						// expected image layout when this descriptor set is being used
						pDescriptorSet->setImageLayout(
							iShaderResourceIndex,
							RenderDriver::Common::ImageLayout::GENERAL);
					}
#endif // #if 0

				}
			}

			if(aImageInfo.size() > 0 && renderJob.mType == Render::Common::JobType::Graphics)
			{
				uint32_t iDescriptorSetIndex = (renderJob.mType == Render::Common::JobType::Graphics) ? 1 : 0;

				auto const& pipeline = maPipelines[renderJob.miPipelineIndex];
				RenderDriver::Vulkan::CDescriptorSet* pDescriptorSet = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(getDescriptorSet(pipeline.mDescriptorHandle).get());
				std::vector<VkDescriptorSet>& aNativeDescriptorSets = pDescriptorSet->getNativeDescriptorSets();
				WTFASSERT(aNativeDescriptorSets.size() > iDescriptorSetIndex, "Descriptor set index out of bounds: %d", iDescriptorSetIndex);

				uint32_t iLastShaderResourceIndex = UINT32_MAX;
				{
					auto const& aLayoutInfo = pDescriptorSet->getLayoutInfo();
					for(uint32_t i = 0; i < static_cast<uint32_t>(aLayoutInfo.size()); i++)
					{
						if(aLayoutInfo[i].miSetIndex == iDescriptorSetIndex)
						{
							if(iLastShaderResourceIndex < aLayoutInfo[i].miBindingIndex || iLastShaderResourceIndex == UINT32_MAX)
							{
								iLastShaderResourceIndex = aLayoutInfo[i].miBindingIndex;
							}
						}
					}
				}

				VkDevice& nativeDevice = *(static_cast<VkDevice*>(mpDevice->getNativeDevice()));

				// last 2 samplers
				for(uint32_t i = 0; i < 2; i++)
				{
					VkWriteDescriptorSet writeDescriptorSet = {};
					writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					writeDescriptorSet.dstSet = aNativeDescriptorSets[iDescriptorSetIndex];
					writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
					writeDescriptorSet.dstBinding = iLastShaderResourceIndex + i + 1;
					writeDescriptorSet.pImageInfo = aImageInfo.data();
					writeDescriptorSet.descriptorCount = 1;

					vkUpdateDescriptorSets(
						nativeDevice,
						1,
						&writeDescriptorSet,
						0,
						nullptr);
				}
			}
		}

		/*
		**
		*/
		void Serializer::platformUpdateOutputAttachments(
			Render::Common::RenderJobInfo& renderJob)
		{
			if(renderJob.mPassType == Render::Common::PassType::Imgui)
			{
				return;
			}

			RenderDriver::Common::ImageLayout outputAttachmentImageLayout = RenderDriver::Common::ImageLayout::GENERAL;
			RenderDriver::Common::ImageLayout inputAttachmentImageLayout = RenderDriver::Common::ImageLayout::GENERAL;

			if(renderJob.mType == Render::Common::JobType::Graphics)
			{
				Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(mpRenderer);

				// pipeline state
				auto const& pipeline = maPipelines[renderJob.miPipelineIndex];
				RenderDriver::Vulkan::CDescriptorSet* pDescriptorSet = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(getDescriptorSet(pipeline.mDescriptorHandle).get());
				std::vector<VkDescriptorSet>& aNativeDescriptorSets = pDescriptorSet->getNativeDescriptorSets();

				// transition output attachments
				std::vector<RenderDriver::Vulkan::Utils::ImageLayoutTransition> aTransitions;
				std::vector<VkDescriptorImageInfo> aImageInfo;
				for(auto const& handle : renderJob.maOutputRenderTargetAttachments)
				{
					auto iter = std::find_if(
						mapRenderTargets.begin(),
						mapRenderTargets.end(),
						[&](auto const& keyPair)
						{
							return keyPair.first == handle;
						});

					if(iter != mapRenderTargets.end() && iter->second.get() != nullptr)
					{
						auto const& renderTarget = mapRenderTargets[handle];
						RenderDriver::Common::CImage* pColorImage = renderTarget->getImage().get();
						RenderDriver::Vulkan::CImage* pColorImageVulkan = static_cast<RenderDriver::Vulkan::CImage*>(pColorImage);
						RenderDriver::Common::ImageLayout imageLayout = pColorImageVulkan->getImageLayout(0);
						if(imageLayout == RenderDriver::Common::ImageLayout::UNDEFINED)
						{
							RenderDriver::Vulkan::CImageView* pColorImageView = static_cast<RenderDriver::Vulkan::CImageView*>(renderTarget->getColorImageView());

							VkImageView& imageView = pColorImageView->getNativeImageView();

							VkDescriptorImageInfo imageInfo;
							imageInfo.imageLayout = SerializeUtils::Vulkan::convert(outputAttachmentImageLayout);
							imageInfo.imageView = imageView;
							imageInfo.sampler = pRendererVulkan->getNativeLinearSampler();

							aImageInfo.push_back(imageInfo);

							RenderDriver::Vulkan::Utils::ImageLayoutTransition transition = {};
							transition.mpImage = pColorImage;
							transition.mBefore = imageLayout;
							transition.mAfter = outputAttachmentImageLayout;

							aTransitions.push_back(transition);
						}
					}
				}

				pRendererVulkan->transitionImageLayouts(aTransitions);
				aTransitions.clear();
				aImageInfo.clear();

				std::vector<RenderDriver::Vulkan::CDescriptorSet::DescriptorSetLayoutInfo> const& aLayoutInfo =
					pDescriptorSet->getLayoutInfo();

				// update input attachments
				std::vector<std::pair<uint32_t, uint32_t>> aBindingIndices;
				std::vector<VkDescriptorType> aDescriptorTypes;
				for(uint32_t iShaderResourceIndex = 0; iShaderResourceIndex < static_cast<uint32_t>(renderJob.maShaderResourceInfo.size()); iShaderResourceIndex++)
				{
					auto const& shaderResource = renderJob.maShaderResourceInfo[iShaderResourceIndex];
					if(shaderResource.mExternalResource.mpImage)
					{
						RenderDriver::Vulkan::CImageView* pImageView = nullptr;
						for(uint32_t j = 0; j < static_cast<uint32_t>(renderJob.maInputRenderTargetAttachments.size()); j++)
						{
							PLATFORM_OBJECT_HANDLE inputAttachmentHandle = renderJob.maInputRenderTargetAttachments[j];

							// get the render target to get the image view
							
							// validity check
							auto iter = std::find_if(
								mapRenderTargets.begin(),
								mapRenderTargets.end(),
								[&](auto const& keyPair)
								{
									return keyPair.first == inputAttachmentHandle;
								});

							if(iter != mapRenderTargets.end())
							{
								RenderDriver::Common::CRenderTarget* pRenderTarget = mapRenderTargets[renderJob.maInputRenderTargetAttachments[j]].get();
								if(pRenderTarget->getImage().get() == shaderResource.mExternalResource.mpImage)
								{
									pImageView = static_cast<RenderDriver::Vulkan::CImageView*>(pRenderTarget->getColorImageView());
									break;
								}
							}

						}	// for j = 0 to num input attachments

						if(pImageView)
						{
							RenderDriver::Vulkan::CImage* pImageVulkan = static_cast<RenderDriver::Vulkan::CImage*>(shaderResource.mExternalResource.mpImage);

							VkDescriptorImageInfo imageInfo;
							imageInfo.imageLayout = SerializeUtils::Vulkan::convert(inputAttachmentImageLayout);
							imageInfo.imageView = pImageView->getNativeImageView();
							imageInfo.sampler = pRendererVulkan->getNativeLinearSampler();
							aImageInfo.push_back(imageInfo);
						
							auto iter = std::find_if(
								aLayoutInfo.begin(),
								aLayoutInfo.end(),
								[&](auto const& layoutInfo)
								{
									return layoutInfo.miShaderResourceIndex == iShaderResourceIndex;
								});
							WTFASSERT(iter != aLayoutInfo.end(), "Can\'t find binding index for \"%s\"", pImageVulkan->getID().c_str());

							aBindingIndices.push_back(std::make_pair(iter->miSetIndex, iter->miBindingIndex));
							aDescriptorTypes.push_back(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

							RenderDriver::Vulkan::Utils::ImageLayoutTransition transition = {};
							transition.mpImage = pImageVulkan;
							transition.mBefore = pImageVulkan->getImageLayout(0);
							transition.mAfter = inputAttachmentImageLayout;

							aTransitions.push_back(transition);

							pDescriptorSet->setImageLayout(iShaderResourceIndex, inputAttachmentImageLayout);
						}

					}	// if valid image
				
				}	// for i = 0 to num shader resources

				if(aImageInfo.size() > 0)
				{
					pRendererVulkan->updateImageDescriptors(
						pDescriptorSet, 
						aImageInfo,
						aBindingIndices,
						aDescriptorTypes);
				}

				if(aTransitions.size() > 0)
				{
					pRendererVulkan->transitionImageLayouts(aTransitions);
				}

			}
			else if(renderJob.mType == Render::Common::JobType::Compute)
			{
				Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(mpRenderer);
				
				// pipeline state
				auto const& pipeline = maPipelines[renderJob.miPipelineIndex];
				RenderDriver::Vulkan::CDescriptorSet* pDescriptorSet = static_cast<RenderDriver::Vulkan::CDescriptorSet*>(getDescriptorSet(pipeline.mDescriptorHandle).get());
				std::vector<VkDescriptorSet>& aNativeDescriptorSets = pDescriptorSet->getNativeDescriptorSets();

				std::vector<RenderDriver::Vulkan::CDescriptorSet::DescriptorSetLayoutInfo> const& aLayoutInfo =
					pDescriptorSet->getLayoutInfo();

				// get output textures
				std::vector<RenderDriver::Vulkan::Utils::ImageLayoutTransition> aTransitions;
				std::vector<VkDescriptorImageInfo> aImageInfo;
				std::vector<uint32_t> aiBindingIndex;
				std::vector<RenderDriver::Common::CImage*> apOutputImages;
				std::vector<std::pair<uint32_t, uint32_t>> aBindingIndices;
				std::vector<VkDescriptorType> aDescriptorTypes;
				for(uint32_t iShaderResource = 0; iShaderResource < static_cast<uint32_t>(renderJob.maShaderResourceInfo.size()); iShaderResource++)
				{
					if(renderJob.maShaderResourceInfo[iShaderResource].mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT)
					{
						uint32_t iMapping = UINT32_MAX;
						auto iter = std::find_if(
							renderJob.maAttachmentInfo.begin(),
							renderJob.maAttachmentInfo.end(),
							[&](Render::Common::RenderJobAttachmentInfo const& attachmentInfo)
							{
								return (attachmentInfo.mShaderResourceName == renderJob.maShaderResourceInfo[iShaderResource].mName);
							});

						WTFASSERT(iter != renderJob.maAttachmentInfo.end(), "Can\'t find attachment for shader resource\"%s\"",
							renderJob.maShaderResourceInfo[iShaderResource].mName.c_str());
						iMapping = static_cast<uint32_t>(std::distance(renderJob.maAttachmentInfo.begin(), iter));

						PLATFORM_OBJECT_HANDLE renderTargetAttachmentHandle = renderJob.maOutputRenderTargetAttachments[iMapping];
						auto renderTargetIter = std::find_if(
							mapRenderTargets.begin(),
							mapRenderTargets.end(),
							[&](auto const& keyValue)
							{
								return keyValue.first == renderTargetAttachmentHandle;
							});
						if(renderTargetIter != mapRenderTargets.end())
						{
							RenderDriver::Common::CRenderTarget& renderTarget = *getRenderTarget(renderTargetAttachmentHandle).get();
							RenderDriver::Vulkan::CImageView* pColorImageView = static_cast<RenderDriver::Vulkan::CImageView*>(renderTarget.getColorImageView());
							RenderDriver::Common::CImage* pImage = renderTarget.getImage().get();
							RenderDriver::Vulkan::CImage* pImageVulkan = static_cast<RenderDriver::Vulkan::CImage*>(pImage);

							VkImageView& imageView = pColorImageView->getNativeImageView();

							VkDescriptorImageInfo imageInfo;
							imageInfo.imageLayout = SerializeUtils::Vulkan::convert(outputAttachmentImageLayout);
							imageInfo.imageView = imageView;
							imageInfo.sampler = pRendererVulkan->getNativeLinearSampler();

							aImageInfo.push_back(imageInfo);

							aiBindingIndex.push_back(iShaderResource);
							apOutputImages.push_back(pImage);

							auto iter = std::find_if(
								aLayoutInfo.begin(),
								aLayoutInfo.end(),
								[&](auto const& layoutInfo)
								{
									return layoutInfo.miShaderResourceIndex == iShaderResource;
								});
							WTFASSERT(iter != aLayoutInfo.end(), "Can\'t find binding index for \"%s\"", pImageVulkan->getID().c_str());

							aBindingIndices.push_back(std::make_pair(iter->miSetIndex, iter->miBindingIndex));
							VkDescriptorType descriptorType = (iter->mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_OUT) ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
							aDescriptorTypes.push_back(descriptorType);

							// expected image layout when this descriptor set is being used
							pDescriptorSet->setImageLayout(
								iShaderResource,
								outputAttachmentImageLayout);

							RenderDriver::Vulkan::Utils::ImageLayoutTransition transition = {};
							transition.mpImage = pImage;
							transition.mBefore = pImageVulkan->getImageLayout(0);
							transition.mAfter = outputAttachmentImageLayout;
							aTransitions.push_back(transition);
						}
					}
				}
					
				// update the output texture descriptors
				pRendererVulkan->updateImageDescriptors(
					pDescriptorSet,
					aImageInfo,
					aBindingIndices,
					aDescriptorTypes);
				aImageInfo.clear();

				// transition image layouts
				pRendererVulkan->transitionImageLayouts(aTransitions);
				aTransitions.clear();

				// get input textures to transition from undefined
				for(uint32_t iShaderResource = 0; iShaderResource < static_cast<uint32_t>(renderJob.maShaderResourceInfo.size()); iShaderResource++)
				{
					if(renderJob.maShaderResourceInfo[iShaderResource].mType == ShaderResourceType::RESOURCE_TYPE_TEXTURE_IN)
					{
						if(renderJob.maShaderResourceInfo[iShaderResource].mName.find("filler") != std::string::npos)
						{
							continue;
						}

						// image
						RenderDriver::Common::CImage* pImage = renderJob.maShaderResourceInfo[iShaderResource].mExternalResource.mpImage;
						RenderDriver::Vulkan::CImage* pImageVulkan = static_cast<RenderDriver::Vulkan::CImage*>(pImage);

						// expected image layout at the time of descriptor set usage
						pDescriptorSet->setImageLayout(iShaderResource, inputAttachmentImageLayout);

						RenderDriver::Vulkan::Utils::ImageLayoutTransition transition = {};
						transition.mpImage = pImage;
						transition.mBefore = pImageVulkan->getImageLayout(0);
						transition.mAfter = inputAttachmentImageLayout;
						aTransitions.push_back(transition);
						
					}	// if input attachment 

				}	// for shader resource

				pRendererVulkan->transitionImageLayouts(aTransitions);

			}
			else if(renderJob.mType == Render::Common::JobType::Copy)
			{
				Render::Vulkan::CRenderer* pRendererVulkan = static_cast<Render::Vulkan::CRenderer*>(mpRenderer);

				// transition to shader readable state 
				std::vector<RenderDriver::Vulkan::Utils::ImageLayoutTransition> aTransitions;
				for(uint32_t iAttachment = 0; iAttachment < static_cast<uint32_t>(renderJob.maAttachmentInfo.size()); iAttachment++)
				{
					if(renderJob.maAttachmentInfo[iAttachment].mType == Render::Common::AttachmentType::TextureOut)
					{
						// render target
						PLATFORM_OBJECT_HANDLE renderTargetAttachmentHandle = renderJob.maAttachmentInfo[iAttachment].mAttachmentHandle;
						auto renderTargetIter = std::find_if(
							mapRenderTargets.begin(),
							mapRenderTargets.end(),
							[&](auto const& keyValue)
							{
								return keyValue.first == renderTargetAttachmentHandle;
							});

						if(renderTargetIter == mapRenderTargets.end())
						{
							// image
							RenderDriver::Common::CImage* pImage = getImage(renderTargetAttachmentHandle).get();
							WTFASSERT(pImage, "no image for handle %d", renderTargetAttachmentHandle);
							RenderDriver::Vulkan::CImage* pImageVulkan = static_cast<RenderDriver::Vulkan::CImage*>(pImage);

							RenderDriver::Vulkan::Utils::ImageLayoutTransition transition = {};
							transition.mpImage = pImage;
							transition.mBefore = pImageVulkan->getImageLayout(0);
							transition.mAfter = outputAttachmentImageLayout;
							aTransitions.push_back(transition);
						}
						else 
						{
							RenderDriver::Common::CRenderTarget& renderTarget = *getRenderTarget(renderTargetAttachmentHandle).get();
							RenderDriver::Common::CImage* pImage = renderTarget.getImage().get();
							WTFASSERT(pImage, "no image for handle %d", renderTargetAttachmentHandle);
							RenderDriver::Vulkan::CImage* pImageVulkan = static_cast<RenderDriver::Vulkan::CImage*>(pImage);

							RenderDriver::Vulkan::Utils::ImageLayoutTransition transition = {};
							transition.mpImage = pImage;
							transition.mBefore = pImageVulkan->getImageLayout(0);
							transition.mAfter = outputAttachmentImageLayout;
							aTransitions.push_back(transition);
						}
					
					}	// if texture out

				}	// for shader resource
			
				pRendererVulkan->transitionImageLayouts(aTransitions);

			}	// if render job == copy
		}

	}	// Vulkan

}	// RenderDriver