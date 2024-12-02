#include "RenderJobVulkan.h"
#include <render-driver/Vulkan/CommandAllocatorVulkan.h>
#include <render-driver/Vulkan/CommandBufferVulkan.h>
#include <render-driver/Vulkan/FrameBufferVulkan.h>
#include <render-driver/Vulkan/PipelineStateVulkan.h>
#include <render-driver/Vulkan/SwapChainVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>

#include <iostream>
#include <stdio.h>
#include <LogPrint.h>
#include <wtfassert.h>

namespace Render
{
    namespace Vulkan
    {
		CRenderJob::CRenderJob() {}
		CRenderJob::~CRenderJob() {}

		/*
		**
		*/
		void CRenderJob::platformCreateAttachmentImage(
			std::string const& name,
			uint32_t iWidth,
			uint32_t iHeight,
			RenderDriver::Common::Format const& format
		)
		{
			maOutputImageAttachments[name] = std::make_unique<RenderDriver::Vulkan::CImage>();
			maOutputImageAttachmentViews[name] = std::make_unique<RenderDriver::Vulkan::CImageView>();

			RenderDriver::Vulkan::CImage* pImage = maOutputImageAttachments[name].get();
			RenderDriver::Vulkan::CImageView* pImageView = maOutputImageAttachmentViews[name].get();

			float afDefaultClearColor[4] = {0.0f, 0.0f, 0.3f, 0.0f};
			RenderDriver::Common::ImageDescriptor desc = {};
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(format);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(format);
			desc.miWidth = iWidth;
			desc.miHeight = iHeight;
			desc.miNumImages = 1;
			desc.mFormat = format;
			desc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess;
			desc.mafClearColor = afDefaultClearColor;
			pImage->create(
				desc, 
				*mpDevice
			);
			pImage->setID(name);
			pImage->getDescriptor().miWidth = iWidth;
			pImage->getDescriptor().miHeight = iHeight;
			pImage->getDescriptor().miNumImages = 1;
			pImage->getDescriptor().mFormat = desc.mFormat;
			pImage->setID(name + " Image");
			pImage->setImageLayout(RenderDriver::Common::ImageLayout::UNDEFINED, 0);
			pImage->setImageLayout(RenderDriver::Common::ImageLayout::UNDEFINED, 1);
			pImage->setInitialImageLayout(RenderDriver::Common::ImageLayout::COLOR_ATTACHMENT_OPTIMAL, 0);
			pImage->setInitialImageLayout(RenderDriver::Common::ImageLayout::COLOR_ATTACHMENT_OPTIMAL, 1);
			mapImageAttachments[name] = pImage;
			
			RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
			imageViewDesc.mFormat = desc.mFormat;
			imageViewDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
			imageViewDesc.mpImage = pImage;
			imageViewDesc.mViewType = RenderDriver::Common::ResourceViewType::RenderTargetView;
			pImageView->create(imageViewDesc, *mpDevice);
			pImageView->setID(name + " Image View");
			mapImageAttachmentViews[name] = pImageView;
			
		}

		/*
		**
		*/
		void CRenderJob::platformCreateAttachmentBuffer(
			std::string const& name,
			uint32_t iBufferSize,
			RenderDriver::Common::Format const& format,
			RenderDriver::Common::BufferUsage const& usage
		)
		{
			maOutputBufferAttachments[name] = std::make_unique<RenderDriver::Vulkan::CBuffer>();
			maOutputBufferAttachmentViews[name] = std::make_unique<RenderDriver::Vulkan::CBufferView>();
		
			RenderDriver::Vulkan::CBuffer* pBuffer = maOutputBufferAttachments[name].get();
			RenderDriver::Vulkan::CBufferView* pBufferView = maOutputBufferAttachmentViews[name].get();

			RenderDriver::Common::BufferDescriptor desc = {};
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(format);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(format);

			// data size
			desc.miSize = iBufferSize;
			
			// usage
			desc.mBufferUsage = usage;

			// format and writeable flag
			desc.mFormat = format;
			uint32_t iUsage = (uint32_t)usage;
			uint32_t iStorageBuffer = (uint32_t)RenderDriver::Common::BufferUsage::StorageBuffer;
			uint32_t iStorageTexel = (uint32_t)RenderDriver::Common::BufferUsage::StorageTexel;
			if((iUsage & iStorageBuffer) > 0 || (iUsage & iStorageTexel) > 0)
			{
				desc.mFlags = RenderDriver::Common::ResourceFlagBits::AllowUnorderedAccess;
			}

			// create and register to global buffer dictionary
			pBuffer->create(desc, *mpDevice);
			pBuffer->setID(name);
			mapBufferAttachments[name] = pBuffer;

			RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
			{
				/* .mFormat				*/  format,
				/* .mpBuffer			*/  pBuffer,
				/* .mpCounterBuffer		*/  nullptr,
				/* .miSize				*/  iBufferSize,
				/* .miStructStrideSize	*/  1,
				/* .mFlags				*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
				/* .mViewType			*/  RenderDriver::Common::ResourceViewType::ShaderResourceView,
				/* .mpDescriptorHeap	*/  nullptr,
				/* .miDescriptorOffset	*/  0,
				/* .mpDescriptorSet		*/  nullptr,
			};

			pBufferView->create(bufferViewDesc, *mpDevice);
			mapBufferAttachmentViews[name] = pBufferView;
		}

