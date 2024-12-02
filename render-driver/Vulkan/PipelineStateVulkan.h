#pragma once

#include <render-driver/Vulkan/DescriptorSetVulkan.h>
#include <render-driver/PipelineState.h>

#include <vulkan/vulkan.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        class CPipelineState : public RenderDriver::Common::CPipelineState
        {
        public:
            struct GraphicsPipelineStateDescriptor : public RenderDriver::Common::GraphicsPipelineStateDescriptor
            {
                uint32_t miImageWidth = 0;
                uint32_t miImageHeight = 0;
                RenderDriver::Vulkan::CDescriptorSet* mpDescriptorSet = nullptr;
                uint32_t miNumRootConstants = 0;
                float mafRootConstants[16];
            };

            struct ComputePipelineStateDescriptor : public RenderDriver::Common::ComputePipelineStateDescriptor
            {
                uint32_t miNumRootConstants = 0;
                float mafRootConstants[16];
            };

            struct RayTracePipelineStateDescriptor : public RenderDriver::Common::RayTracePipelineStateDescriptor
            {
                void*      mpPlatformInstance;
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

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::RayTracePipelineStateDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& name);

            virtual void* getNativePipelineState();
            void* getNativeRenderPass();

            inline VkPipelineLayout* getNativePipelineLayout()
            {
                return &mNativePipelineLayout;
            }

            inline void setNativeDevice(VkDevice* pNativeDevice)
            {
                mpNativeDevice = pNativeDevice;
            }

        protected:
            VkPipeline           mNativePipelineState;
            VkPipelineLayout     mNativePipelineLayout;

            VkShaderModule       mVertexNativeShaderModule;
            VkShaderModule       mFragmentNativeShaderModule;
            VkShaderModule       mComputeNativeShaderModule;

            VkShaderModule       mRayGenShaderModule;
            VkShaderModule       mMissShaderModule;
            VkShaderModule       mClosestHitShaderModule;

            VkRenderPass         mNativeRenderPass;

            VkDevice*            mpNativeDevice;
        };


    }   // Vulkan

}   // RenderDriver