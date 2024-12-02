#pragma once

#include <render-driver/SwapChain.h>
#include <render-driver/DX12/DescriptorHeapDX12.h>
#include <render-driver/DX12/ImageDX12.h>
#include <render-driver/CommandBuffer.h>

namespace RenderDriver
{
    namespace DX12
    {
        struct SwapChainDescriptor : public RenderDriver::Common::SwapChainDescriptor
        {
            HWND                            mHWND;
        };

        class CSwapChain : public RenderDriver::Common::CSwapChain
        {
        public:
            CSwapChain() = default;
            virtual ~CSwapChain() = default;

            virtual PLATFORM_OBJECT_HANDLE create(
                RenderDriver::Common::SwapChainDescriptor& desc,
                RenderDriver::Common::CDevice& device);
            
            virtual void clear(RenderDriver::Common::ClearRenderTargetDescriptor const& desc);

            virtual void present(RenderDriver::Common::SwapChainPresentDescriptor const& desc);

            inline ComPtr<ID3D12Resource>& getNativeColorRenderTarget(uint32_t iIndex)
            {
                return maNativeColorRenderTargets[iIndex];
            }

            inline ComPtr<ID3D12Resource>& getNativeDepthStencilRenderTarget()
            {
                return mNativeDepthStencilRenderTarget;
            }

            CImage& getColorRenderTarget(uint32_t iIndex)
            {
                return maColorRenderTargets[iIndex];
            }

            CImage& getDepthStencilRenderTarget()
            {
                return mDepthStencilRenderTarget;
            }

            virtual void transitionRenderTarget(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::ResourceStateFlagBits const& before,
                RenderDriver::Common::ResourceStateFlagBits const& after);

            RenderDriver::DX12::CDescriptorHeap& getColorRenderTargetDescriptorHeap()
            {
                return mColorRenderTargetDescriptorHeap;
            }

            RenderDriver::DX12::CDescriptorHeap& getDepthStencilRenderTargetDescriptorHeap()
            {
                return mDepthStencilRenderTargetDescriptorHeap;
            }

            virtual void* getNativeSwapChain();

            virtual uint32_t getCurrentBackBufferIndex();

        protected:
            ComPtr<IDXGISwapChain3>                                 mNativeSwapChain;
            HANDLE													mSwapChainWaitableObject;

            RenderDriver::DX12::CImage                              maColorRenderTargets[NUM_SWAPCHAIN_RENDER_TARGETS];
            RenderDriver::DX12::CImage                              mDepthStencilRenderTarget;

            ComPtr<ID3D12Resource>						            maNativeColorRenderTargets[NUM_SWAPCHAIN_RENDER_TARGETS];
            ComPtr<ID3D12Resource>					                mNativeDepthStencilRenderTarget;
            
            RenderDriver::DX12::CDescriptorHeap                     mColorRenderTargetDescriptorHeap;
            RenderDriver::DX12::CDescriptorHeap                     mDepthStencilRenderTargetDescriptorHeap;
        
            uint32_t                                                miColorRenderTargetDescriptorSize;
            uint32_t                                                miDepthStencilRenderTargetDescriptorSize;
        
            RenderDriver::Common::ResourceStateFlagBits             maRenderTargetStates[NUM_SWAPCHAIN_RENDER_TARGETS];
        };

    }   // Common

}   // RenderDriver