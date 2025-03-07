#pragma once

#if defined(_MSC_VER)
#include "dxgi1_6.h"
#include "d3d12.h"
#include "wrl/client.h"
#endif // _MSC_VER

#include <vector>
#include <string>
#include <map>

#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"

#include "render_enums.h"

#include <render-driver/Device.h>
#include <render-driver/Format.h>
#include <render-driver/AccelerationStructure.h>

#if defined(_MSC_VER)
#include "vulkan/vulkan.h"
using namespace Microsoft::WRL;
#endif // _MSC_VER

enum class AttachmentTypeBits
{
	None = 0,
	BufferInput = 1,
	BufferOutput = 2,
	
	TextureInput = 4,
	TextureOutput = 8,

	BufferInputOutput = (BufferInput | BufferOutput),
	TextureInputOutput = (TextureInput | TextureOutput),

	VertexOutput = 16,

	IndirectDrawListInput = 32,
	IndirectDrawListOutput = 64,
};

struct AttachmentInfo
{
	std::string							mName;
	std::string							mParentRenderJobName;
	AttachmentTypeBits					mType;
	RenderDriver::Common::Format		mFormat;
	bool								mbBlend = true;
	float								mfScaleWidth = 1.0f;
	float								mfScaleHeight = 1.0f;

	uint32_t							miShaderResourceIndex = UINT32_MAX;
	bool								mbComputeShaderWritable = false;
	bool								mbHasDepthStencil = true;

	uint32_t							miDataSize = UINT32_MAX;
	bool								mbExternalData = false;
};


struct ShaderDataInfo
{
	void* mpMappedData;
	uint32_t				miDataSize;
	uint32_t				miDataOffset;
	ShaderType				mShaderType;
};

struct PipelineDataInfo
{
	std::map<std::string, ShaderDataInfo>		maDataInfo;

	uint32_t									miDataSize;

};

struct PipelineInfo
{
	std::string									mName;
	std::string									mFilePath;

	PipelineType								mType;
	PipelineDataInfo							mDataInfo;
	
	std::vector<std::string>					maParentPipelines;
	std::vector<std::string>					maChildrenPipelines;
	std::vector<AttachmentInfo>					maAttachments;

	std::vector<uint32_t>						maiChildAttachmentIndices;
	std::vector<uint32_t>						maiParentAttachmentIndices;
};

struct ShaderInfo
{
	std::string		mName;
	ShaderType		mType;
	
	std::string		mFilePath;
	std::string		mFileOutputPath;
};

struct ShaderResourceDataInfo
{
	void const*		mpData = nullptr;
	uint32_t		miDataSize = 0;
	void*			mpMappedData = nullptr;

	ShaderResourceDataInfo() = default;

	ShaderResourceDataInfo(void const* pData, uint32_t iDataSize)
	{
		mpData = pData;
		miDataSize = iDataSize;
	}
};

struct ShaderResourceInfo
{
	std::string					mName;
	ShaderResourceType			mType;
	uint32_t					miResourceIndex;
	std::string					mImageFilePath;
	std::string					mBufferFilePath;
	ResourceViewType			mViewType;

	ShaderResourceDataInfo		mDataInfo;
	uint32_t					miStructByteStride = 0;

	uint32_t					miParentRenderJobIndex = UINT32_MAX;
	uint32_t					miParentShaderResourceIndex = UINT32_MAX;

};

namespace RenderDriver
{
	namespace Common
	{
		enum class BlendType
		{
			None = 0,
			Zero,
			One,
			SrcColor,
			InvSrcColor,
			SrcOneMinusColor,
			SrcAlpha,
			InvSrcAlpha,
			SrcOneMinusAlpha,
			DestColor,
			InvDestColor,
			DestOneMinusColor,
			DestAlpha,
			InvDestAlpha,
			DestOneMinusAlpha,
			SrcAlphaSaturate,
			BlendFactor,
			InvBlendFactor,
			Src1Color,
			InvSrc1Color,
			Src1Alpha,
			InvSrc1Alpha,
		};

		enum class BlendOperator
		{
			None = 0,
			Add,
			Subtract,
			ReverseSubtract,
			Min,
			Max,
			Multiply
		};

