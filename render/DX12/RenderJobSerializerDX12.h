#pragma once

#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/ImageDX12.h>
#include <render-driver/DX12/ImageViewDX12.h>
#include <render-driver/DX12/BufferDX12.h>
#include <render-driver/DX12/BufferViewDX12.h>
#include <render-driver/DX12/DescriptorHeapDX12.h>
#include <render-driver/DX12/DescriptorSetDX12.h>
#include <render-driver/DX12/PipelineStateDX12.h>
#include <render-driver/DX12/CommandBufferDX12.h>
#include <render-driver/DX12/CommandAllocatorDX12.h>
#include <render-driver/DX12/FenceDX12.h>
#include <render-driver/DX12/CommandBufferDX12.h>
#include <render-driver/DX12/RenderTargetDX12.h>
#include <render/RenderJobSerializer.h>

namespace Render
{
    namespace DX12
    {
        class Serializer : public Render::Common::Serializer
        {
        public:
            Serializer() = default;
            virtual ~Serializer() = default;

            virtual void initCommandBuffer(
                PLATFORM_OBJECT_HANDLE& commandList,
                PLATFORM_OBJECT_HANDLE* commandAllocator,
                PLATFORM_OBJECT_HANDLE* aFenceHandles,
                PLATFORM_OBJECT_HANDLE pipelineState,
                PipelineType const& type,
                std::string const& renderJobName,
                RenderDriver::Common::CDevice& device);

            virtual void updateShaderResourceData(
                SerializeUtils::Common::ShaderResourceInfo& shaderResource,
                void const* pNewData,
                uint32_t iBufferIndex,
                RenderDriver::Common::CDevice& device);

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
                RenderDriver::Common::CDevice& device);

            virtual void initAttachmentImage(
                PLATFORM_OBJECT_HANDLE& attachmentImageHandle,
                AttachmentInfo const& attachmentInfo,
                uint32_t iWidth,
                uint32_t iHeight,
                RenderDriver::Common::CDevice& device);

            RenderDriver::Common::CDescriptorHeap* registerObject(
                std::unique_ptr<RenderDriver::DX12::CDescriptorHeap>& pObject,
                PLATFORM_OBJECT_HANDLE handle);

            RenderDriver::Common::CRenderTarget* registerObject(
                std::unique_ptr<RenderDriver::DX12::CRenderTarget>& pObject,
                PLATFORM_OBJECT_HANDLE handle);

            RenderDriver::Common::CCommandAllocator* registerObject(
                std::unique_ptr<RenderDriver::DX12::CCommandAllocator>& pObject,
                PLATFORM_OBJECT_HANDLE handle);

            RenderDriver::Common::CCommandBuffer* registerObject(
                std::unique_ptr<RenderDriver::DX12::CCommandBuffer>& pObject,
                PLATFORM_OBJECT_HANDLE handle);

            RenderDriver::Common::CFence* registerObject(
                std::unique_ptr<RenderDriver::DX12::CFence>& pObject,
                PLATFORM_OBJECT_HANDLE handle);

        protected:
            PLATFORM_OBJECT_HANDLE miCurrHandle = 1;

            virtual PLATFORM_OBJECT_HANDLE serializeShaderDescriptors(
                std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResourceInfo,
                PipelineType pipelineType,
                uint32_t iNumRootConstants,
                RenderDriver::Common::CDevice& device);

            virtual PLATFORM_OBJECT_HANDLE serializeComputePipeline(
                PLATFORM_OBJECT_HANDLE descriptorHandle,
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

            virtual PLATFORM_OBJECT_HANDLE createDescriptorHeap(
                uint32_t iNumShaderResources,
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

            void addShaderRange(
                std::vector<D3D12_DESCRIPTOR_RANGE>& aRanges,
                D3D12_DESCRIPTOR_RANGE_TYPE descriptorRangeType,
                uint32_t iNumDescriptors,
                std::string const& name);
        
            void initDescriptorRange(
                D3D12_DESCRIPTOR_RANGE& descriptorRange,
                D3D12_DESCRIPTOR_RANGE_TYPE type,
                uint32_t iNumDescriptors,
                uint32_t iBaseRegister);

            void initRootParameterAsDescriptorTable(
                D3D12_ROOT_PARAMETER& rootParameter,
                D3D12_DESCRIPTOR_RANGE const* aDescriptorRanges,
                uint32_t iNumDescriptorRanges,
                D3D12_SHADER_VISIBILITY shaderVisibility);

            void initRootSignatureDesc(
                D3D12_VERSIONED_ROOT_SIGNATURE_DESC& rootSignatureDesc,
                D3D12_ROOT_PARAMETER const* pRootParameter,
                uint32_t iNumRootParameters,
                D3D12_STATIC_SAMPLER_DESC const* pSamplerDesc,
                uint32_t iNumSamplerDescs,
                D3D12_ROOT_SIGNATURE_FLAGS flags);

            virtual void createResourceViewOnly(
                PLATFORM_OBJECT_HANDLE descriptorHeapHandle,
                SerializeUtils::Common::ShaderResourceInfo& shaderResource,
                uint32_t iResourceIndex,
                RenderDriver::Common::CDevice& device);

            virtual void createAttachmentView(
                Render::Common::RenderJobInfo& renderJob,
                uint32_t iAttachmentIndex,
                uint32_t iShaderResourceIndex,
                uint32_t iTotalSize,
                bool bBufferAttachment,
                bool bExternalImageAttachment,
                Render::Common::AttachmentType const& viewType,
                RenderDriver::Common::CDevice& device);

            virtual void createWritableAttachmentView(
                Render::Common::RenderJobInfo& renderJob,
                uint32_t iAttachmentIndex,
                uint32_t iShaderResourceIndex,
                uint32_t iTotalSize,
                bool bBufferAttachment,
                bool bExternalImageAttachment,
                Render::Common::AttachmentType const& viewType,
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
                RenderDriver::Common::CDevice& device);

            virtual void platformCreateFrameBuffer(
                Render::Common::RenderJobInfo& renderJobInfo);

            virtual void platformUpdateSamplers(
                Render::Common::RenderJobInfo& renderJobInfo);

            virtual void platformUpdateOutputAttachments(
                Render::Common::RenderJobInfo& renderJobInfo);

        protected:
            std::map<PLATFORM_OBJECT_HANDLE, D3D12_CPU_DESCRIPTOR_HANDLE>               mCPUDescriptorsDX12;
            std::map<PLATFORM_OBJECT_HANDLE, ID3D12Resource*>                           mResourcesDX12;

            //RenderDriver::DX12::CCommandAllocator                                       mUploadDataCommandAllocator;
            //RenderDriver::DX12::CCommandBuffer                                          mUploadDataCommandBuffer;
            //RenderDriver::DX12::CFence                                                  mUploadFence;

            std::vector<RenderDriver::DX12::CBuffer>                                    maUploadBuffers;
};


    }   // DX12

}   // RenderDriver