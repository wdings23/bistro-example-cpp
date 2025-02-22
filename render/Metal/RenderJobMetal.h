#pragma once

#include <render-driver/Metal/BufferMetal.h>
//#include <render-driver/Metal/BufferViewMetal.h>
#include <render-driver/Metal/ImageMetal.h>
//#include <render-driver/Metal/ImageViewMetal.h>
#include <render-driver/Metal/DescriptorSetMetal.h>
#include <render-driver/Metal/PipelineStateMetal.h>
//#include <render-driver/Metal/FrameBufferMetal.h>
#include <render-driver/Metal/FenceMetal.h>

#include <render/RenderJob.h>

#include <vector>

namespace Render
{
	namespace Metal
	{
		class CRenderJob : public Render::Common::CRenderJob
		{
		public:

			CRenderJob();
			virtual ~CRenderJob();


		protected:
			std::map<std::string, std::unique_ptr<RenderDriver::Metal::CImage>>				maOutputImageAttachments;
			
			std::map<std::string, std::unique_ptr<RenderDriver::Metal::CBuffer>>				maOutputBufferAttachments;
			
			std::map<std::string, std::unique_ptr<RenderDriver::Metal::CBuffer>>				mapBuffers;
			
			std::map<std::string, std::unique_ptr<RenderDriver::Metal::CImage>>				mapImages;
			
			
			std::unique_ptr<RenderDriver::Metal::CDescriptorSet>								mDescriptorSet;
			std::unique_ptr<RenderDriver::Metal::CPipelineState>								mPipelineState;

			std::unique_ptr<RenderDriver::Metal::CPipelineState::GraphicsPipelineStateDescriptor>		mGraphicsPipelineStateDesc;
			std::unique_ptr<RenderDriver::Metal::CPipelineState::ComputePipelineStateDescriptor>		mComputePipelineStateDesc;
			//std::unique_ptr<RenderDriver::Metal::CPipelineState::RayTracePipelineStateDescriptor>		mRayTracePipelineStateDesc;

			std::unique_ptr<RenderDriver::Metal::CImage>					mDepthImage;
			
			//std::unique_ptr<RenderDriver::Metal::CFrameBuffer>				mFrameBuffer;

			std::unique_ptr<RenderDriver::Metal::CFence>					mSignalFence;

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
