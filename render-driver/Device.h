#pragma once

#include <render-driver/Object.h>
#include <render-driver/PhysicalDevice.h>

#include <memory>

namespace RenderDriver
{
    namespace Common
    {
        struct CCommandBuffer;
        struct CSwapChain;
        struct CCommandQueue;

        class CDevice : public CObject
        {
        public:
            struct CreateDesc
            {
                RenderDriver::Common::CPhysicalDevice*          mpPhyiscalDevice;
            };

        public:
            CDevice() = default;
            virtual ~CDevice() = default;

            virtual PLATFORM_OBJECT_HANDLE create(CreateDesc& createDesc);

            PLATFORM_OBJECT_HANDLE registerSwapChain(CSwapChain* pSwapChain);
            PLATFORM_OBJECT_HANDLE registerCommandQueue(CCommandQueue* pCommandQueue);

            PLATFORM_OBJECT_HANDLE assignHandleAndUpdate();

            virtual void* getNativeDevice();

        protected:
            std::map<PLATFORM_OBJECT_HANDLE, CCommandBuffer*>       mCommandBuffers;
            std::map<PLATFORM_OBJECT_HANDLE, CSwapChain*>           mSwapChains;
            std::map<PLATFORM_OBJECT_HANDLE, CCommandQueue*>        mCommandQueues;

            PLATFORM_OBJECT_HANDLE                                  miCurrHandle;
        };
    }
}

