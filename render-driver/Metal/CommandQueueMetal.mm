#include <render-driver/Metal/DeviceMetal.h>
#include <render-driver/Metal/FenceMetal.h>
#include <render-driver/Metal/CommandBufferMetal.h>
#include <render-driver/Metal/CommandQueueMetal.h>

#include <utils/wtfassert.h>

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
        void CCommandQueue::execCommandBuffer3(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint64_t* piWaitValue,
            uint64_t* piSignalValue,
            RenderDriver::Common::CFence* pWaitFence,
            RenderDriver::Common::CFence* pSignalFence
        )
        {
            id<MTLCommandBuffer> nativeCommandBuffer = (__bridge id<MTLCommandBuffer>)commandBuffer.getNativeCommandList();
            [nativeCommandBuffer commit];
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
