#pragma once

#include <render-driver/Device.h>
#include <render-driver/Image.h>
#include <render-driver/ImageView.h>
#include <render-driver/Buffer.h>
#include <render-driver/CommandQueue.h>
#include <render-driver/DescriptorSet.h>
#include <render-driver/PipelineState.h>

#include <render/RenderJobSerializer.h>

#include <vector>

namespace Render
{
    namespace Common
    {
		enum class LoadStoreOp
		{
			Clear,
			Load,
			Store,
			Discard
		};

        class CRenderJob
        {
		public:
            struct CreateInfo
            {
                std::string const                                   mName;
                RenderDriver::Common::CDevice*                      mpDevice;
				RenderDriver::Common::CCommandQueue*				mpCommandQueue;
                std::string const                                   mFilePath;
				std::string const									mTopDirectory;
                uint32_t                                            miScreenWidth;
                uint32_t                                            miScreenHeight;
                std::vector<Render::Common::CRenderJob*>*            mpaRenderJobs;
                std::vector<std::map<std::string, std::string>> const*    mpAttachmentJSONInfo;
				std::vector<std::map<std::string, std::string>> const*	mpExtraAttachmentJSONInfo;
				RenderDriver::Common::CBuffer*						mpDefaultUniformBuffer;
				uint3												mDispatchSize;
				uint64_t*										    mpiSemaphoreValue;
				RenderDriver::Common::CSwapChain*					mpSwapChain;
				void*												maSamplers;
				void*												mpPlatformInstance;		// for getting function ptr, ie. vulkan
				std::map<std::string, RenderDriver::Common::CAccelerationStructure*>* mpapAccelerationStructures;
			};

        public:

            CRenderJob();
            virtual ~CRenderJob();

            void create(CreateInfo& createInfo);

			void createOutputAttachments(CreateInfo const& createInfo);

		public:
			std::string												mName;
			Render::Common::JobType									mType;
			PassType												mPassType;

			std::map<std::string, RenderDriver::Common::CImage*>		mapOutputImageAttachments;
			std::map<std::string, RenderDriver::Common::CImageView*>	mapOutputImageAttachmentViews;

			std::map<std::string, RenderDriver::Common::CBuffer*>		mapOutputBufferAttachments;
			std::map<std::string, RenderDriver::Common::CBufferView*>	mapOutputBufferAttachmentViews;

			std::map<std::string, RenderDriver::Common::CImage*>		mapInputImageAttachments;
			std::map<std::string, RenderDriver::Common::CImageView*>	mapInputImageAttachmentViews;

			std::map<std::string, RenderDriver::Common::CBuffer*>		mapInputBufferAttachments;
			std::map<std::string, RenderDriver::Common::CBufferView*>	mapInputBufferAttachmentViews;

			RenderDriver::Common::CPipelineState*						mpPipelineState;

			RenderDriver::Common::CFrameBuffer*							mpFrameBuffer;
			float2														mViewportScale = float2(1.0f, 1.0f);

			RenderDriver::Common::CDescriptorSet*						mpDescriptorSet;
			RenderDriver::Common::CDescriptorHeap*						mpDescriptorHeap = nullptr;
			RenderDriver::Common::CDescriptorHeap*						mpDepthStencilDescriptorHeap = nullptr;

			uint32_t													miNumRootConstants;
			std::vector<float>											mafRootConstants;

			std::map<std::string, std::map<std::string, std::string>>	maAttachmentInfo;
			std::map<std::string, std::map<std::string, std::string>>	maShaderResourceInfo;

			std::map<std::string, RenderDriver::Common::CBuffer*>		mapUniformBuffers;
			std::map<std::string, RenderDriver::Common::CImage*>		mapUniformImages;

			std::map<std::string, RenderDriver::Common::CImageView*>	mapUniformImageViews;

			std::map<std::string, RenderDriver::Common::CAccelerationStructure*>	mapAccelerationStructures;