		enum class BlendLogicOperator
		{
			Clear = 0,
			Set,
			Copy,
			CopyInverted,
			NoOp,
			Invert,
			And,
			Nand,
			Or,
			Nor,
			Xor,
			Equivalent,
			AndReverse,
			AndInverted,
			OrReverse,
			OrInverted
		};

		enum class ColorWriteMask
		{
			WriteAll = 0xf
		};

		struct BlendState
		{
			bool										mbEnabled = true;
			bool										mbLogicOpEnabled = false;
			RenderDriver::Common::BlendType				mSrcColor = BlendType::One;
			RenderDriver::Common::BlendType				mDestColor = BlendType::Zero;
			RenderDriver::Common::BlendOperator			mColorOp = BlendOperator::Add;
			RenderDriver::Common::BlendType				mSrcAlpha = BlendType::One;
			RenderDriver::Common::BlendType				mDestAlpha = BlendType::Zero;
			RenderDriver::Common::BlendOperator			mAlphaOp = BlendOperator::Add;
			RenderDriver::Common::BlendLogicOperator	mLogicOp = BlendLogicOperator::NoOp;
			RenderDriver::Common::ColorWriteMask		mWriteMask = ColorWriteMask::WriteAll;
		};

		enum class Dimension
		{
			Unknown = 0,
			Buffer,
			Texture1D,
			Texture2D,
			Texture3D
		};

		enum class DepthStencilDimension
		{
			Unknown = 0,
			Texture1D,
			Texture2D,
		};

		enum class ShaderResourceViewDimension
		{
			Unknown = 0,
			Buffer,
			Texture1D,
			Texture1DArray,
			Texture2D,
			Texture2DArray,
			Texture2DMS,
			Texture2DMSArray,
			Texture3D,
			TextureCube,
			TextureCubeArray,
			RayTracingAccelerationStructure
		};

		struct SampleDescriptor
		{
			uint32_t		miCount;
			uint32_t		miQuality;
		};

		enum class TextureLayout
		{
			Unknown = 0,
			RowMajor,
			UndefinedSwizzle,
			StandardSwizzle,
		};

		enum class ImageLayout
		{
			UNDEFINED = 0,
			GENERAL,
			COLOR_ATTACHMENT_OPTIMAL,
			DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			SHADER_READ_ONLY_OPTIMAL,
			TRANSFER_SRC_OPTIMAL,
			TRANSFER_DST_OPTIMAL,
			PREINITIALIZED,
			DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
			DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
			DEPTH_ATTACHMENT_OPTIMAL,
			DEPTH_READ_ONLY_OPTIMAL,
			STENCIL_ATTACHMENT_OPTIMAL,
			STENCIL_READ_ONLY_OPTIMAL,
			READ_ONLY_OPTIMAL,
			ATTACHMENT_OPTIMAL,
			PRESENT_SRC,
		};

		enum class LoadOp
		{
			Clear = 0,
			Load,
		};

		enum class ResourceFlagBits
		{
			None = 0,
			AllowRenderTarget = 0x1,
			AllowDepthStencil = 0x2,
			AllowUnorderedAccess = 0x4,
			DenyShaderResource = 0x8,
			AllowCrossAdaptor = 0x10,
			AllowSimultaneousAccess = 0x20,
			VideoDecodeReferenceOnly = 0x40
		};

		enum class HeapType
		{
			Default,
			Upload,
			ReadBack,
			Custom
		};

		enum class BufferUsage
		{
			TransferSrc = 0x00000001,
			TransferDest = 0x00000002,
			UniformTexel = 0x00000004,
			StorageTexel = 0x00000008,
			UniformBuffer = 0x00000010,
			StorageBuffer = 0x00000020,
			IndexBuffer = 0x00000040,
			VertexBuffer = 0x00000080,
			IndirectBuffer = 0x00000100,
			ShaderBindingTable = 0x00000400,
			ShaderDeviceAddress = 0x00020000,
			AccelerationStructureBuildInputReadOnly = 0x00080000,
			AccelerationStructureStorageBit = 0x00100000,
		};

		struct ResourceDesc
		{
			RenderDriver::Common::Dimension				mDimension;
			uint32_t									miAlignment;
			uint32_t									miWidth;
			uint32_t									miHeight;
			uint32_t									miDepthOrArraySize;
			uint32_t									miMipLevels;
			Format										mFormat;
			Format										mViewFormat;
			SampleDescriptor							mSampleDesc;
			TextureLayout								mLayout;
			RenderDriver::Common::ResourceFlagBits		mFlags;
			uint32_t									miCount;
		};

