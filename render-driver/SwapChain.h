#pragma once

#include <render-driver/Object.h>
#include <render-driver/Format.h>
#include <render-driver/Device.h>
#include <render-driver/CommandQueue.h>
#include <render-driver/DescriptorHeap.h>

#include <vector>

#define NUM_SWAPCHAIN_RENDER_TARGETS 3

namespace RenderDriver
{
    namespace Common
    {
        struct SwapChainDescriptor
        {
            uint32_t                        miWidth;
            uint32_t                        miHeight;
            Format                          mFormat;
            uint8_t                         miSamples;
            SwapChainFlagBits               miFlags;
            uint32_t                        miNumBuffers;
            CCommandQueue*                  mpGraphicsCommandQueue;
            CPhysicalDevice*                mpPhysicalDevice;
            CDevice*                        mpDevice;
        };

        struct ClearRenderTargetDescriptor
        {
            CCommandBuffer* mpCommandBuffer;

            uint32_t        miFrameIndex;

            // (RGBA) range: (0.0, 1.0) 
            float mafClearColor[4];
        };

        struct SwapChainPresentDescriptor
        {
            uint32_t                                   miFlags;
            uint32_t                                   miSyncInterval;
            RenderDriver::Common::CDevice*             mpDevice;
            RenderDriver::Common::CCommandQueue*       mpPresentQueue;
        };

        struct CSwapChain : public CObject
        {
        public:
            CSwapChain() = default;
            virtual ~CSwapChain() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                SwapChainDescriptor& desc,
                CDevice& device);
            
            virtual void clear(ClearRenderTargetDescriptor const& desc);

            virtual void present(SwapChainPresentDescriptor const& desc);

            virtual void* getNativeSwapChain();

            virtual uint32_t getCurrentBackBufferIndex() = 0;
            
            virtual void transitionRenderTarget(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::ResourceStateFlagBits const& before,
                RenderDriver::Common::ResourceStateFlagBits const& after);

            inline RenderDriver::Common::CImage* getColorRenderTarget(uint32_t iIndex)
            {
                return mapColorRenderTargets[iIndex];
            }

            inline RenderDriver::Common::CImage* getDepthStencilRenderTarget()
            {
                return mpDepthStencilRenderTarget;
            }
            
            virtual RenderDriver::Common::CImage* getDrawableTexture()
            {
                return nullptr;
            }

        protected:
            SwapChainDescriptor                                     mDesc;

            uint32_t                                                miFrameIndex;
            uint32_t                                                miCurrBackBuffer;

            std::vector<PLATFORM_OBJECT_HANDLE>                     maRenderTargets;
            std::vector<PLATFORM_OBJECT_HANDLE>                     maDepthStencilRenderTargets;

            RenderDriver::Common::CImage*                           mapColorRenderTargets[NUM_SWAPCHAIN_RENDER_TARGETS];
            RenderDriver::Common::CImage*                           mpDepthStencilRenderTarget;
        };


    }   // Common

}   // RenderDriver
