#pragma once

#include <render-driver/Device.h>
#include <render-driver/DescriptorSet.h>

namespace RenderDriver
{
    namespace Metal
    {
        class CDescriptorSet : public RenderDriver::Common::CDescriptorSet
        {
        public:

            CDescriptorSet() = default;
            virtual ~CDescriptorSet() = default;

            PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::DescriptorSetDescriptor const& desc,
                RenderDriver::Common::CDevice& device);

            virtual void setID(std::string const& name);

            inline id<MTLBuffer> getRootConstant()
            {
                return mRootConstant;
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
            
            std::vector<SerializeUtils::Common::ShaderResourceInfo> maShaderResources;
            
        protected:
            id<MTLBuffer>               mRootConstant;
            id<MTLDevice>               mNativeDevice;
            
            std::vector<std::vector<RenderDriver::Common::CBuffer*>> maapBuffers;
            std::vector<std::vector<RenderDriver::Common::CImage*>> maapImages;
            std::vector<std::vector<RenderDriver::Common::CImageView*>> maapImageViews;
            std::vector<std::vector<RenderDriver::Common::CAccelerationStructure*>> maapAccelerationStructures;
            
        };

    }   // Metal

}   // RenderDriver
