#pragma once

#include <render-driver/Device.h>
#include <render-driver/AccelerationStructure.h>
#include <vector>

#include <utils/serialize_utils.h>

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

            inline DescriptorSetDescriptor& getDesc() { return mDesc; }
            
            inline void setComputeLayoutIndices(std::vector<uint32_t> const& aiLayoutIndices)
            {
                maiComputeLayoutIndices = aiLayoutIndices;
            }
            
            inline void setVertexLayoutIndices(std::vector<uint32_t> const& aiLayoutIndices)
            {
                maiVertexLayoutIndices = aiLayoutIndices;
            }
            
            inline void setFragmentLayoutIndices(std::vector<uint32_t> const& aiLayoutIndices)
            {
                maiFragmentLayoutIndices = aiLayoutIndices;
            }
            
            inline std::vector<uint32_t> const& getComputeLayoutIndices()
            {
                return maiComputeLayoutIndices;
            }
            
            inline std::vector<uint32_t> const& getVertexLayoutIndices()
            {
                return maiVertexLayoutIndices;
            }
            
            inline std::vector<uint32_t> const& getFragmentLayoutIndices()
            {
                return maiFragmentLayoutIndices;
            }
            
        protected:
            DescriptorSetDescriptor             mDesc;
            
            std::vector<uint32_t>               maiVertexLayoutIndices;
            std::vector<uint32_t>               maiFragmentLayoutIndices;
            std::vector<uint32_t>               maiComputeLayoutIndices;
        };

    }   // Common

}   // RenderDriver
