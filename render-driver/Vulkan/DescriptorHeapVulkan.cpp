#include <render-driver/Vulkan/DescriptorHeapVulkan.h>
#include <render-driver/Vulkan/UtilsVulkan.h>

#include <serialize_utils.h>
#include <wtfassert.h>

namespace RenderDriver
{
    namespace Vulkan
    {
        PLATFORM_OBJECT_HANDLE CDescriptorHeap::create(
            RenderDriver::Common::DescriptorHeapDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CDescriptorHeap::create(desc, device);

            return mHandle;
        }

        /*
        **
        */
        void CDescriptorHeap::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
        }

        /*
        **
        */
        void* CDescriptorHeap::getNativeDescriptorHeap()
        {
            return nullptr;
        }

        /*
        **
        */
        uint64_t CDescriptorHeap::getCPUHandle(uint32_t iIndex, RenderDriver::Common::CDevice& device)
        {
            return 0;
        }

        /*
        **
        */
        uint64_t CDescriptorHeap::getGPUHandle(uint32_t iIndex, RenderDriver::Common::CDevice& device)
        {
            return 0;
        }

        /*
        **
        */
        void CDescriptorHeap::setImageView(
            RenderDriver::Common::CImage* pImage,
            uint32_t iIndex,
            RenderDriver::Common::CDevice& device)
        {
            WTFASSERT(0, "implement me");
        }

    }   // Vulkan

}   // RenderDriver