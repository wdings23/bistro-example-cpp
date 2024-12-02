#pragma once

#include <stdint.h>
#include <serialize_utils.h>
#include <mat4.h>
#include <Camera.h>
#include <render-driver/Device.h>
#include <render-driver/PipelineInfo.h>
#include <render-driver/DescriptorSet.h>
#include <render-driver/DescriptorHeap.h>
#include <render-driver/CommandBuffer.h>
#include <render-driver/RenderTarget.h>
#include <render-driver/Fence.h>
#include <render-driver/PipelineState.h>
#include <render-driver/BufferView.h>
#include <render-driver/FrameBuffer.h>

#include <render/render_job_enums.h>

#include <functional>

#define NUM_BUFFER_OBJECTS 3

namespace Render
{
	namespace Common
	{
		class CRenderer;

		struct RenderJobAttachmentInfo
		{
			std::string						mName;
			std::string						mParentName;
			std::string						mParentJobName;
			std::string						mShaderResourceName;
			uint32_t						miDataSize = 0;
			PLATFORM_OBJECT_HANDLE			mAttachmentHandle = 0;
			bool							mbClear = true;
			AttachmentType					mType = AttachmentType::None;
			AttachmentType					mViewType = AttachmentType::None;
			RenderDriver::Common::Format	mFormat = RenderDriver::Common::Format::R32G32B32A32_FLOAT;
			float							mafClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			bool							mbHasClearColor = false;
			bool							mbExternalData = false;
		};

		struct DrawOverrideInfo
		{
			void*					mpUserData;
			void*					mpRenderJob;
		};

		struct ExtraConstantBufferEntry
		{
			std::string						mName;
			std::string						mType;
			float							mfValue;
			std::string						mDynamicValue = "";
		};

		struct RenderJobInfo
		{
			std::string															mName;
			std::string															mTypeStr;
			std::string															mPassTypeStr;

			std::vector<std::map<std::string, std::string>>						maDelayedAttachments;
			uint32_t															maiViewPort[4];
			bool																mbDrawEnabled;
			uint32_t															maiDispatchSizes[3];
			
			std::unique_ptr<RenderDriver::Common::CImage>						mDepthTexture;
			std::unique_ptr<RenderDriver::Common::CImageView>					mDepthTextureView;

			std::vector<SerializeUtils::Common::ShaderResourceInfo>				maShaderResourceInfo;
			std::vector<SerializeUtils::Common::ShaderResourceDataInfo>			maaTripleBufferShaderResourceDataInfo[NUM_BUFFER_OBJECTS];

			std::vector<RenderJobAttachmentInfo>								maAttachmentInfo;

			uint32_t															miPipelineIndex;
			uint32_t															miPassLevel;

			float2																mViewportScale = float2(1.0f, 1.0f);

			JobType																mType;
			PassType															mPassType;

			std::vector<ExtraConstantBufferEntry>								maExtraConstantBufferEntries;

			PLATFORM_OBJECT_HANDLE												maDescriptorHeapHandles[NUM_BUFFER_OBJECTS];
			PLATFORM_OBJECT_HANDLE												mCommandBufferHandle;
			PLATFORM_OBJECT_HANDLE												maCommandAllocatorHandles[NUM_BUFFER_OBJECTS];
			PLATFORM_OBJECT_HANDLE												maFenceHandles[3];

			std::vector<PLATFORM_OBJECT_HANDLE>									maResourceHandles[NUM_BUFFER_OBJECTS];

			std::vector<PLATFORM_OBJECT_HANDLE>									maOutputRenderTargetAttachments;
			std::vector<PLATFORM_OBJECT_HANDLE>									maOutputRenderTargetColorHeaps;
			std::vector<PLATFORM_OBJECT_HANDLE>									maOutputRenderTargetDepthHeaps;

			std::vector<PLATFORM_OBJECT_HANDLE>									maOutputRenderTargetAttachmentBuffers;
			std::vector<PLATFORM_OBJECT_HANDLE>									maInputRenderTargetAttachmentBufferAsImages;
			std::vector<PLATFORM_OBJECT_HANDLE>									maInputRenderTargetAttachmentBuffers;

			std::vector<PLATFORM_OBJECT_HANDLE>									maInputRenderTargetAttachments;
			
			std::vector<uint32_t>												maiOutputAttachmentMapping;
			std::vector<uint32_t>												maiInputAttachmentMapping;

			std::vector<RenderJobInfo const*>									mapChildren;
			std::vector<RenderJobInfo const*>									mapParents;

			float																mafRootConstants[16];
			uint32_t															miNumRootConstants = 0;

