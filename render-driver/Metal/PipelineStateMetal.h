#pragma once

#include <RenderDriver/PipelineState.h>

#import <Metal/Metal.h>

namespace RenderDriver
{
    namespace Metal
    {
        class CPipelineState : public RenderDriver::Common::CPipelineState
        {
        public:
            struct GraphicsPipelineStateDescriptor : public RenderDriver::Common::GraphicsPipelineStateDescriptor
            {
                RenderDriver::Common::Format          maColorAttachmentFormats[8];
                RenderDriver::Common::CImage*   mapColorAttachmentImages[8];
                uint32_t                        miNumColorAttachments;
                
                RenderDriver::Common::CImage*   mpDepthImage;
                
                char const*                     mszVertexEntryName;
                char const*                     mszFragementEntryName;
                char const*                     mszLibraryFilePath;
            };

            struct ComputePipelineStateDescriptor : public RenderDriver::Common::ComputePipelineStateDescriptor
            {
                char const*                     mszComputeEntryName;
                char const*                     mszLibraryFilePath;
            };

        public:
            CPipelineState() = default;
            virtual ~CPipelineState() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::GraphicsPipelineStateDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::ComputePipelineStateDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& name);

            virtual void* getNativePipelineState();

            inline MTLRenderPassDescriptor* getNativeRenderPassDecriptor()
            {
                return mNativeRenderPassDescriptor;
            }
            
            inline id<MTLDepthStencilState> getNativeDepthStencilState()
            {
                return mNativeDepthStencilState;
            }
            
        public:
            static uint32_t constexpr  kiVertexBufferIndex = 30;

        protected:
            id<MTLRenderPipelineState>                mNativeRenderPipelineState;
            id<MTLComputePipelineState>               mNativeComputePipelineState;
            id<MTLDevice>                             mNativeDevice;
            
        protected:
            struct ShaderResourceReflectionInfo
            {
                ShaderResourceReflectionInfo(char const* szName, MTLArgumentType type, RenderDriver::Common::ShaderType const& shaderType)
                {
                    mName = szName;
                    mType = type;
                    mShaderType = shaderType;
                }
                
                std::string                             mName;
                MTLArgumentType                         mType;
                RenderDriver::Common::ShaderType        mShaderType;
            };
            
            std::vector<ShaderResourceReflectionInfo>         maVertexShaderResourceReflectionInfo;
            std::vector<ShaderResourceReflectionInfo>         maFragmentShaderResourceReflectionInfo;
            std::vector<ShaderResourceReflectionInfo>         maComputeShaderResourceReflectionInfo;
            
            std::vector<uint32_t>                       maiComputeShaderResourceLayoutIndices;
            std::vector<uint32_t>                       maiVertexShaderResourceLayoutIndices;
            std::vector<uint32_t>                       maiFragmentShaderResourceLayoutIndices;
            
            MTLRenderPassDescriptor*                 mNativeRenderPassDescriptor;
            MTLDepthStencilDescriptor*               mNativeDepthStencilDescriptor;
            
            id<MTLDepthStencilState>                mNativeDepthStencilState;
            
        protected:
            void setLayoutMapping(
                std::vector<uint32_t>& aiShaderResourceLayoutIndices,
                std::vector<ShaderResourceReflectionInfo>& aReflectionInfo,
                std::vector<SerializeUtils::Common::ShaderResourceInfo> const& aShaderResources);
            
        };


    }   // Metal

}   // RenderDriver
