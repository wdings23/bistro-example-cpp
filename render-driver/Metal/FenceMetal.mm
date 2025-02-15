#include <RenderDriver/Metal/DeviceMetal.h>
#include <RenderDriver/Metal/FenceMetal.h>
//#include <RenderDriver/Metal/UtilsMetal.h>
#include <RenderDriver/Metal/CommandQueueMetal.h>

#include <LogPrint.h>
#include <wtfassert.h>

#include <Metal/Metal.h>
#include <chrono>

namespace RenderDriver
{
    namespace Metal
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CFence::create(
            RenderDriver::Common::FenceDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Metal::CFence::FenceDescriptor const& descMetal = static_cast<RenderDriver::Metal::CFence::FenceDescriptor const&>(desc);
            
            RenderDriver::Common::CFence::create(desc, device);

            RenderDriver::Metal::CDevice& deviceMetal = static_cast<RenderDriver::Metal::CDevice&>(device);
            mNativeDevice = (__bridge id<MTLDevice>)deviceMetal.getNativeDevice();

            mpCommandBuffer = descMetal.mpCommandBuffer;

            mEvent = [mNativeDevice newSharedEvent];
            
            return mHandle;
        }

        /*
        **
        */
        void CFence::place(RenderDriver::Common::PlaceFenceDescriptor const& desc)
        {
            mPlaceFenceDesc = desc;
        }

        /*
        **
        */
        bool CFence::isCompleted()
        {
            return true;
        }

        /*
        **
        */
        void CFence::waitCPU(
            uint64_t iMilliseconds,
            uint64_t iWaitValue)
        {
            WTFASSERT(0, "%s : %d Implement me", __FILE__, __LINE__);
        }

        /*
        **
        */
        void CFence::waitCPU2(
            uint64_t iMilliseconds,
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            RenderDriver::Common::CCommandBuffer* pCommandBuffer,
            uint64_t iWaitValue)
        {
            WTFASSERT(pCommandBuffer, "need command buffer for metal");
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)pCommandBuffer->getNativeCommandList();
            [nativeCommandBuffer waitUntilCompleted];
        }

        /*
        **
        */
        void CFence::reset(RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
            
        }

        /*
        **
        */
        void CFence::waitGPU(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            RenderDriver::Common::CFence* pSignalFence,
            uint64_t iWaitValue)
        {
            WTFASSERT(mpCommandBuffer, "no command buffer given");
            //RenderDriver::Metal::CFence* pSignalFenceMetal = static_cast<RenderDriver::Metal::CFence*>(pSignalFence);
            //id<MTLEvent> signalEvent = pSignalFenceMetal->getEvent();
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)mpCommandBuffer->getNativeCommandList();
            //[nativeCommandBuffer encodeWaitForEvent: signalEvent value: iWaitValue];
            
            [nativeCommandBuffer
             encodeWaitForEvent: mEvent
             value: iWaitValue];
        }

        /*
        **
        */
        void CFence::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
        }

        /*
        **
        */
        uint64_t CFence::getFenceValue()
        {
            return UINT64_MAX;
        }

        /*
        **
        */
        void* CFence::getNativeFence()
        {
            return (__bridge void*)mNativeFence;
        }

        /*
        **
        */
        void CFence::signal(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t const& iSignalFenceValue)
        {
            miFenceValue = iSignalFenceValue;
            mPlaceFenceDesc.mpCommandQueue = pCommandQueue;
            mPlaceFenceDesc.miFenceValue = iSignalFenceValue;

            WTFASSERT(mpCommandBuffer, "no command buffer given");
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)mpCommandBuffer->getNativeCommandList();
            [nativeCommandBuffer encodeSignalEvent: mEvent value: iSignalFenceValue];
        }

        /*
        **
        */
        void CFence::waitCPU(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t iFenceValue)
        {
            WTFASSERT(mpCommandBuffer, "no command buffer given");
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)mpCommandBuffer->getNativeCommandList();
            [nativeCommandBuffer waitUntilCompleted];
        }

        /*
        **
        */
        void CFence::waitGPU()
        {
            WTFASSERT(0, "%s : %d Implement me", __FILE__, __LINE__);
        }
    }
}