			uint32_t															miNumDispatchX = 0;
			uint32_t															miNumDispatchY = 0;
			uint32_t															miNumDispatchZ = 0;

			std::vector<std::string>											maWaitOnJobs;
			std::vector<std::string>											maSkipWaitOnJobs;

			void(*mpfnDrawOverride)(RenderDriver::Common::CCommandBuffer&, void*) = nullptr;
			void*																mpDrawFuncUserData = nullptr;

			PLATFORM_OBJECT_HANDLE												mFrameBuffer;
		};

		class Serializer;
		struct ShaderResourceInitializeDescriptor
		{
			SerializeUtils::Common::ShaderResourceInfo* mpShaderResource;
			Render::Common::RenderJobInfo*				mpRenderJobInfo;
			Render::Common::Serializer*					mpSerializer;
			Render::Common::CRenderer*					mpRenderer;
			uint32_t									miResourceIndex;
			void*										mpUserData;
		};

		struct ShaderResourceInitializeFunctions
		{
			void (*mPreCreationDataInitFunction)(ShaderResourceInitializeDescriptor&) = nullptr;
			void (*mPostCreationDataInitFunction)(ShaderResourceInitializeDescriptor&) = nullptr;
			void* mpUserData = nullptr;
		};

		struct PipelineInfo;

		class Serializer
		{
		public:
			Serializer() = default;
			virtual ~Serializer() = default;

			void init(ShaderResourceInitializeDescriptor& desc);

			void addInitDataFunction(
				std::string const& renderJobName,
				std::string const& shaderResourceName,
				ShaderResourceInitializeFunctions const& functions);

			uint32_t serializePipeline(
				std::vector<RenderDriver::Common::PipelineInfo>& aPipelineInfos,
				std::vector<RenderDriver::Common::RootSignature>& aRootSignatures,
				std::vector<RenderDriver::Common::PipelineState>& aPipelineStates,
				std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResourceInfo,
				std::string const& filePath,
				std::vector<RenderJobInfo> const& aRenderJobs,
				RenderJobInfo const& renderJob,
				RenderDriver::Common::CDevice& device);

			void initShaderResources(
				RenderJobInfo& renderJobInfo,
				uint32_t iScreenWidth,
				uint32_t iScreenHeight,
				std::string const& renderJobName,
				RenderDriver::Common::CDevice& device);

			void initRenderJobs(
				std::string const& filePath,
				uint32_t iScreenWidth,
				uint32_t iScreenHeight,
				RenderDriver::Common::CDevice& device);

			void buildDependencyGraph();

			void setRenderJobDrawFunc(
				void(*pfnDrawOverride)(RenderDriver::Common::CCommandBuffer&, void*),
				void* pUserData,
				std::string const& renderJobName);
			
			RenderDriver::Common::PipelineState& getPipelineState(uint32_t iRenderJob);
			
			std::vector<SerializeUtils::Common::ShaderResourceInfo> const& getShaderResourceInfo(uint32_t iRenderJob);

			RenderDriver::Common::PipelineInfo const& getPipelineInfo(uint32_t iRenderJob);
			RenderDriver::Common::PipelineInfo const* getPipelineInfo(std::string const& name);
			
			std::vector<RenderJobInfo>& getRenderJobs();
			RenderJobInfo& getRenderJob(uint32_t iRenderJob);
			RenderJobInfo& getRenderJob(std::string const& jobName);
			bool hasRenderJob(std::string const& jobName);

			inline uint32_t getNumComputeJobs()
			{
				return miNumComputeJobs;
			}

			inline uint32_t getNumGraphicJobs()
			{
				return miNumGraphicJobs;
			}

			inline RenderDriver::Common::CDevice* getDevice()
			{
				return mpDevice;
			}

			virtual void updateShaderResourceData(
				SerializeUtils::Common::ShaderResourceInfo& shaderResource,
				void const* pNewData,
				uint32_t iBufferIndex,
				RenderDriver::Common::CDevice& device);

			inline void setCommandQueue(RenderDriver::Common::CCommandQueue* pCommandQueue)
			{
				mpCommandQueue = pCommandQueue;
			}

			inline void setRenderer(Render::Common::CRenderer* pRenderer)
			{
				mpRenderer = pRenderer;
			}

			virtual std::unique_ptr<RenderDriver::Common::CImage>& platformGetRenderTargetImage(PLATFORM_OBJECT_HANDLE handle);
			virtual std::unique_ptr<RenderDriver::Common::CBuffer>& platformGetRenderTargetBuffer(PLATFORM_OBJECT_HANDLE handle);

