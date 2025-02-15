#include <RenderDriver/Metal/DeviceMetal.h>
#include <RenderDriver/Metal/DescriptorSetMetal.h>

#include <LogPrint.h>
#include <sstream>

#include <wtfassert.h>

#define DEBUG_PRINT_OUT 1


namespace RenderDriver
{
    namespace Metal
    {
        PLATFORM_OBJECT_HANDLE CDescriptorSet::create(
            RenderDriver::Common::DescriptorSetDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            // have at most 2 sets for each pipeline
            //        set 0 => vertex shader resources
            //        set 1 => fragment shader resources
            //        set 0 => compute shader resources

            RenderDriver::Common::CDescriptorSet::create(desc, device);

            RenderDriver::Metal::CDevice& deviceMetal = static_cast<RenderDriver::Metal::CDevice&>(device);
            mNativeDevice = (__bridge id<MTLDevice>)deviceMetal.getNativeDevice();

            if(desc.miNumRootConstants > 0)
            {
                mRootConstant = [mNativeDevice
                     newBufferWithLength: 1024
                     options: MTLResourceOptionCPUCacheModeWriteCombined];
            }

            return mHandle;
        }

        /*
        **
        */
        void CDescriptorSet::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            
            [mRootConstant setLabel: [NSString stringWithUTF8String: std::string(id + " Root Constant").c_str()]];
        }

    }   // Metal

}   // RenderDriver
