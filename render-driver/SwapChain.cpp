#include <render-driver/SwapChain.h>
#include <render-driver/CommandQueue.h>

namespace RenderDriver
{
    namespace Common
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CSwapChain::create(
            SwapChainDescriptor& desc,
            CDevice& device)
        {
            mHandle = device.assignHandleAndUpdate();
            mDesc = desc;

            return mHandle;
        }

        /*
        **
        */
        void CSwapChain::present(SwapChainPresentDescriptor const& desc)
        {

        }

        /*
        **
        */
        void CSwapChain::clear( ClearRenderTargetDescriptor const& desc)
        {

        }

        /*
        **
        */
        void CSwapChain::transitionRenderTarget(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::ResourceStateFlagBits const& before,
            RenderDriver::Common::ResourceStateFlagBits const& after)
        {
        }

        /*
        **
        */
        void* CSwapChain::getNativeSwapChain()
        {
            return nullptr;
        }

    }   // Common

}   // RenderDriver