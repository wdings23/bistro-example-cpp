#pragma once

#include <render-driver/Device.h>
#include <render-driver/AccelerationStructure.h>
#include <vector>

#include <serialize_utils.h>

namespace RenderDriver
{
    namespace Common
    {
        class CImageView;

        struct DescriptorSetDescriptor
        {
            std::vector<SerializeUtils::Common::ShaderResourceInfo> const*      mpaShaderResources;
            PipelineType                                                        mPipelineType;
            uint32_t                                                            miNumRootConstants;
        };

        class CDescriptorSet : public CObject
        {
        public:
            CDescriptorSet() = default;
            virtual ~CDescriptorSet() = default;

            PLATFORM_OBJECT_HANDLE create(
                DescriptorSetDescriptor const& desc,
                CDevice& device);

            virtual void* getNativeDescriptorSet();
            virtual void addImage(
                RenderDriver::Common::CImage* pImage,
                RenderDriver::Common::CImageView* pImageView,
                uint32_t iBindingIndex,
                uint32_t iGroup,
                bool bReadOnly);
            virtual void addBuffer(
                RenderDriver::Common::CBuffer* pBuffer,
                uint32_t iBindingIndex,
                uint32_t iGroup,
                bool bReadOnly);

            virtual void addAccelerationStructure(
                RenderDriver::Common::CAccelerationStructure* pAccelerationStructure,
                uint32_t iBindingIndex,
                uint32_t iGroup);

            virtual void finishLayout(
                void* pSampler
            );

        protected:
            DescriptorSetDescriptor             mDesc;
        };

    }   // Common

}   // RenderDriver