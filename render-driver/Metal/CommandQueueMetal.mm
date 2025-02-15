#include <RenderDriver/Metal/DeviceMetal.h>
#include <RenderDriver/Metal/FenceMetal.h>
#include <RenderDriver/Metal/CommandBufferMetal.h>
#include <RenderDriver/Metal/CommandQueueMetal.h>

#include <wtfassert.h>

namespace RenderDriver
{
    namespace Metal
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CCommandQueue::create(RenderDriver::Common::CCommandQueue::CreateDesc& desc)
        {
            RenderDriver::Metal::CDevice* pDeviceMetal = static_cast<RenderDriver::Metal::CDevice*>(desc.mpDevice);
            mNativeDevice = (__bridge id<MTLDevice>)pDeviceMetal->getNativeDevice();
            
            mNativeCommandQueue = [mNativeDevice newCommandQueue];
            
            mType = desc.mType;
            
            mHandle = pDeviceMetal->registerCommandQueue(this);

            return mHandle;
        }

        /*
        **
        */
        void CCommandQueue::execCommandBuffer(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CDevice& device)
        {
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)commandBuffer.getNativeCommandList();
            [nativeCommandBuffer commit];
            
        }
        
        /*
        **
        */
        void CCommandQueue::execCommandBuffer2(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::CFence& fence,
            uint64_t iFenceValue,
            RenderDriver::Common::CDevice& device)
        {
            
        }

        /*
        **
        */
        void CCommandQueue::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            [mNativeCommandQueue setLabel: [NSString stringWithUTF8String: id.c_str()]];
        }

        /*
        **
        */
        void* CCommandQueue::getNativeCommandQueue()
        {
            return (__bridge void*)mNativeCommandQueue;
        }
        
    }   // Metal

}   // RenderDriver
