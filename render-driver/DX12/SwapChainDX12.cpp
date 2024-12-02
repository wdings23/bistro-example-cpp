#include <render-driver/DX12/SwapChainDX12.h>
#include <render-driver/DX12/CommandQueueDX12.h>
#include <render-driver/DX12/PhysicalDeviceDX12.h>
#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/DescriptorHeapDX12.h>
#include <render-driver/DX12/CommandBufferDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

#include <serialize_utils.h>
#include <wtfassert.h>

#include "wrl/client.h"
#include "dxgi1_6.h"
#include "d3d12.h"

#include <assert.h>

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        PLATFORM_OBJECT_HANDLE CSwapChain::create(
			RenderDriver::Common::SwapChainDescriptor& desc,
			RenderDriver::Common::CDevice& device)
        {
			RenderDriver::Common::CSwapChain::create(desc, device);

			RenderDriver::DX12::SwapChainDescriptor const& dx12Desc = static_cast<RenderDriver::DX12::SwapChainDescriptor const&>(desc);

			// swap chain descriptor
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.BufferCount = dx12Desc.miNumBuffers;
			swapChainDesc.Width = dx12Desc.miWidth;
			swapChainDesc.Height = dx12Desc.miHeight;
			swapChainDesc.Format = SerializeUtils::DX12::convert(dx12Desc.mFormat);
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.SampleDesc.Count = dx12Desc.miSamples;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING | DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			swapChainDesc.Stereo = FALSE;

			RenderDriver::DX12::CPhysicalDevice* pPhysicalDevice = static_cast<RenderDriver::DX12::CPhysicalDevice*>(desc.mpPhysicalDevice);
			RenderDriver::DX12::CCommandQueue* pDX12GraphicsCommandQueue = static_cast<RenderDriver::DX12::CCommandQueue*>(desc.mpGraphicsCommandQueue);

			// create
			ComPtr<IDXGISwapChain1> swapChain;
			HRESULT hr = pPhysicalDevice->getNativeFactory()->CreateSwapChainForHwnd(
				static_cast<ID3D12CommandQueue*>(pDX12GraphicsCommandQueue->getNativeCommandQueue()),
				dx12Desc.mHWND,
				&swapChainDesc,
				nullptr,
				nullptr,
				&swapChain);
			assert(SUCCEEDED(hr));

			hr = pPhysicalDevice->getNativeFactory()->MakeWindowAssociation(pPhysicalDevice->getHWND(), DXGI_MWA_NO_ALT_ENTER);
			assert(SUCCEEDED(hr));

			hr = swapChain->QueryInterface(IID_PPV_ARGS(&mNativeSwapChain));
			assert(SUCCEEDED(hr));
			swapChain->Release();
			pPhysicalDevice->getNativeFactory()->Release();
			
			//mNativeSwapChain->SetMaximumFrameLatency(3);
			//mSwapChainWaitableObject = mNativeSwapChain->GetFrameLatencyWaitableObject();

			miFrameIndex = mNativeSwapChain->GetCurrentBackBufferIndex();

			RenderDriver::DX12::CDevice* pNativeDevice = static_cast<RenderDriver::DX12::CDevice*>(desc.mpDevice);
			ID3D12Device* pDeviceDX12 = static_cast<ID3D12Device*>(pNativeDevice->getNativeDevice());

			// color render target descriptor heap
			RenderDriver::Common::DescriptorHeapDescriptor descriptorHeapDesc;
			descriptorHeapDesc.mFlag = RenderDriver::Common::DescriptorHeapFlag::None;
			descriptorHeapDesc.miNumDescriptors = desc.miNumBuffers;
			descriptorHeapDesc.mType = RenderDriver::Common::DescriptorHeapType::RenderTarget;
			mColorRenderTargetDescriptorHeap.create(descriptorHeapDesc, *dx12Desc.mpDevice);

			// create render targets for swap chain 
			RenderDriver::DX12::CDescriptorHeap& dX12DescriptorHeap = static_cast<RenderDriver::DX12::CDescriptorHeap&>(mColorRenderTargetDescriptorHeap);
			dX12DescriptorHeap.setID("Swap Chain Color Descriptor Heap");
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
			rtvHandle = static_cast<ID3D12DescriptorHeap*>(dX12DescriptorHeap.getNativeDescriptorHeap())->GetCPUDescriptorHandleForHeapStart();
			uint32_t iRTVDescriptorSet = pDeviceDX12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			for(uint32_t iBuffer = 0; iBuffer < dx12Desc.miNumBuffers; iBuffer++)
			{
				//hr = mNativeSwapChain->GetBuffer(iBuffer, IID_PPV_ARGS(&maNativeColorRenderTargets[iBuffer]));
				hr = mNativeSwapChain->GetBuffer(
					iBuffer, 
					IID_PPV_ARGS(&maNativeColorRenderTargets[iBuffer]));
				assert(SUCCEEDED(hr));

				D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
				viewDesc.Format = swapChainDesc.Format;
				viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				viewDesc.Texture2D.MipSlice = 0;
				viewDesc.Texture2D.PlaneSlice = 0;

				pDeviceDX12->CreateRenderTargetView(
					maNativeColorRenderTargets[iBuffer].Get(), 
					&viewDesc,
					rtvHandle);
		
				wchar_t szName[256];
				wsprintf(szName, L"Swap Chain Color Render Target %d", iBuffer);

				maNativeColorRenderTargets[iBuffer]->SetName(szName);

				maColorRenderTargets[iBuffer].setNativeImage(maNativeColorRenderTargets[iBuffer]);

				rtvHandle.ptr += iRTVDescriptorSet;
			}
			
			// depth stencil render target descriptor heap
			RenderDriver::Common::DescriptorHeapDescriptor depthStencilDescriptorHeapDesc;
			depthStencilDescriptorHeapDesc.mFlag = RenderDriver::Common::DescriptorHeapFlag::None;
			depthStencilDescriptorHeapDesc.miNumDescriptors = 1;
			depthStencilDescriptorHeapDesc.mType = RenderDriver::Common::DescriptorHeapType::DepthStencil;
			mDepthStencilRenderTargetDescriptorHeap.create(depthStencilDescriptorHeapDesc, *dx12Desc.mpDevice);
			mDepthStencilRenderTargetDescriptorHeap.setID("Swap Chain Depth Stencil Descriptor Heap");

			// depth stencil
			// view descriptor
			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {};
			depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

			// heap properties
			D3D12_HEAP_PROPERTIES depthStencilHeapProp = {};
			depthStencilHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
			depthStencilHeapProp.CreationNodeMask = 1;
			depthStencilHeapProp.VisibleNodeMask = 1;

			// resource descriptor
			D3D12_RESOURCE_DESC depthStencilResouceDesc = {};
			depthStencilResouceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			depthStencilResouceDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			depthStencilResouceDesc.Width = desc.miWidth;
			depthStencilResouceDesc.Height = desc.miHeight;
			depthStencilResouceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			depthStencilResouceDesc.DepthOrArraySize = 1;
			depthStencilResouceDesc.SampleDesc.Count = desc.miSamples;

			// clear value
			D3D12_CLEAR_VALUE depthOptimizeClearValue = {};
			depthOptimizeClearValue.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			depthOptimizeClearValue.DepthStencil.Depth = 1.0;
			depthOptimizeClearValue.DepthStencil.Stencil = 0;
			depthOptimizeClearValue.Color[0] = 1.0f; depthOptimizeClearValue.Color[1] = 0.0f;
			depthOptimizeClearValue.Color[2] = 0.0f; depthOptimizeClearValue.Color[3] = 0.0f;

			RenderDriver::DX12::CDescriptorHeap& dX12DepthStencilDescriptorHeap = static_cast<RenderDriver::DX12::CDescriptorHeap&>(mDepthStencilRenderTargetDescriptorHeap);
			
			// create depth stencil resource
			hr = pDeviceDX12->CreateCommittedResource(
				&depthStencilHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&depthStencilResouceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizeClearValue,
				IID_PPV_ARGS(&mNativeDepthStencilRenderTarget));
			assert(SUCCEEDED(hr));

			mNativeDepthStencilRenderTarget->SetName(L"Swap Chain Depth Stencil Render Target");

			// create depth stencil view
			D3D12_CPU_DESCRIPTOR_HANDLE stencilDepthHandle = static_cast<ID3D12DescriptorHeap*>(dX12DepthStencilDescriptorHeap.getNativeDescriptorHeap())->GetCPUDescriptorHandleForHeapStart();
			pDeviceDX12->CreateDepthStencilView(
				mNativeDepthStencilRenderTarget.Get(),
				&depthStencilViewDesc,
				stencilDepthHandle);

			mDepthStencilRenderTarget.setNativeImage(mNativeDepthStencilRenderTarget);

			miColorRenderTargetDescriptorSize = pDeviceDX12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			miDepthStencilRenderTargetDescriptorSize = pDeviceDX12->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

			miCurrBackBuffer = mNativeSwapChain->GetCurrentBackBufferIndex();

			for(uint32_t i = 0; i < NUM_SWAPCHAIN_RENDER_TARGETS; i++)
			{
				mapColorRenderTargets[i] = &maColorRenderTargets[i];
				mpDepthStencilRenderTarget = &mDepthStencilRenderTarget;
			}

			mNativeSwapChain->SetFullscreenState(
				FALSE,
				nullptr);

			mNativeSwapChain->SetMaximumFrameLatency(16);


			return mHandle;
        }
        
		/*
		**
		*/
		void CSwapChain::transitionRenderTarget(
			RenderDriver::Common::CCommandBuffer& commandBuffer,
			RenderDriver::Common::ResourceStateFlagBits const& before, 
			RenderDriver::Common::ResourceStateFlagBits const& after)
		{
			miCurrBackBuffer = mNativeSwapChain->GetCurrentBackBufferIndex();

			//assert(maRenderTargetStates[miCurrBackBuffer] == before);

			RenderDriver::DX12::CCommandBuffer& dx12CommandBuffer = static_cast<RenderDriver::DX12::CCommandBuffer&>(commandBuffer);

			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = maNativeColorRenderTargets[miCurrBackBuffer].Get();
			barrier.Transition.StateBefore = SerializeUtils::DX12::convert(before);
			barrier.Transition.StateAfter = SerializeUtils::DX12::convert(after);
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			static_cast<ID3D12GraphicsCommandList*>(dx12CommandBuffer.getNativeCommandList())->ResourceBarrier(1, &barrier);

			maRenderTargetStates[miCurrBackBuffer] = after;
		}

		/*
		**
		*/
		void CSwapChain::clear(RenderDriver::Common::ClearRenderTargetDescriptor const& desc)
		{
			RenderDriver::DX12::CCommandBuffer* pCommandBuffer = static_cast<RenderDriver::DX12::CCommandBuffer * >(desc.mpCommandBuffer);

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
			rtvHandle.ptr = static_cast<ID3D12DescriptorHeap*>(mColorRenderTargetDescriptorHeap.getNativeDescriptorHeap())->GetCPUDescriptorHandleForHeapStart().ptr;
			uint64_t iOffset = static_cast<uint64_t>(miColorRenderTargetDescriptorSize) * desc.miFrameIndex;
			rtvHandle.ptr += iOffset;
			
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;
			dsvHandle.ptr = static_cast<ID3D12DescriptorHeap*>(mDepthStencilRenderTargetDescriptorHeap.getNativeDescriptorHeap())->GetCPUDescriptorHandleForHeapStart().ptr;
			
			ID3D12GraphicsCommandList* commandList = static_cast<ID3D12GraphicsCommandList*>(pCommandBuffer->getNativeCommandList());

			commandList->OMSetRenderTargets(
				1,
				&rtvHandle,
				FALSE,
				&dsvHandle);

			commandList->ClearRenderTargetView(
				rtvHandle,
				desc.mafClearColor,
				0,
				nullptr);

			commandList->ClearDepthStencilView(
				dsvHandle,
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
				1.0f,
				0,
				0,
				nullptr);
		}

		/*
		**
		*/
		void CSwapChain::present(RenderDriver::Common::SwapChainPresentDescriptor const& desc)
		{
			WTFASSERT(mNativeSwapChain, "native swap chain is null");
			HRESULT hr = mNativeSwapChain->Present(desc.miSyncInterval, desc.miFlags);
			if(!SUCCEEDED(hr))
			{
				RenderDriver::DX12::Utils::showDebugBreadCrumbs(*static_cast<RenderDriver::DX12::CDevice*>(mDesc.mpDevice));
			}

			miCurrBackBuffer = mNativeSwapChain->GetCurrentBackBufferIndex();
		}

		/*
		**
		*/
		uint32_t CSwapChain::getCurrentBackBufferIndex()
		{
			return mNativeSwapChain->GetCurrentBackBufferIndex();
		}

		/*
		**
		*/
		void* CSwapChain::getNativeSwapChain()
		{
			return mNativeSwapChain.Get();
		}

    }   // DX12

}   // RenderDriver