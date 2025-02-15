#pragma once

#include <render-driver/Device.h>
#include <render-driver/DescriptorSet.h>

#include <utils/serialize_utils.h>

namespace RenderDriver
{
    namespace Common
    {
        struct GraphicsPipelineStateDescriptor
        {
            CDescriptorSet*                 mpDescriptor;
            uint8_t*                        mpVertexShader;
            uint8_t*                        mpPixelShader;
            uint32_t                        miVertexShaderSize;
            uint32_t                        miPixelShaderSize;
            
            VertexFormat*                   maVertexFormats;
            uint32_t                        miNumVertexMembers;

            BlendState                      maRenderTargetBlendStates[8];
            RasterState                     mRasterState;
            DepthStencilState               mDepthStencilState;
            std::vector<VertexFormat>       maInputFormat;
            TopologyType                    mTopologyType;
            uint32_t                        miNumRenderTarget;
            Format                          maRenderTargetFormats[8] = { static_cast<Format>(UINT32_MAX) };
            Format                          mDepthStencilFormat;
            SampleDescriptor                mSample;

            uint32_t                        miFlags;
            bool                            mbOutputPresent = false;
            bool                            mbFullTrianglePass = false;

            LoadOp                          maLoadOps[8] = {LoadOp::Clear, LoadOp::Clear, LoadOp::Clear, LoadOp::Clear, LoadOp::Clear, LoadOp::Clear, LoadOp::Clear, LoadOp::Clear};
        };

        struct ComputePipelineStateDescriptor
        {
            CDescriptorSet*                 mpDescriptor;
            uint8_t*                        mpComputeShader;
            uint32_t                        miComputeShaderSize;
            uint32_t                        miFlags;
        };

        struct RayTracePipelineStateDescriptor
        {
            CDescriptorSet*                 mpDescriptor;
            
            uint8_t*                        mpRayGenShader;
            uint8_t*                        mpMissShader;
            uint8_t*                        mpCloseHitShader;
            
            uint32_t                        miRayGenShaderSize;
            uint32_t                        miMissShaderSize;
            uint32_t                        miHitShaderSize;

            uint32_t                        miFlags;
        };

        class CPipelineState : public CObject
        {
        public:
            CPipelineState() = default;
            virtual ~CPipelineState() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                GraphicsPipelineStateDescriptor const& desc,
                CDevice& device);

            virtual PLATFORM_OBJECT_HANDLE create(
                ComputePipelineStateDescriptor const& desc,
                CDevice& device);

            virtual PLATFORM_OBJECT_HANDLE create(
                RayTracePipelineStateDescriptor const& desc,
                CDevice& device);

            virtual void* getNativePipelineState();

            inline PipelineType getType()
            {
                return mType;
            }

            inline GraphicsPipelineStateDescriptor& getGraphicsDesc()
            {
                return mGraphicsDesc;
            }

        protected:
            GraphicsPipelineStateDescriptor         mGraphicsDesc;
            ComputePipelineStateDescriptor          mComputeDesc;
            RayTracePipelineStateDescriptor         mRayTraceDesc;
            PipelineType                            mType;
        };


    }   // Commin

}   // RenderDriver
