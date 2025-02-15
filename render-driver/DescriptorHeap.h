#pragma once

#include <render-driver/Device.h>

#include <utils/serialize_utils.h>

namespace RenderDriver
{
    namespace Common
    {
        struct DescriptorHeapDescriptor
        {
            DescriptorHeapType          mType;
            DescriptorHeapFlag          mFlag;
            uint32_t                    miNumDescriptors;
        };

        class CDescriptorHeap : public CObject
        {
        public:
            CDescriptorHeap() = default;
            virtual ~CDescriptorHeap() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                DescriptorHeapDescriptor const& desc,
                CDevice& device);

            inline DescriptorHeapType getType()
            {
                return mDesc.mType;
            }

            virtual void* getNativeDescriptorHeap();

            virtual uint64_t getCPUHandle(uint32_t iIndex, RenderDriver::Common::CDevice& device) = 0;
            virtual uint64_t getGPUHandle(uint32_t iIndex, RenderDriver::Common::CDevice& device) = 0;

            virtual void setImageView(
                RenderDriver::Common::CImage* pImage, 
                uint32_t iIndex,
                RenderDriver::Common::CDevice& device) = 0;

            inline DescriptorHeapDescriptor getDescriptor() { return mDesc; }

        protected:
            DescriptorHeapDescriptor        mDesc;
        };

    }   // Common

}   // RenderDriver
