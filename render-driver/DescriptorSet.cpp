#include <render-driver/DescriptorSet.h>

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CDescriptorSet::create(
            DescriptorSetDescriptor const& desc,
            CDevice& device)
        {
            mDesc = desc;
            mHandle = device.assignHandleAndUpdate();

            return mHandle;
        }

        /*
        **
        */
        void* CDescriptorSet::getNativeDescriptorSet()
        {
            return nullptr;
        }

        /*
        **
        */
        void CDescriptorSet::addImage(
            RenderDriver::Common::CImage* pImage,
            RenderDriver::Common::CImageView* pImageView,
            uint32_t iBindingIndex,
            uint32_t iGroup,
            bool bReadOnly)
        {

        }

        /*
        **
        */
        void CDescriptorSet::addBuffer(
            RenderDriver::Common::CBuffer* pBuffer,
            uint32_t iBindingIndex,
            uint32_t iGroup,
            bool bReadOnly)
        {

        }

        /*
        **
        */
        void CDescriptorSet::addAccelerationStructure(
            RenderDriver::Common::CAccelerationStructure* pAccelerationStructure,
            uint32_t iBindingIndex,
            uint32_t iGroup)
        {

        }

        /*
        **
        */
        void CDescriptorSet::finishLayout(void* pSampler)
        {

        }

    }   // Common

}   // RenderDriver