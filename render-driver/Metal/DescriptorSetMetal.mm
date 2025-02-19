#include <render-driver/Metal/DeviceMetal.h>
#include <render-driver/Metal/DescriptorSetMetal.h>

#include <utils/LogPrint.h>
#include <sstream>

#include <utils/wtfassert.h>

#define DEBUG_PRINT_OUT 1


namespace RenderDriver
{
    namespace Metal
    {
        PLATFORM_OBJECT_HANDLE CDescriptorSet::create(
            RenderDriver::Common::DescriptorSetDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            // have at most 2 sets for each pipeline
            //        set 0 => vertex shader resources
            //        set 1 => fragment shader resources
            //        set 0 => compute shader resources

            RenderDriver::Common::CDescriptorSet::create(desc, device);

            RenderDriver::Metal::CDevice& deviceMetal = static_cast<RenderDriver::Metal::CDevice&>(device);
            mNativeDevice = (__bridge id<MTLDevice>)deviceMetal.getNativeDevice();

            if(desc.miNumRootConstants > 0)
            {
                mRootConstant = [mNativeDevice
                     newBufferWithLength: 1024
                     options: MTLResourceOptionCPUCacheModeWriteCombined];
            }

            return mHandle;
        }

        /*
        **
        */
        void CDescriptorSet::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            
            [mRootConstant setLabel: [NSString stringWithUTF8String: std::string(id + " Root Constant").c_str()]];
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
            // filler, need this to keep track of the valid buffers
            if(maapBuffers.size() <= iGroup)
            {
                maapBuffers.resize(iGroup + 1);
            }
            if(maapBuffers[iGroup].size() <= iBindingIndex)
            {
                maapBuffers[iGroup].resize(iBindingIndex + 1);
            }
            maapBuffers[iGroup][iBindingIndex] = nullptr;

            if(maapImages.size() <= iGroup)
            {
                maapImages.resize(iGroup + 1);
            }
            if(maapImages[iGroup].size() <= iBindingIndex)
            {
                maapImages[iGroup].resize(iBindingIndex + 1);
            }
            maapImages[iGroup][iBindingIndex] = pImage;

            if(maapImageViews.size() <= iGroup)
            {
                maapImageViews.resize(iGroup + 1);
            }
            if(maapImageViews[iGroup].size() <= iBindingIndex)
            {
                maapImageViews[iGroup].resize(iBindingIndex + 1);
            }
            maapImageViews[iGroup][iBindingIndex] = pImageView;

            if(maapAccelerationStructures.size() <= iGroup)
            {
                maapAccelerationStructures.resize(iGroup + 1);
            }
            if(maapAccelerationStructures[iGroup].size() <= iBindingIndex)
            {
                maapAccelerationStructures[iGroup].resize(iBindingIndex + 1);
            }
            maapAccelerationStructures[iGroup][iBindingIndex] = nullptr;

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
            if(maapBuffers.size() <= iGroup)
            {
                maapBuffers.resize(iGroup + 1);
            }
            if(maapBuffers[iGroup].size() <= iBindingIndex)
            {
                maapBuffers[iGroup].resize(iBindingIndex + 1);
            }
            maapBuffers[iGroup][iBindingIndex] = pBuffer;

            if(maapImages.size() <= iGroup)
            {
                maapImages.resize(iGroup + 1);
            }
            if(maapImages[iGroup].size() <= iBindingIndex)
            {
                maapImages[iGroup].resize(iBindingIndex + 1);
            }
            maapImages[iGroup][iBindingIndex] = nullptr;

            if(maapImageViews.size() <= iGroup)
            {
                maapImageViews.resize(iGroup + 1);
            }
            if(maapImageViews[iGroup].size() <= iBindingIndex)
            {
                maapImageViews[iGroup].resize(iBindingIndex + 1);
            }
            maapImageViews[iGroup][iBindingIndex] = nullptr;

            if(maapAccelerationStructures.size() <= iGroup)
            {
                maapAccelerationStructures.resize(iGroup + 1);
            }
            if(maapAccelerationStructures[iGroup].size() <= iBindingIndex)
            {
                maapAccelerationStructures[iGroup].resize(iBindingIndex + 1);
            }
            maapAccelerationStructures[iGroup][iBindingIndex] = nullptr;
        }

        /*
        **
        */
        void CDescriptorSet::addAccelerationStructure(
            RenderDriver::Common::CAccelerationStructure* pAccelerationStructure,
            uint32_t iBindingIndex,
            uint32_t iGroup)
        {
            if(maapBuffers.size() <= iGroup)
            {
                maapBuffers.resize(iGroup + 1);
            }
            if(maapBuffers[iGroup].size() <= iBindingIndex)
            {
                maapBuffers[iGroup].resize(iBindingIndex + 1);
            }
            maapBuffers[iGroup][iBindingIndex] = nullptr;

            if(maapImages.size() <= iGroup)
            {
                maapImages.resize(iGroup + 1);
            }
            if(maapImages[iGroup].size() <= iBindingIndex)
            {
                maapImages[iGroup].resize(iBindingIndex + 1);
            }
            maapImages[iGroup][iBindingIndex] = nullptr;

            if(maapImageViews.size() <= iGroup)
            {
                maapImageViews.resize(iGroup + 1);
            }
            if(maapImageViews[iGroup].size() <= iBindingIndex)
            {
                maapImageViews[iGroup].resize(iBindingIndex + 1);
            }
            maapImageViews[iGroup][iBindingIndex] = nullptr;

            if(maapAccelerationStructures.size() <= iGroup)
            {
                maapAccelerationStructures.resize(iGroup + 1);
            }
            if(maapAccelerationStructures[iGroup].size() <= iBindingIndex)
            {
                maapAccelerationStructures[iGroup].resize(iBindingIndex + 1);
            }
            maapAccelerationStructures[iGroup][iBindingIndex] = pAccelerationStructure;
        }

    }   // Metal

}   // RenderDriver