			uint32_t													maiDispatchSize[3];

			uint64_t													miWaitSemaphoreValue;
			uint64_t													miSignalSemaphoreValue;

			RenderDriver::Common::CFence*								mpWaitFence;
			RenderDriver::Common::CFence*								mpSignalFence;

			RenderDriver::Common::CImage*								mpDepthImage;
			RenderDriver::Common::CImageView*							mpDepthImageView;

			std::vector<std::pair<std::string, std::pair<std::string, std::string>>>			maCopyAttachmentMapping;

        protected:
            RenderDriver::Common::CDevice*							mpDevice;
			RenderDriver::Common::CCommandQueue*					mpCommandQueue;

			bool													mbDrawEnabled;
			uint32_t												mViewPort[4];
			
			std::map<std::string, std::string>						mShaderResourceUserData;

			std::map<std::string, RenderDriver::Common::CImage*>		mapImageAttachments;
			std::map<std::string, RenderDriver::Common::CImageView*>	mapImageAttachmentViews;
			std::map<std::string, RenderDriver::Common::CBuffer*>		mapBufferAttachments;
			std::map<std::string, RenderDriver::Common::CBufferView*>	mapBufferAttachmentViews;
			std::map<std::string, RenderDriver::Common::CAccelerationStructure*> mapAccelerationAttachments;

			std::vector<std::pair<std::string, std::string>>					maAttachmentMappings;
			
			std::map<std::string, RenderDriver::Common::Format>			maAttachmentFormats;
			std::map<std::string, LoadStoreOp>							maAttachmentLoadOps;
			std::map<std::string, LoadStoreOp>							maAttachmentStoreOps;
			std::map<std::string, RenderDriver::Common::CImage const*>	mapCopyImageAttachments;
			std::map<std::string, RenderDriver::Common::CBuffer const*> mapCopyBufferAttachments;

			
			std::vector<std::pair<std::string, std::string>>			maUniformMappings;

			std::vector<RenderDriver::Common::CImage*>					mapCopyImageSrc;
			std::vector<RenderDriver::Common::CBuffer*>					mapCopyBufferSrc;

			std::vector<RenderDriver::Common::CImage*>							mapCopyImageDest;
			std::vector<RenderDriver::Common::CBuffer*>							mapCopyBufferDest;

			RenderDriver::Common::CBuffer*								mpDefaultUniformBuffer;

			bool														mbInputOutputDepth = false;

		protected:
			void createAttachments(
				rapidjson::Document const& doc,
				uint32_t iScreenWidth,
				uint32_t iScreenHeight,
				std::vector<Render::Common::CRenderJob*>* apRenderJobs,
				std::vector<std::map<std::string, std::string>> const* pExtraAttachmentJSONInfo
			);

			void createPipelineData(
				rapidjson::Document const& doc,
				std::vector<CRenderJob*>* apRenderJobs
			);

			void initPipelineLayout(
				CreateInfo const& createInfo
			);

			void initPipelineState(
				rapidjson::Document const& doc,
				std::vector<CRenderJob*>* apRenderJobs,
				uint32_t iWidth,
				uint32_t iHeight,
				std::string const& shaderDirectory,
				void* pPlatformInstance	// used for getting vulkan function ptr
			);

			void createFrameBuffer(
				RenderDriver::Common::CSwapChain* pSwapChain
			);

			void processDepthAttachment(
				std::vector<CRenderJob*>* apRenderJobs
			);

			void handleTextureOutput(
				rapidjson::Value const& attachmentJSON,
				uint32_t iImageWidth,
				uint32_t iImageHeight
			);

			void handleTextureInput(
				rapidjson::Value const& attachmentJSON,
				std::vector<Render::Common::CRenderJob*>* apRenderJobs
			);

			void handleBufferOutput(
				rapidjson::Value const& attachmentJSON
			);

