#pragma once

#include <render-driver/Device.h>
#include <serialize_utils.h>

namespace RenderDriver
{
    namespace Common
    {
        struct CommandAllocatorDescriptor
        {
            CommandBufferType                mType;
        };

        class CCommandAllocator : public CObject
        {
        public:
            CCommandAllocator() = default;
            virtual ~CCommandAllocator() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                CommandAllocatorDescriptor const& desc,
                CDevice& device);
            
            virtual void reset();

            virtual void* getNativeCommandAllocator();

        protected:
            CommandAllocatorDescriptor      mDesc;
        
        };


    }   // Common

}   // RenderDriver