		enum class InputClassification
		{
			PerVertexData = 0,
			PerInstanceData = 1
		};

		struct VertexFormat
		{
			std::string									mSemanticName;
			uint32_t									miSemanticIndex;
			Format										mFormat;
			uint32_t									miInputSlot;
			uint32_t									miAlignedByteOffset;
			InputClassification							mInputSlotClass;
			uint32_t									miInstanceDataStepRate;
		};

		enum class DepthWriteMask
		{
			Zero = 0,
			All = 1
		};

		enum class ComparisonFunc
		{
			Never = 1,
			Less,
			Equal,
			LessEqual,
			Greater,
			NotEqual,
			GreaterEqual,
			Always
		};

		enum class StencilOperator
		{
			Keep = 1,
			Zero = 2,
			Replace = 3,
			IncrementSaturate = 4,
			DecrementSaturate = 5,
			Invert = 6,
			Increment = 7,
			Decrement = 8
		};

		struct DepthStencilOpDesc
		{
			StencilOperator				mStencilFailOp;
			StencilOperator				mStencilDepthFailOp;
			StencilOperator				mStencilPassOp;
			ComparisonFunc				mStencilFunc;
		};

		struct DepthStencilState
		{
			bool						mbDepthEnabled;
			DepthWriteMask				mDepthWriteMask;
			ComparisonFunc				mDepthFunc;
			bool						mbStencilEnabled;
			uint8_t						miStencilReadMask;
			uint8_t						miStencilWriteMask;
			DepthStencilOpDesc			mFrontFace;
			DepthStencilOpDesc			mBackFace;
		};

		struct ViewportState
		{
			float						mfScaleWidth = 1.0f;
			float						mfScaleHeight = 1.0f;
		};

		enum class FillMode
		{
			Wireframe,
			Solid 
		};

		enum class CullMode
		{
			None,
			Front,
			Back
		};

		enum ConservativeRasterizationMode
		{
			Off,
			On
		};

		enum class TopologyType
		{
			Undefined = 0,
			Point,
			Line,
			Triangle
		};

		struct RasterState
		{
			FillMode mFillMode;
			CullMode mCullMode;
			bool mbFrontCounterClockwise;
			uint32_t miDepthBias;
			float mfDepthBiasClamp;
			float mfSlopeScaledDepthBias;
			bool mbDepthClipEnable;
			bool mbMultisampleEnable;
			bool mbAntialiasedLineEnable;
			uint32_t miForcedSampleCount;
			ConservativeRasterizationMode mConservativeRaster;
		};

		enum class DescriptorRangeType
		{
			SRV,
			UAV,
			CBV,
			Sampler,
		};

		struct DescriptorRange
		{
			DescriptorRangeType			mRangeType;
			uint32_t					miNumDescriptors;
			uint32_t					miBaseShaderRegister;
			uint32_t					miRegisterSpace;
			uint32_t					miOffsetInDescriptorsFromTableStart;
		};

		struct RootSignature
		{

		};

		struct PipelineState
		{

		};

		enum class RootParameterType
		{
			DescriptorTable,
			Constant,
			ConstantBufferView,
			ShaderResourceView,
			UnorderedAccessView
		};

		struct RootDescriptorTable
		{
			uint32_t				miNumDescriptorRanges;
			DescriptorRange* maDescriptorRanges;
		};

		struct RootConstants
		{
			uint32_t		 miShaderRegister;
			uint32_t		 miRegisterSpace;
			uint32_t		 miNum32BitValues;
		};

		struct RootDescriptor
		{
			uint32_t		 miShaderRegister;
			uint32_t		 miRegisterSpace;
		};

		enum class ShaderVisibility
		{
			All,
			Vertex,
			Hull,
			Domain,
			Geometry,
			Pixel
		};

		struct RootParameter
		{
			RootParameterType			mParameterType;
			union
			{
				RootDescriptorTable		mDescriptorTable;
				RootConstants			mConstants;
				RootDescriptor			mDescriptor;
			};
			ShaderVisibility			mShaderVisibility;
		};