		/*
		**
		*/
		RenderDriver::Common::CBuffer* CRenderJob::platformCreateBuffer(
			std::string const& name,
			RenderDriver::Common::BufferDescriptor const& desc
		)
		{
			mapBuffers[name] = std::make_unique<RenderDriver::Vulkan::CBuffer>();
			mapBufferViews[name] = std::make_unique<RenderDriver::Vulkan::CBufferView>();

			RenderDriver::Vulkan::CBuffer* pBuffer = mapBuffers[name].get();
			RenderDriver::Vulkan::CBufferView* pBufferView = mapBufferViews[name].get();

			pBuffer->create(desc, *mpDevice);
			pBuffer->setID(name);

			RenderDriver::Common::BufferViewDescriptor bufferViewDesc =
			{
				/* .mFormat				*/  desc.mFormat,
				/* .mpBuffer			*/  pBuffer,
				/* .mpCounterBuffer		*/  nullptr,
				/* .miSize				*/  (uint32_t)desc.miSize,
				/* .miStructStrideSize	*/  1,
				/* .mFlags				*/  RenderDriver::Common::UnorderedAccessViewFlags::None,
				/* .mViewType			*/  RenderDriver::Common::ResourceViewType::ShaderResourceView,
				/* .mpDescriptorHeap	*/  nullptr,
				/* .miDescriptorOffset	*/  0,
				/* .mpDescriptorSet		*/  nullptr,
			};

			pBufferView->create(bufferViewDesc, *mpDevice);

			return pBuffer;
		}

		/*
		**
		*/
		RenderDriver::Common::CImage* CRenderJob::platformCreateImage(
			std::string const& name,
			RenderDriver::Common::ImageDescriptor const& desc
		)
		{
			mapImages[name] = std::make_unique<RenderDriver::Vulkan::CImage>();
			mapImageViews[name] = std::make_unique<RenderDriver::Vulkan::CImageView>();

			RenderDriver::Vulkan::CImage* pImage = maOutputImageAttachments[name].get();
			mapImages[name]->create(desc, *mpDevice);

			RenderDriver::Common::ImageViewDescriptor imageViewDesc = {};
			imageViewDesc.mFormat = desc.mFormat;
			imageViewDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
			imageViewDesc.mpImage = pImage;
			imageViewDesc.mViewType = RenderDriver::Common::ResourceViewType::RenderTargetView;
			mapImageViews[name]->create(imageViewDesc, *mpDevice);

			return pImage;
		}

