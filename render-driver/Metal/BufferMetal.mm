#include <render-driver/Metal/BufferMetal.h>
#include <render-driver/Metal/DeviceMetal.h>
//#include <RenderDriver/Metal/UtilsMetal.h>
#include <render-driver/Metal/CommandBufferMetal.h>

#include <utils/serialize_utils.h>
#include <utils/LogPrint.h>
#include <utils/wtfassert.h>


namespace RenderDriver
{
    namespace Metal
    {
        PLATFORM_OBJECT_HANDLE CBuffer::create(
            RenderDriver::Common::BufferDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CBuffer::create(desc, device);

            RenderDriver::Metal::CDevice& deviceMetal = static_cast<RenderDriver::Metal::CDevice&>(device);
            mNativeDevice = (__bridge id<MTLDevice>)deviceMetal.getNativeDevice();
            
            bool bShared = ((static_cast<uint32_t>(desc.mBufferUsage) & static_cast<uint32_t>(RenderDriver::Common::BufferUsage::TransferSrc)) > 0);
            
            MTLResourceOptions options = (bShared) ? MTLResourceStorageModeShared : MTLResourceStorageModePrivate;
            mNativeBuffer = [mNativeDevice newBufferWithLength:desc.miSize options: options];
            
            mDesc = desc;
            
            return mHandle;
        }

    CBuffer::~CBuffer()
    {
        releaseNativeBuffer();
    }
        
        /*
        **
        */
        void CBuffer::copy(
            RenderDriver::Common::CBuffer& buffer,
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            uint32_t iDestOffset,
            uint32_t iSrcOffset,
            uint32_t iDataSize)
        {
            id<MTLBuffer> srcNativeBuffer = (__bridge id<MTLBuffer>)buffer.getNativeBuffer();
            
            RenderDriver::Metal::CCommandBuffer& commandBufferMetal = static_cast<RenderDriver::Metal::CCommandBuffer&>(commandBuffer);
            commandBufferMetal.beginCopy();
            id<MTLBlitCommandEncoder> blitEncoder = commandBufferMetal.getNativeBlitCommandEncoder();
            WTFASSERT(blitEncoder != nil, "invalid blit encoder");
            
            [blitEncoder
                copyFromBuffer: srcNativeBuffer
                sourceOffset: iSrcOffset
                toBuffer: mNativeBuffer
                destinationOffset: iDestOffset
                size: iDataSize];
        }

        /*
        **
        */
        void CBuffer::setDataOpen(
            void* pSrcData,
            uint32_t iDataSize)
        {
            bool bShared = ((static_cast<uint32_t>(mDesc.mBufferUsage) & static_cast<uint32_t>(RenderDriver::Common::BufferUsage::TransferSrc)) > 0);
            WTFASSERT(bShared, "Can'\t set data directly for private memory buffer");
            
            memcpy(
                mNativeBuffer.contents,
                pSrcData,
                iDataSize);
            
        }

        /*
        **
        */
        void CBuffer::setData(
            void* pSrcData,
            uint32_t iDataSize)
        {
            bool bShared = ((static_cast<uint32_t>(mDesc.mBufferUsage) & static_cast<uint32_t>(RenderDriver::Common::BufferUsage::TransferSrc)) > 0);
            WTFASSERT(bShared, "Can'\t set data directly for private memory buffer");
            
            memcpy(
                mNativeBuffer.contents,
                pSrcData,
                iDataSize);
        }

        /*
        **
        */
        void CBuffer::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            [mNativeBuffer setLabel: [NSString stringWithUTF8String: id.c_str()]];
            
        }

        /*
        **
        */
        void* CBuffer::getNativeBuffer()
        {
            return (__bridge void*)mNativeBuffer;
        }

        /*
        **
        */
        uint64_t CBuffer::getGPUVirtualAddress()
        {
            return 0;
        }

        /*
        **
        */
        void CBuffer::releaseNativeBuffer()
        {
            [mNativeBuffer setPurgeableState:MTLPurgeableStateEmpty];
            mNativeBuffer = nil;
        }

    }   // DX12

}   // RenderDriver