			SerializeUtils::Common::ShaderResourceInfo* getShaderResource(
				std::string const& renderJobName,
				std::string const& shaderResourceName);


			std::unique_ptr<RenderDriver::Common::CRenderTarget>& getRenderTarget(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CFence>& getFence(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CCommandBuffer>& getCommandBuffer(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CCommandAllocator>& getCommandAllocator(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CPipelineState>& getPipelineStateFromHandle(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CDescriptorHeap>& getDescriptorHeap(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CDescriptorSet>& getDescriptorSet(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CBuffer>& getBuffer(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CImage>& getImage(PLATFORM_OBJECT_HANDLE handle);
			std::unique_ptr<RenderDriver::Common::CFrameBuffer>& getFrameBuffer(PLATFORM_OBJECT_HANDLE handle);

			RenderDriver::Common::CBuffer* getBuffer(std::string const& bufferName);

			void setBuffer(std::unique_ptr<RenderDriver::Common::CBuffer>& buffer);

			bool isImage(PLATFORM_OBJECT_HANDLE handle);

			bool hasRenderTarget(PLATFORM_OBJECT_HANDLE handle);

		protected:
			std::vector<RenderJobInfo>							maRenderJobs;
			std::vector<RenderDriver::Common::PipelineInfo>		maPipelines;
			std::vector<RenderDriver::Common::RootSignature>	maPipelineRootSignatures;
			std::vector<RenderDriver::Common::PipelineState>	maPipelineStates;

			uint32_t											miNumGraphicJobs;
			uint32_t											miNumComputeJobs;
			uint32_t											miNumCopyJobs;

			std::map<std::pair<std::string, std::string>, ShaderResourceInitializeFunctions> mInitializeFunctions;

			RenderDriver::Common::CDevice*						mpDevice;
			RenderDriver::Common::CCommandQueue*				mpCommandQueue;
			Render::Common::CRenderer*							mpRenderer;

			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CRenderTarget>>         mapRenderTargets;
			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CFence>>                mapFences;
			
			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CCommandBuffer>>        mapCommandBuffers;
			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CCommandAllocator>>     mapCommandAllocators;

			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CPipelineState>>        mapComputePipelineStates;
			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CPipelineState>>        mapGraphicsPipelineStates;

			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CDescriptorHeap>>	   mapDescriptorHeaps;
			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CDescriptorSet>>        mapDescriptorSets;

			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CBuffer>>               mapBuffers;
			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CBufferView>>           mapBufferViews;

			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CImage>>				   mapImages;
			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CImageView>>			   mapImageViews;

			std::map<PLATFORM_OBJECT_HANDLE, std::unique_ptr<RenderDriver::Common::CFrameBuffer>>		   mapFrameBuffers;

		protected:
			virtual void initRenderJob(
				RenderJobInfo& renderJobInfo,
				uint32_t iPipelineIndex,
				RenderDriver::Common::CDevice& device);

			virtual PLATFORM_OBJECT_HANDLE createDescriptorHeap(
				uint32_t iNumShaderResources,
				RenderDriver::Common::CDevice& device);

			virtual void initCommandBuffer(
				PLATFORM_OBJECT_HANDLE& commandList,
				PLATFORM_OBJECT_HANDLE* aCommandAllocator,
				PLATFORM_OBJECT_HANDLE* aFenceHandles,
				PLATFORM_OBJECT_HANDLE pipelineState,
				PipelineType const& type,
				std::string const& renderJobName,
				RenderDriver::Common::CDevice& device);

			virtual PLATFORM_OBJECT_HANDLE serializeShaderDescriptors(
				std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResourceInfo,
				PipelineType pipelineType,
				uint32_t iNumRootConstants,
				RenderDriver::Common::CDevice& device);

			virtual PLATFORM_OBJECT_HANDLE serializeComputePipeline(
				PLATFORM_OBJECT_HANDLE rootSignature,
				ShaderInfo const& shaderInfo,
				Render::Common::RenderJobInfo const& renderJob,
				RenderDriver::Common::CDevice& device);

			virtual PLATFORM_OBJECT_HANDLE serializeGraphicsPipeline(
				PLATFORM_OBJECT_HANDLE descriptorHandle,
				std::vector<RenderDriver::Common::BlendState> const& aBlendStates,
				RenderDriver::Common::DepthStencilState const& depthStencilState,
				RenderDriver::Common::RasterState const& rasterState,
				std::vector<RenderDriver::Common::Format> const& aRenderTargetFormats,
				rapidjson::Document const& doc,
				ShaderInfo const& vertexShaderInfo,
				ShaderInfo const& fragmentShaderInfo,
				Render::Common::RenderJobInfo const& renderJob,
				RenderDriver::Common::CDevice& device);

			virtual PLATFORM_OBJECT_HANDLE createResourceView(
				SerializeUtils::Common::ShaderResourceInfo& shaderResource,
				SerializeUtils::Common::ShaderResourceDataInfo& shaderData,
				uint32_t iDataSize,
				RenderDriver::Common::CDevice& device);

			
			virtual PLATFORM_OBJECT_HANDLE createResource(
				PLATFORM_OBJECT_HANDLE descriptorHeapHandle,
				SerializeUtils::Common::ShaderResourceInfo& shaderResource,
				uint32_t iResourceIndex,
				uint32_t iImageWidth,
				uint32_t iImageHeight,
				uint32_t iTripleBufferIndex,
				Render::Common::JobType jobType,
				RenderDriver::Common::CDevice& device);

			virtual void createAttachmentView(
				Render::Common::RenderJobInfo& renderJob,
				uint32_t iAttachmentIndex,
				uint32_t iShaderResourceIndex,
				uint32_t iTotalSize,
				bool bBufferAttachment,
				bool bExternalImageAttachment,
				Render::Common::AttachmentType const& viewType,
				RenderDriver::Common::CDevice& device) = 0;

			virtual void createWritableAttachmentView(
				Render::Common::RenderJobInfo& renderJob,
				uint32_t iAttachmentIndex,
				uint32_t iShaderResourceIndex,
				uint32_t iTotalSize,
				bool bBufferAttachment,
				bool bExternalImageAttachment,
				Render::Common::AttachmentType const& viewType,
				RenderDriver::Common::CDevice& device) = 0;

			virtual void initRenderTarget(
				PLATFORM_OBJECT_HANDLE& renderTarget,
				PLATFORM_OBJECT_HANDLE& colorRenderTargetHeap,
				PLATFORM_OBJECT_HANDLE& depthStencilRenderTargetHeap,
				AttachmentInfo const& attachmentInfo,
				uint32_t iWidth,
				uint32_t iHeight,
				float const* afClearColor,
				RenderDriver::Common::CDevice& device);

			virtual void initRenderTargetBuffer(
				PLATFORM_OBJECT_HANDLE& renderTargetBuffer,
				AttachmentInfo const& attachmentInfo,
				uint32_t iWidth,
				uint32_t iHeight,
				uint32_t iPipelineStateIndex,
				RenderDriver::Common::CDevice& device) = 0;

			virtual void initAttachmentImage(
				PLATFORM_OBJECT_HANDLE& attachmentImageHandle,
				AttachmentInfo const& attachmentInfo,
				uint32_t iWidth,
				uint32_t iHeight,
				RenderDriver::Common::CDevice& device);

			uint32_t findRenderJobLevel(RenderJobInfo& renderJob, uint32_t iLevel);

			void execPreDataInitFunctions(RenderJobInfo& renderJob);
			void execPostDataInitFunctions(RenderJobInfo& renderJob);

			virtual void createResourceViewOnly(
				PLATFORM_OBJECT_HANDLE descriptorHeapHandle,
				SerializeUtils::Common::ShaderResourceInfo& shaderResource,
				uint32_t iResourceIndex,
				RenderDriver::Common::CDevice& device);
			
			virtual void associateReadOnlyBufferToShaderResource(
				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo);
			virtual void associateReadWriteBufferToShaderResource(
				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo);
			virtual void associateConstantBufferToShaderResource(
				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo,
				std::string const& renderJobName);
			virtual void associateRayTraceSceneBufferToShaderResource(
				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo);
			virtual void associateExternalBufferToShaderResource(
				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo);

			virtual void setBufferView(
				Render::Common::RenderJobInfo& renderJob,
				RenderDriver::Common::CBuffer* pBuffer,
				uint32_t iShaderResourceIndex,
				uint32_t iBufferSize,
				RenderDriver::Common::CDevice& device) = 0;

			bool canSkipShaderResourceCreation(
				SerializeUtils::Common::ShaderResourceInfo& shaderResourceInfo,
				Render::Common::RenderJobInfo& renderJobInfo);

			virtual void platformCreateFrameBuffer(
				Render::Common::RenderJobInfo& renderJobInfo) = 0;
			virtual void platformUpdateSamplers(
				Render::Common::RenderJobInfo& renderJobInfo) = 0;
			virtual void platformUpdateOutputAttachments(
				Render::Common::RenderJobInfo& renderJobInfo) = 0;
		};
	
	}	// Common

}	// Render