			void handleBufferInput(
				rapidjson::Value const& attachmentJSON,
				std::vector<Render::Common::CRenderJob*>* apRenderJobs
			);

			void handleBufferInputOutput(
				rapidjson::Value const& attachmentJSON,
				std::vector<Render::Common::CRenderJob*>* apRenderJobs
			);

			void handleTextureInputOutput(
				rapidjson::Value const& attachmentJSON,
				std::vector<Render::Common::CRenderJob*>* apRenderJobs,
				uint32_t iImageWidth,
				uint32_t iImageHeight
			);

			void handleUniformBuffer(
				rapidjson::Value const& shaderResource,
				std::vector<CRenderJob*>* apRenderJobs
			);

			void handleUniformTexture(
				rapidjson::Value const& shaderResource,
				std::vector<CRenderJob*>* apRenderJobs
			);

			void computeSemaphoreValues(
				CreateInfo& createInfo
			);

			virtual void platformCreateAttachmentImage(
				std::string const& name,
				uint32_t iWidth,
				uint32_t iHeight,
				RenderDriver::Common::Format const& format
			) = 0;

			virtual void platformCreateAttachmentBuffer(
				std::string const& name,
				uint32_t iBufferSize,
				RenderDriver::Common::Format const& format,
				RenderDriver::Common::BufferUsage const& usage
			) = 0;

			virtual RenderDriver::Common::CBuffer* platformCreateBuffer(
				std::string const& name,
				RenderDriver::Common::BufferDescriptor const& desc
			) = 0;

			virtual RenderDriver::Common::CImage* platformCreateImage(
				std::string const& name,
				RenderDriver::Common::ImageDescriptor const& desc
			) = 0;

			virtual RenderDriver::Common::CImage* platformCreateImageWithData(
				std::string const& name,
				unsigned char const* pImageData,
				uint32_t iTextureWidth,
				uint32_t iTextureHeight,
				RenderDriver::Common::Format const& format
			) = 0;

			virtual void platformInitDescriptorSet() = 0;
			virtual void platformInitPipelineState() = 0;
			virtual RenderDriver::Common::GraphicsPipelineStateDescriptor* platformInitGraphicsPipelineStateDescriptor() = 0;
			virtual RenderDriver::Common::ComputePipelineStateDescriptor* platformInitComputePipelineStateDescriptor() = 0;
			virtual RenderDriver::Common::RayTracePipelineStateDescriptor* platformInitRayTracePipelineStateDescriptor() = 0;

			virtual RenderDriver::Common::GraphicsPipelineStateDescriptor* platformFillOutGraphicsPipelineDescriptor(
				RenderDriver::Common::GraphicsPipelineStateDescriptor* pDesc,
				std::vector<char>& acShaderBufferVS,
				std::vector<char>& acShaderBufferFS,
				std::string const& shaderPath
			) = 0;
			
			virtual RenderDriver::Common::ComputePipelineStateDescriptor* platformFillOutComputePipelineDescriptor(
				RenderDriver::Common::ComputePipelineStateDescriptor* pDesc,
				std::vector<char>& acShaderBuffer,
				std::string const& shaderPath
			) = 0;

			virtual RenderDriver::Common::RayTracePipelineStateDescriptor* platformFillOutRayTracePipelineDescriptor(
				RenderDriver::Common::RayTracePipelineStateDescriptor* pDesc,
				std::vector<char>& acRayGenShaderBuffer,
				std::vector<char>& acCloseHitShaderBuffer,
				std::vector<char>& acMissShaderBuffer,
				std::string const& shaderPath,
				void* pPlatformInstance
			) = 0;

			virtual void platformCreateDepthImage(
				uint32_t iWidth,
				uint32_t iHeight
			) = 0;

			virtual void platformCreateFrameBuffer(
				RenderDriver::Common::CSwapChain* pSwapChain
			) = 0;

			virtual void platformCreateSemaphore() = 0;
		};
    
    }   // Common

}   // Render
