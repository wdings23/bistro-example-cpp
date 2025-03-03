#pragma once

#include <render-driver/Vulkan/BufferVulkan.h>
#include <render-driver/Vulkan/BufferViewVulkan.h>
#include <render-driver/Vulkan/ImageVulkan.h>
#include <render-driver/Vulkan/ImageViewVulkan.h>
#include <render-driver/Vulkan/DescriptorSetVulkan.h>
#include <render-driver/Vulkan/PipelineStateVulkan.h>
#include <render-driver/Vulkan/FrameBufferVulkan.h>
#include <render-driver/Vulkan/FenceVulkan.h>

#include <render/RenderJob.h>

#include <vector>

namespace Render
{
	namespace Vulkan
	{
		class CRenderJob : public Render::Common::CRenderJob
		{
		public:

			CRenderJob();
			virtual ~CRenderJob();


		protected:
			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CImage>>				maOutputImageAttachments;
			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CImageView>>			maOutputImageAttachmentViews;

			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CBuffer>>				maOutputBufferAttachments;
			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CBufferView>>			maOutputBufferAttachmentViews;

			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CBuffer>>				mapBuffers;
			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CBufferView>>			mapBufferViews;

			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CImage>>				mapImages;
			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CImageView>>			mapImageViews;

			std::map<std::string, std::unique_ptr<RenderDriver::Vulkan::CImageView>>			maUniformImageViews;

			std::unique_ptr<RenderDriver::Vulkan::CDescriptorSet>								mDescriptorSet;
			std::unique_ptr<RenderDriver::Vulkan::CPipelineState>								mPipelineState;

			std::unique_ptr<RenderDriver::Vulkan::CPipelineState::GraphicsPipelineStateDescriptor>		mGraphicsPipelineStateDesc;
			std::unique_ptr<RenderDriver::Vulkan::CPipelineState::ComputePipelineStateDescriptor>		mComputePipelineStateDesc;
			std::unique_ptr<RenderDriver::Vulkan::CPipelineState::RayTracePipelineStateDescriptor>		mRayTracePipelineStateDesc;

			std::unique_ptr<RenderDriver::Vulkan::CImage>					mDepthImage;
			std::unique_ptr<RenderDriver::Vulkan::CImageView>				mDepthImageView;

			std::unique_ptr<RenderDriver::Vulkan::CFrameBuffer>				mFrameBuffer;

			std::unique_ptr<RenderDriver::Vulkan::CFence>					mSignalFence;

		protected:
			
			virtual void platformCreateAttachmentImage(
				std::string const& name,
				uint32_t iWidth,
				uint32_t iHeight,
				RenderDriver::Common::Format const& format
			);

			virtual void platformCreateAttachmentBuffer(
				std::string const& name,
				uint32_t iBufferSize,
				RenderDriver::Common::Format const& format,
				RenderDriver::Common::BufferUsage const& usage
			);

			virtual RenderDriver::Common::CBuffer* platformCreateBuffer(
				std::string const& name,
				RenderDriver::Common::BufferDescriptor const& desc
			);

			virtual RenderDriver::Common::CImage* platformCreateImage(
				std::string const& name,
				RenderDriver::Common::ImageDescriptor const& desc
			);

			virtual RenderDriver::Common::CImage* platformCreateImageWithData(
				std::string const& name,
				unsigned char const* pImageData,
				uint32_t iTextureWidth,
				uint32_t iTextureHeight,
				RenderDriver::Common::Format const& format,
				RenderDriver::Common::CCommandQueue* pCommandQueue
			);

			virtual void platformCreateImageView(
				std::string const& name,
				RenderDriver::Common::ImageViewDescriptor const& imageViewDesc);

			virtual void platformInitDescriptorSet();
			virtual void platformInitPipelineState();
			virtual RenderDriver::Common::GraphicsPipelineStateDescriptor* platformInitGraphicsPipelineStateDescriptor();
			virtual RenderDriver::Common::ComputePipelineStateDescriptor* platformInitComputePipelineStateDescriptor();
			virtual RenderDriver::Common::RayTracePipelineStateDescriptor* platformInitRayTracePipelineStateDescriptor();

			virtual RenderDriver::Common::GraphicsPipelineStateDescriptor* platformFillOutGraphicsPipelineDescriptor(
				RenderDriver::Common::GraphicsPipelineStateDescriptor* pDesc,
				std::vector<char>& acShaderBufferVS,
				std::vector<char>& acShaderBufferFS,
				std::string const& shaderPath,
                std::string const& pipelineName
			);

			virtual RenderDriver::Common::ComputePipelineStateDescriptor* platformFillOutComputePipelineDescriptor(
				RenderDriver::Common::ComputePipelineStateDescriptor* pDesc,
				std::vector<char>& acShaderBuffer,
				std::string const& shaderPath,
                std::string const& pipelineName
			);

			virtual RenderDriver::Common::RayTracePipelineStateDescriptor* platformFillOutRayTracePipelineDescriptor(
				RenderDriver::Common::RayTracePipelineStateDescriptor* pDesc,
				std::vector<char>& acRayGenShaderBuffer,
				std::vector<char>& acCloseHitShaderBuffer,
				std::vector<char>& acMissShaderBuffer,
				std::string const& shaderPath,
                std::string const& pipelineName,
				void* pPlatformInstance
			);

			virtual void platformCreateDepthImage(
				uint32_t iWidth,
				uint32_t iHeight
			);

			virtual void platformCreateFrameBuffer(
				RenderDriver::Common::CSwapChain* pSwapChain
			);

			virtual void platformCreateSemaphore();

			virtual void platformInitAttachmentBarriers(
				CreateInfo const& createInfo
			);

			virtual void platformUploadDataToBuffer(
				RenderDriver::Common::CBuffer& buffer,
				char const* pacData,
				uint32_t iDataSize,
				RenderDriver::Common::CCommandQueue& commandQueue
			);
		};

	}   // Common

}   // Render