		struct PipelineDataInfo
		{
			DescriptorRange								maDescriptorRange[uint32_t(PipelineDataType::NUM_PIPELINE_DATA_TYPES)];
			RootParameter								maRootParameter[uint32_t(PipelineDataType::NUM_PIPELINE_DATA_TYPES)];
			std::map<std::string, ShaderDataInfo>		maDataInfo;

			uint32_t									miDataSize;

		};

		enum class ResourceStateFlagBits
		{
			Common = 0,
			VertexAndConstantBuffer = 0x1,
			IndexBuffer = 0x2,
			RenderTarget = 0x4,
			UnorderedAccess = 0x8,
			DepthWrite = 0x10,
			DepthRead = 0x20,
			NonPixelShaderResource = 0x40,
			PixelShaderResource = 0x80,
			StreamOut = 0x100,
			IndirectArgument = 0x200,
			CopyDestination = 0x400,
			CopySource = 0x800,
			ResolveDestination = 0x1000,
			ResolveSource = 0x2000,
			RayTracingAccelerationStructure = 0x400000,
			ColorAttachment = 0x80000,
			DepthStencilAttachment = 0x100000,
			Present = 0,
			GenericRead,
			Present2 = 0x8000,
			None = 0x0000DEAD,
		};

		enum class DescriptorHeapType
		{
			General = 0,
			Sampler,
			RenderTarget,
			DepthStencil,
		};

		enum class DescriptorHeapFlag
		{
			None = 0,
			ShaderVisible
		};
			
		enum class UAVDimension
		{
			Unknown = 0,
			Buffer = 1,
			Texture1D = 2,
			Texture1DArray = 3,
			Texture2D = 4,
			Texture2Drray = 5,
			Texture3D = 8
		};

		enum class SwapChainFlagBits
		{
			FIFO = 0x1,
		};

		enum class CommandBufferType
		{
			Graphics = 0,
			Compute,
			Copy,
		};

		enum class CommandListType
		{
			Direct = 0,
			Bundle,
			Compute,
			Copy,
			VideoDecode,
			VideoProcess,
			VideoEncode,
		};

		enum class UnorderedAccessViewFlags
		{
			None = 0,
			Raw = 0x1,
		};


		enum class MemoryPool
		{
			Unknown = 0,
			L0 = 1,
			L1 = 2,
		};

		enum class CPUPageProperty
		{
			Unknown,
			NotAvailable,
			WriteCombine,
			WriteBack,
		};

		enum class ResourceViewType
		{
			None = -1,
			ShaderResourceView = 0,
			UnorderedAccessView,
			ConstantBufferView,
			RenderTargetView,
		};

		enum class CommandBufferState
		{
			Initialized = 0,
			Executing,
			Closed,
		};

		enum class ShaderType
		{
			None = 0,
			Vertex = 1,
			Fragment,
			Compute,
			RayTrace,
		};

		class CImage;
		class CBuffer;

	}	// Common

}	// RenderDriver


namespace SerializeUtils
{
	namespace Common
	{
		struct ShaderResourceDataInfo
		{
			void const*							mpData = nullptr;
			uint32_t							miDataSize = 0;
			void*								mpMappedData = nullptr;

			PLATFORM_OBJECT_HANDLE				mHandle = 0;

			ShaderResourceDataInfo() = default;

			ShaderResourceDataInfo(void const* pData, uint32_t iDataSize)
			{
				mpData = pData;
				miDataSize = iDataSize;
			}
		};

		struct ExternalResource
		{
			RenderDriver::Common::CImage*						mpImage = nullptr;
			RenderDriver::Common::CBuffer*						mpBuffer = nullptr;
			RenderDriver::Common::Dimension						mDimension = RenderDriver::Common::Dimension::Unknown;
			RenderDriver::Common::ShaderResourceViewDimension	mShaderResourceViewDimension;
			uint32_t											miNumImages = 0;
			RenderDriver::Common::CImage const*					mpaImages = nullptr;
			uint32_t											miGPUAddressOffset = 0;

			PLATFORM_OBJECT_HANDLE								mRenderTargetHandle = UINT64_MAX;
			bool												mbSkipCreation = false;
		};


