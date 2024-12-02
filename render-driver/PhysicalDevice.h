#pragma once

#include <map>
#include <render-driver/Object.h>

namespace RenderDriver
{
    namespace Common
    {
        class CPhysicalDevice : public CObject
        {
        public:
            enum AdapterType
            {
                Hardware = 0,
                Software,

                NumAdapterTypes
            };

            struct Descriptor
            {
                AdapterType         mAdapterType = Hardware;
                void*               mpAppInstance = nullptr;
            };

        public:
            CPhysicalDevice() = default;
            virtual ~CPhysicalDevice() = default;

            virtual PLATFORM_OBJECT_HANDLE create(Descriptor const& desc);

            virtual void* getNativeDevice()
            {
                return mpNativeDevice;
            }

        protected:
            Descriptor                                                  mDesc;
            void*                                                       mpNativeDevice;
        };


    }   // Common

}   // RenderDriver