		/*
		**
		*/
		RenderDriver::Common::CImage* CRenderJob::platformCreateImageWithData(
			std::string const& name,
			unsigned char const* pImageData,
			uint32_t iTextureWidth,
			uint32_t iTextureHeight,
			RenderDriver::Common::Format const& format
		)
		{
			mapImages[name] = std::make_unique<RenderDriver::Vulkan::CImage>();
			mapImageViews[name] = std::make_unique<RenderDriver::Vulkan::CImageView>();

			RenderDriver::Vulkan::CImage* pImage = mapImages[name].get();

			float afDefaultClearColor[4] = {0.0f, 0.0f, 0.3f, 0.0f};
			RenderDriver::Common::ImageDescriptor desc = {};
			uint32_t iNumChannels = SerializeUtils::Common::getBaseComponentSize(format);
			uint32_t iChannelSize = SerializeUtils::Common::getNumComponents(format);
			desc.miWidth = iTextureWidth;
			desc.miHeight = iTextureHeight;
			desc.miNumImages = 1;
			desc.mFormat = format;
			desc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess;
			desc.mafClearColor = afDefaultClearColor;
			pImage->create(desc, *mpDevice);
			pImage->setID(name);

			// upload command buffer allocator
			RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc = {};
			commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
			RenderDriver::Vulkan::CCommandAllocator commandAllocator;
			commandAllocator.create(commandAllocatorDesc, *mpDevice);
			
			// upload command buffer
			RenderDriver::Common::CommandBufferDescriptor commandBufferDesc = {};
			commandBufferDesc.mpCommandAllocator = &commandAllocator;
			commandBufferDesc.mpPipelineState = nullptr;
			commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
			RenderDriver::Vulkan::CCommandBuffer commandBuffer;
			commandBuffer.create(commandBufferDesc, *mpDevice);
			commandBuffer.reset();

			// upload buffer 
			RenderDriver::Vulkan::CBuffer uploadBuffer;
			uint32_t iSize = iTextureWidth * iTextureHeight * 4;
			RenderDriver::Common::BufferDescriptor bufferDesc;
			bufferDesc.miSize = iSize;
			bufferDesc.mFormat = format;
			uploadBuffer.create(bufferDesc, *mpDevice);

			// set image data
			uploadBuffer.setData(
				(void *)pImageData,
				iSize);

			// copy to image
			uploadBuffer.copyToImage(
				uploadBuffer,
				*pImage,
				commandBuffer,
				iTextureWidth,
				iTextureHeight
			);

			// execute command 
			mpCommandQueue->execCommandBuffer(
				commandBuffer,
				*mpDevice
			);

			return pImage;
		}

		/*
		**
		*/
		void CRenderJob::platformInitDescriptorSet()
		{
			mDescriptorSet = std::make_unique<RenderDriver::Vulkan::CDescriptorSet>();
			RenderDriver::Common::DescriptorSetDescriptor desc = {};
			desc.mpaShaderResources = nullptr;

			desc.mPipelineType = PipelineType::GRAPHICS_PIPELINE_TYPE;
			if(mType == Render::Common::JobType::Compute)
				desc.mPipelineType = PipelineType::COMPUTE_PIPELINE_TYPE;
			else if(mType == Render::Common::JobType::Copy)
				desc.mPipelineType = PipelineType::COPY_PIPELINE_TYPE;

			mDescriptorSet->create(desc, *mpDevice);
			mpDescriptorSet = mDescriptorSet.get();
		}

		/*
		**
		*/
		void CRenderJob::platformInitPipelineState()
		{
			mPipelineState = std::make_unique<RenderDriver::Vulkan::CPipelineState>();
			mpPipelineState = mPipelineState.get();

			mSignalFence = std::make_unique<RenderDriver::Vulkan::CFence>();
			
			RenderDriver::Common::FenceDescriptor desc = {};
			mSignalFence->create(
				desc, 
				*mpDevice);
			mpSignalFence = mSignalFence.get();
		}

		/*
		**
		*/
		RenderDriver::Common::GraphicsPipelineStateDescriptor* CRenderJob::platformInitGraphicsPipelineStateDescriptor()
		{
			mGraphicsPipelineStateDesc = std::make_unique<RenderDriver::Vulkan::CPipelineState::GraphicsPipelineStateDescriptor>();
			return mGraphicsPipelineStateDesc.get();
		}

		/*
		**
		*/
		RenderDriver::Common::ComputePipelineStateDescriptor* CRenderJob::platformInitComputePipelineStateDescriptor()
		{
			mComputePipelineStateDesc = std::make_unique<RenderDriver::Vulkan::CPipelineState::ComputePipelineStateDescriptor>();
			return mComputePipelineStateDesc.get();
		}

		/*
		**
		*/
		RenderDriver::Common::RayTracePipelineStateDescriptor* CRenderJob::platformInitRayTracePipelineStateDescriptor()
		{
			mRayTracePipelineStateDesc = std::make_unique<RenderDriver::Vulkan::CPipelineState::RayTracePipelineStateDescriptor>();
			return mRayTracePipelineStateDesc.get();
		}