		struct ShaderResourceInfo
		{
			std::string									mName;
            std::string                                 mShaderResourceName;
			ShaderResourceType							mType;
			RenderDriver::Common::ResourceDesc			mDesc;
			uint32_t									miResourceIndex;
            uint32_t                                    miResourceSet;
			std::string									mImageFilePath;
			std::string									mBufferFilePath;
			RenderDriver::Common::ResourceViewType		mViewType;

			Common::ShaderResourceDataInfo				maDataInfo[3];
			uint32_t									miStructByteStride = 4;

			uint32_t									miParentRenderJobIndex = UINT32_MAX;
			uint32_t									miParentShaderResourceIndex = UINT32_MAX;

			ExternalResource							mExternalResource;

			bool										mbReadOnly = false;

			PLATFORM_OBJECT_HANDLE						maHandles[3];

			uint64_t									maPlatformExtras[4];

			RenderDriver::Common::ShaderType			mShaderType;
            
            RenderDriver::Common::CAccelerationStructure* mpAccelerationStructure = nullptr;
		};

		struct JobResource
		{

		};

		void getPipelineShaders(
			std::vector<ShaderInfo>& aPipelineShaders,
			char const* szShaderFilePath);

		uint32_t getNumComponents(RenderDriver::Common::Format format);

		uint32_t getBaseComponentSize(RenderDriver::Common::Format format);

		void getVertexFormat(
			std::vector<RenderDriver::Common::VertexFormat>& aInputElementDescs,
			rapidjson::Document const& doc);

		void getAttachments(
			std::vector<AttachmentInfo>& aAttachments,
			rapidjson::Document const& doc);

		void loadShaderResources(
			std::vector<Common::ShaderResourceInfo>& aResourceDesc,
			std::string const& filePath);
	
		void loadShaderResources2(
			std::vector<ShaderResourceInfo>& aResourceDesc,
			std::string const& pipelineFilePath,
			std::string const& shaderResourceRegisterFilePath,
			char const* szShaderType);

		void loadShaderResources3(
			std::vector<ShaderResourceInfo>& aResourceDesc,
			std::string const& pipelineFilePath,
			std::string const& shaderResourceRegisterFilePath,
			char const* szShaderType,
			bool bFillOutConstantBufferEntries);

		void getBlendState(
			std::vector<RenderDriver::Common::BlendState>& aBlendStates,
			rapidjson::Document const& doc);

		void getRenderTargetsFormat(
			std::vector<RenderDriver::Common::Format>& aFormats,
			rapidjson::Document const& doc);

		RenderDriver::Common::Format getTextureFormat(std::string const& format);

		RenderDriver::Common::DepthStencilState getDepthStencilState(rapidjson::Document const& doc);
		RenderDriver::Common::RasterState getRasterState(rapidjson::Document const& doc);

	}	// Common

#if defined(_MSC_VER)
	namespace DX12
	{
		struct ShaderResourceInfo
		{
			std::string									mName;
			ShaderResourceType							mType;
			RenderDriver::Common::ResourceDesc			mDesc;
			uint32_t									miResourceIndex;
			std::string									mImageFilePath;
			std::string									mBufferFilePath;
			RenderDriver::Common::ResourceViewType		mViewType;

			Common::ShaderResourceDataInfo				maDataInfo[3];
			uint32_t									miStructByteStride = 0;

			uint32_t									miParentRenderJobIndex = UINT32_MAX;
			uint32_t									miParentShaderResourceIndex = UINT32_MAX;

			SerializeUtils::Common::ExternalResource	mExternalResource;

			bool										mbReadOnly = false;

			PLATFORM_OBJECT_HANDLE						maHandles[3];

			D3D12_CPU_DESCRIPTOR_HANDLE					mCPUDescriptorHandle;
			void*										mpCounterBuffer;
		};

		struct ShaderResourceDataInfo : public SerializeUtils::Common::ShaderResourceDataInfo
		{

		};

