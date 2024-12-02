#include <render-driver/Device.h>
#include <render-driver/SwapChain.h>

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CDevice::create(CreateDesc& createDesc)
        {
            mHandle = assignHandleAndUpdate();
            
            return mHandle;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CDevice::registerSwapChain(RenderDriver::Common::CSwapChain* pSwapChain)
        {
            PLATFORM_OBJECT_HANDLE iCurrHandle = miCurrHandle++;
            mSwapChains[iCurrHandle] = pSwapChain;

            return iCurrHandle;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CDevice::registerCommandQueue(RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
            PLATFORM_OBJECT_HANDLE iCurrHandle = miCurrHandle++;
            mCommandQueues[iCurrHandle] = pCommandQueue;

            return iCurrHandle;
        }

        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CDevice::assignHandleAndUpdate()
        {
            return miCurrHandle++;
        }

        /*
        **
        */
        void* CDevice::getNativeDevice()
        { 
            return nullptr; 
        }


    }   // Common

}   // RenderDriver