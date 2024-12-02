#pragma once

#include <render-driver/Device.h>
#include <render-driver/DescriptorSet.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        class CDescriptorSet : public RenderDriver::Common::CDescriptorSet
        {
        public:
            struct DescriptorSetLayoutInfo
            {
                uint32_t                        miShaderResourceIndex;
                uint32_t                        miSetIndex;
                uint32_t                        miBindingIndex;
                ShaderResourceType              mType;
            };

        public:

            CDescriptorSet() = default;
            virtual ~CDescriptorSet() = default;

            PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::DescriptorSetDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& name);

            virtual void* getNativeDescriptorSet();
            inline std::vector<VkDescriptorSetLayout>& getNativeDescriptorSetLayouts()
            {
                return maNativeLayoutSets;
            }

            inline std::vector<VkDescriptorSet>& getNativeDescriptorSets()
            {
                return maNativeDescriptorSets;
            }

            inline uint32_t getNumLayoutSets()
            {
                return miNumLayoutSets;
            }

            inline std::vector<DescriptorSetLayoutInfo> const& getLayoutInfo()
            {
                return maLayout;
            }

            void setImageLayout(
                uint32_t iShaderResourceIndex,
                RenderDriver::Common::ImageLayout const& imageLayout);

            inline std::map<uint32_t, RenderDriver::Common::ImageLayout> const& getImageLayouts()
            {
                return maImageLayouts;
            }

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
            VkDescriptorPool                        mNativeDescriptorPool;
            std::vector<VkDescriptorSetLayout>      maNativeLayoutSets;

            std::vector<VkDescriptorSet>            maNativeDescriptorSets;
            uint32_t                                miNumLayoutSets;

            VkDevice*                               mpNativeDevice;

            std::vector<DescriptorSetLayoutInfo>    maLayout;

            std::map<uint32_t, RenderDriver::Common::ImageLayout>       maImageLayouts;
        
            std::vector<std::vector<VkDescriptorSetLayoutBinding>> maaLayoutBindings;
            std::vector<std::vector<RenderDriver::Common::CBuffer*>> maapBuffers;
            std::vector<std::vector<RenderDriver::Common::CImage*>> maapImages;
            std::vector<std::vector<RenderDriver::Common::CImageView*>> maapImageViews;
            std::vector<std::vector<RenderDriver::Common::CAccelerationStructure*>> maapAccelerationStructures;
        };

    }   // Common

}   // RenderDriver