		D3D12_FILL_MODE convert(RenderDriver::Common::FillMode);
		D3D12_CULL_MODE convert(RenderDriver::Common::CullMode);
		D3D12_CONSERVATIVE_RASTERIZATION_MODE convert(RenderDriver::Common::ConservativeRasterizationMode mode);
		D3D12_BLEND convert(RenderDriver::Common::BlendType type);
		D3D12_BLEND_OP convert(RenderDriver::Common::BlendOperator type);
		D3D12_LOGIC_OP convert(RenderDriver::Common::BlendLogicOperator type);
		UINT8 convert(RenderDriver::Common::ColorWriteMask mask);
		D3D12_DEPTH_WRITE_MASK convert(RenderDriver::Common::DepthWriteMask mask);
		D3D12_COMPARISON_FUNC convert(RenderDriver::Common::ComparisonFunc func);
		D3D12_DEPTH_STENCILOP_DESC convert(RenderDriver::Common::DepthStencilOpDesc const& desc);
		DXGI_FORMAT convert(RenderDriver::Common::Format format);
		D3D12_RESOURCE_STATES convert(RenderDriver::Common::ResourceStateFlagBits flagBits);
		D3D12_DESCRIPTOR_HEAP_TYPE convert(RenderDriver::Common::DescriptorHeapType type);
		D3D12_DESCRIPTOR_HEAP_FLAGS convert(RenderDriver::Common::DescriptorHeapFlag flag);
		D3D12_UAV_DIMENSION convert(RenderDriver::Common::UAVDimension dimension);
		D3D12_COMMAND_LIST_TYPE convert(RenderDriver::Common::CommandBufferType type);
		D3D12_INPUT_ELEMENT_DESC convert(RenderDriver::Common::VertexFormat const& format, uint32_t iOffset);
		D3D12_BUFFER_UAV_FLAGS convert(RenderDriver::Common::UnorderedAccessViewFlags flags);
		D3D12_MEMORY_POOL convert(RenderDriver::Common::MemoryPool const& pool);
		D3D12_HEAP_TYPE convert(RenderDriver::Common::HeapType const& type);
		D3D12_INPUT_CLASSIFICATION convert(RenderDriver::Common::InputClassification const& classification);
		D3D12_RESOURCE_FLAGS convert(RenderDriver::Common::ResourceFlagBits const& flagBits);
		D3D12_CPU_PAGE_PROPERTY convert(RenderDriver::Common::CPUPageProperty const& pageProperty);
		D3D12_TEXTURE_LAYOUT convert(RenderDriver::Common::TextureLayout layout);
		D3D12_SRV_DIMENSION convert(RenderDriver::Common::ShaderResourceViewDimension dimension);
		D3D12_RTV_DIMENSION convert(RenderDriver::Common::Dimension);
		D3D12_DSV_DIMENSION convert(RenderDriver::Common::DepthStencilDimension dimension);

		RenderDriver::Common::Format convert(DXGI_FORMAT format);

		uint32_t getBaseComponentSize(DXGI_FORMAT format);

	}	// DX12

	namespace Vulkan
	{
		VkPolygonMode convert(RenderDriver::Common::FillMode const&);
		VkCullModeFlags convert(RenderDriver::Common::CullMode const&);
		VkColorComponentFlags convert(RenderDriver::Common::ColorWriteMask const& mask);
		VkBlendFactor convert(RenderDriver::Common::BlendType const& type);
		VkBlendOp convert(RenderDriver::Common::BlendOperator const& type);
		VkLogicOp convert(RenderDriver::Common::BlendLogicOperator const& type);
		VkFormat convert(RenderDriver::Common::Format const& format);
		VkCompareOp convert(RenderDriver::Common::ComparisonFunc const& op);
		VkStencilOpState convert(RenderDriver::Common::DepthStencilOpDesc const& desc);
		VkVertexInputAttributeDescription convert(
			RenderDriver::Common::VertexFormat const& format, 
			uint32_t iLocation,
			uint32_t iOffset);
		VkImageLayout convert(RenderDriver::Common::TextureLayout const& layout);
		VkImageLayout convert(RenderDriver::Common::ResourceStateFlagBits const& layout);
		VkBufferUsageFlagBits convert(RenderDriver::Common::BufferUsage const& usage);
		VkImageCreateFlags convert(RenderDriver::Common::ResourceFlagBits const& flag);
		VkImageLayout convert(RenderDriver::Common::ImageLayout const& imageLayout);
		VkAttachmentLoadOp convert(RenderDriver::Common::LoadOp const& loadOp);

		RenderDriver::Common::ImageLayout convert(VkImageLayout const& imageLayout);

		uint32_t getBaseComponentSize(VkFormat const& format);


	}	// Vulkan
#endif // _MSC_VER

}	// SerializeUtils