		/*
		**
		*/
		RenderDriver::Common::GraphicsPipelineStateDescriptor* CRenderJob::platformFillOutGraphicsPipelineDescriptor(
			RenderDriver::Common::GraphicsPipelineStateDescriptor* pDesc,
			std::vector<char>& acShaderBufferVS,
			std::vector<char>& acShaderBufferFS,
			std::string const& shaderPath
		)
		{
			RenderDriver::Vulkan::CPipelineState::GraphicsPipelineStateDescriptor* pVulkanDesc = (RenderDriver::Vulkan::CPipelineState::GraphicsPipelineStateDescriptor*)pDesc;

			// vulkan version
			pVulkanDesc->miImageWidth = 0;
			pVulkanDesc->miImageHeight = 0;
			pVulkanDesc->miNumRootConstants = miNumRootConstants;
			memcpy(pVulkanDesc->mafRootConstants, mafRootConstants.data(), mafRootConstants.size() * sizeof(float));

			// get shaders
			auto fileNameEnd = shaderPath.find_last_of(".");
			auto directoryEnd = shaderPath.find_last_of("\\");
			std::string directory = shaderPath.substr(0, directoryEnd);
			std::string outputDirectory = directory + "-output";
			std::string fileName = shaderPath.substr(directoryEnd + 1, (fileNameEnd - (directoryEnd + 1)));
			std::string outputFilePath = outputDirectory + "\\" + fileName + "-vs.spirv";

			// compile vertex shader
			std::string mainFunctionName = "VSMain";
			std::string compileCommand = "D:\\VulkanSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				outputFilePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			int iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}

			FILE* fp = fopen(outputFilePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acShaderBufferVS.resize(iFileSize);
			fread(acShaderBufferVS.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			outputFilePath = outputDirectory + "\\" + fileName + "-fs.spirv";

			// compile fragment shader
			mainFunctionName = "PSMain";
			compileCommand = "D:\\VulkanSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				outputFilePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}

			fp = fopen(outputFilePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acShaderBufferFS.resize(iFileSize);
			fread(acShaderBufferFS.data(), sizeof(char), iFileSize, fp);
			fclose(fp);
			

			return pVulkanDesc;
		}

		/*
		**
		*/
		RenderDriver::Common::ComputePipelineStateDescriptor* CRenderJob::platformFillOutComputePipelineDescriptor(
			RenderDriver::Common::ComputePipelineStateDescriptor* pDesc,
			std::vector<char>& acShaderBuffer,
			std::string const& shaderPath
		)
		{
			RenderDriver::Vulkan::CPipelineState::ComputePipelineStateDescriptor* pVulkanDesc = (RenderDriver::Vulkan::CPipelineState::ComputePipelineStateDescriptor*)pDesc;

			// vulkan version
			pVulkanDesc->miNumRootConstants = miNumRootConstants;
			memcpy(pVulkanDesc->mafRootConstants, mafRootConstants.data(), mafRootConstants.size() * sizeof(float));

			// get shaders
			auto fileNameEnd = shaderPath.find_last_of(".");
			auto directoryEnd = shaderPath.find_last_of("\\");
			std::string directory = shaderPath.substr(0, directoryEnd);
			std::string outputDirectory = directory + "-output";
			std::string fileName = shaderPath.substr(directoryEnd + 1, (fileNameEnd - (directoryEnd + 1)));
			std::string filePath = outputDirectory + "\\" + fileName + "-vs.spirv";

			// compile compute shader
			std::string mainFunctionName = "CSMain";
			std::string compileCommand = "D:\\VulkanSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				filePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			int iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}

			FILE* fp = fopen(filePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acShaderBuffer.resize(iFileSize);
			fread(acShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);

			pDesc->miFlags = 0;
			pDesc->miComputeShaderSize = (uint32_t)iFileSize;
			pDesc->mpComputeShader = (uint8_t*)acShaderBuffer.data();
			pDesc->mpDescriptor = mpDescriptorSet;

			return pVulkanDesc;
		}

		/*
		**
		*/
		RenderDriver::Common::RayTracePipelineStateDescriptor* CRenderJob::platformFillOutRayTracePipelineDescriptor(
			RenderDriver::Common::RayTracePipelineStateDescriptor* pDesc,
			std::vector<char>& acRayGenShaderBuffer,
			std::vector<char>& acCloseHitShaderBuffer,
			std::vector<char>& acMissShaderBuffer,
			std::string const& shaderPath,
			void* pPlatformInstance
		)
		{
			RenderDriver::Vulkan::CPipelineState::RayTracePipelineStateDescriptor* pVulkanDesc = (RenderDriver::Vulkan::CPipelineState::RayTracePipelineStateDescriptor*)pDesc;
			pVulkanDesc->mpPlatformInstance = pPlatformInstance;

			// get shaders
			auto fileNameEnd = shaderPath.find_last_of(".");
			auto directoryEnd = shaderPath.find_last_of("\\");
			std::string directory = shaderPath.substr(0, directoryEnd);
			std::string outputDirectory = directory + "-output";
			std::string fileName = shaderPath.substr(directoryEnd + 1, (fileNameEnd - (directoryEnd + 1)));
			std::string filePath = outputDirectory + "\\" + fileName + "-raygen.spirv";

			// compile compute shader
			// ray gen
			std::string mainFunctionName = "rayGen";
			std::string compileCommand = "D:\\VulkanSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				filePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			int iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}

			FILE* fp = fopen(filePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			size_t iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acRayGenShaderBuffer.resize(iFileSize);
			fread(acRayGenShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);
			pVulkanDesc->mpRayGenShader = (uint8_t*)acRayGenShaderBuffer.data();
			pVulkanDesc->miRayGenShaderSize = (uint32_t)acRayGenShaderBuffer.size();

			// close hit
			filePath = outputDirectory + "\\" + fileName + "-closehit.spirv";
			mainFunctionName = "hitTriangle";
			compileCommand = "D:\\VulkanSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				filePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}
			fp = fopen(filePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acCloseHitShaderBuffer.resize(iFileSize);
			fread(acCloseHitShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);
			pVulkanDesc->mpCloseHitShader = (uint8_t*)acCloseHitShaderBuffer.data();
			pVulkanDesc->miHitShaderSize = (uint32_t)acCloseHitShaderBuffer.size();

			// miss
			filePath = outputDirectory + "\\" + fileName + "-miss.spirv";
			mainFunctionName = "missShader";
			compileCommand = "D:\\VulkanSDK\\1.3.296.0\\Bin\\slangc.exe -profile glsl_450 -target spirv " +
				directory + "\\" + fileName + ".slang " +
				"-fvk-use-entrypoint-name -entry " +
				mainFunctionName +
				" -o " +
				filePath +
				" -g";
			DEBUG_PRINTF("%s\n", compileCommand.c_str());
			iRet = system(compileCommand.c_str());
			if(iRet != 0)
			{
				WTFASSERT(0, "Error compiling \"%s\"", fileName.c_str());
			}
			fp = fopen(filePath.c_str(), "rb");
			fseek(fp, 0, SEEK_END);
			iFileSize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			acMissShaderBuffer.resize(iFileSize);
			fread(acMissShaderBuffer.data(), sizeof(char), iFileSize, fp);
			fclose(fp);
			pVulkanDesc->mpMissShader = (uint8_t*)acMissShaderBuffer.data();
			pVulkanDesc->miMissShaderSize = (uint32_t)acMissShaderBuffer.size();

			return pVulkanDesc;
		}

		/*
		**
		*/
		void CRenderJob::platformCreateDepthImage(
			uint32_t iWidth,
			uint32_t iHeight
		)
		{
			mDepthImage = std::make_unique<RenderDriver::Vulkan::CImage>();
			
			RenderDriver::Common::Format format = RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT;

			float afDefaultClearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
			RenderDriver::Common::ImageDescriptor desc = {};
			desc.miWidth = iWidth;
			desc.miHeight = iHeight;
			desc.miNumImages = 1;
			desc.mFormat = format;
			desc.mResourceFlags = RenderDriver::Common::ResourceFlagBits::AllowSimultaneousAccess;
			desc.mafClearColor = afDefaultClearColor;
			mDepthImage->create(
				desc,
				*mpDevice);
			mDepthImage->setImageLayout(RenderDriver::Common::ImageLayout::UNDEFINED, 0);
			mDepthImage->setImageLayout(RenderDriver::Common::ImageLayout::UNDEFINED, 1);
			mDepthImage->setInitialImageLayout(RenderDriver::Common::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 0);
			mDepthImage->setInitialImageLayout(RenderDriver::Common::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
			mDepthImage->setID(mName + "-" + "Depth Output");
			mpDepthImage = mDepthImage.get();

			mDepthImageView = std::make_unique<RenderDriver::Vulkan::CImageView>();

			RenderDriver::Common::ImageViewDescriptor viewDesc = {};
			viewDesc.mViewType = RenderDriver::Common::ResourceViewType::RenderTargetView;
			viewDesc.mFormat = format;
			viewDesc.mDimension = RenderDriver::Common::Dimension::Texture2D;
			viewDesc.mpImage = mpDepthImage;
			mDepthImageView->create(
				viewDesc, 
				*mpDevice);
			mpDepthImageView = mDepthImageView.get();
			
		}

		/*
		**
		*/
		void CRenderJob::platformCreateFrameBuffer(
			RenderDriver::Common::CSwapChain* pSwapChain
		)
		{
			mFrameBuffer = std::make_unique<RenderDriver::Vulkan::CFrameBuffer>();
			
			if(mPassType == Render::Common::PassType::SwapChain)
			{
				RenderDriver::Vulkan::CSwapChain* pSwapChainVulkan = (RenderDriver::Vulkan::CSwapChain*)pSwapChain;
				RenderDriver::Vulkan::CPipelineState* pPipelineStateVulkan = (RenderDriver::Vulkan::CPipelineState*)mpPipelineState;
				VkDevice& nativeDevice = *(static_cast<VkDevice*>(mpDevice->getNativeDevice()));
				VkRenderPass& nativeRenderPass = *((VkRenderPass *)pPipelineStateVulkan->getNativeRenderPass());
				pSwapChainVulkan->createFrameBuffers(nativeRenderPass, nativeDevice);
			}
			else
			{
				RenderDriver::Common::FrameBufferDescriptor2 desc;
				desc.mDepthView = (mpDepthImageView != nullptr) ?
					std::make_pair(mpDepthImageView, (uint32_t)mapOutputImageAttachments.size() - 1) :
					std::make_pair(nullptr, UINT32_MAX);

				// attachment views for the output render targets used in the frame buffer
				std::vector<RenderDriver::Common::CImageView*> apOutputImageAttachmentViews;
				uint32_t iImageWidth = 0, iImageHeight = 0;
				for(auto const& keyValue : maAttachmentMappings)
				{
					if(keyValue.second == "texture-output")
					{
						auto& attachmentView = mapImageAttachmentViews[keyValue.first];
						apOutputImageAttachmentViews.push_back(attachmentView);
						if(iImageWidth == 0)
						{
							iImageWidth = mapOutputImageAttachments[keyValue.first]->getDescriptor().miWidth;
							iImageHeight = mapOutputImageAttachments[keyValue.first]->getDescriptor().miHeight;
						}
					}
					else if(keyValue.second == "texture-input-output")
					{
						if(keyValue.first == "Depth Output")
						{
							//apOutputImageAttachmentViews.push_back(mpDepthImageView);
						}
						else 
						{
							auto& attachmentView = mapImageAttachmentViews[keyValue.first];
							apOutputImageAttachmentViews.push_back(attachmentView);
						}
						if(iImageWidth == 0)
						{
							iImageWidth = mapImageAttachments[keyValue.first]->getDescriptor().miWidth;
							iImageHeight = mapImageAttachments[keyValue.first]->getDescriptor().miHeight;
						}
					}
				}
				desc.mpaImageView = &apOutputImageAttachmentViews;
				desc.mpPipelineState = mpPipelineState;
				desc.miWidth = iImageWidth;
				desc.miHeight = iImageHeight;

				mFrameBuffer->create2(
					desc,
					*mpDevice
				);

				mpFrameBuffer = mFrameBuffer.get();
			}
		}

		/*
		**
		*/
		void CRenderJob::platformCreateSemaphore()
		{
			mSignalFence = std::make_unique<RenderDriver::Vulkan::CFence>();

			RenderDriver::Common::FenceDescriptor desc = {};
			mSignalFence->create(
				desc,
				*mpDevice);
			mpSignalFence = mSignalFence.get();
		}

    }	// Vulkan

}	// Render 