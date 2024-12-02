#include <render-driver/DX12/DeviceDX12.h>
#include <render-driver/DX12/FenceDX12.h>
#include <render-driver/DX12/UtilsDX12.h>
#include <render-driver/DX12/CommandQueueDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

#include <assert.h>
#include <LogPrint.h>
#include <wtfassert.h>

#include <chrono>

namespace RenderDriver
{
    namespace DX12
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CFence::create(
            RenderDriver::Common::FenceDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            RenderDriver::Common::CFence::create(desc, device);
            
            RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
            ID3D12Device* nativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());

            HRESULT hr = nativeDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mNativeFence));
            assert(SUCCEEDED(hr));

            std::wstring wCommandListName = RenderDriver::DX12::Utils::convertToLPTSTR(mID);
            mNativeFence->SetName(wCommandListName.c_str());

            mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

            return mHandle;
        }

        /*
        **
        */
        void CFence::place(RenderDriver::Common::PlaceFenceDescriptor const& desc)
        {
            RenderDriver::Common::PlaceFenceDescriptor const& dx12Desc = static_cast<RenderDriver::Common::PlaceFenceDescriptor const&>(desc);
            RenderDriver::DX12::CCommandQueue* pCommandQueue = static_cast<RenderDriver::DX12::CCommandQueue*>(dx12Desc.mpCommandQueue);
            
uint64_t iCompletedValue0 = static_cast<ID3D12Fence*>(getNativeFence())->GetCompletedValue();

            //const uint64_t iFenceToWaitFor = mNativeFence->GetCompletedValue() + 1;
            uint64_t const iFenceToWaitFor = desc.miFenceValue;
            static_cast<ID3D12CommandQueue*>(pCommandQueue->getNativeCommandQueue())->Signal(mNativeFence.Get(), iFenceToWaitFor);
       
            miFenceValue = iFenceToWaitFor;

//uint64_t iCompletedValue1 = static_cast<ID3D12Fence*>(getNativeFence())->GetCompletedValue();
//WTFASSERT(iCompletedValue1 >= iCompletedValue0, "Incorrect completed value was %d is %d", iCompletedValue0, iCompletedValue1);

            mPlaceFenceDesc = desc;
        }

        /*
        **
        */
        bool CFence::isCompleted()
        {
            uint64_t iCompletedValue = mNativeFence.Get()->GetCompletedValue();
            return (iCompletedValue >= miFenceValue);
        }

        /*
        **
        */
        void CFence::waitCPU(
            uint64_t iMilliseconds,
            uint64_t iWaitValue)
        {
            if(iWaitValue == UINT64_MAX)
            {
                iWaitValue = miFenceValue;
            }

            uint64_t iCompletedValue = static_cast<ID3D12Fence*>(getNativeFence())->GetCompletedValue();
            
            //if(iCompletedValue < iWaitValue)
            {
auto start = std::chrono::high_resolution_clock::now();
               
                mNativeFence->SetEventOnCompletion(iWaitValue, mFenceEvent);
                
                //PIXNotifyWakeFromFenceSignal(mFenceEvent);
                
                uint32_t iWaitTime = (iMilliseconds == UINT64_MAX) ? INFINITE : static_cast<uint32_t>(iMilliseconds);
                
                //for(;;)
                {
                    DWORD ret = WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
                    if(ret == 0xffffffff)
                    {
                        DWORD errorCode = GetLastError();
                        DEBUG_PRINTF("!!! wait failed returned %X (%d), error code: %X (%d) !!!\n", ret, ret, errorCode, errorCode);
                    }

                    //iCompletedValue = static_cast<ID3D12Fence*>(getNativeFence())->GetCompletedValue();
                    //if(iCompletedValue >= iWaitValue)
                    //{
                    //    break;
                    //}
                }
                
                //ResetEvent(mFenceEvent);
                //CloseHandle(mFenceEvent);

auto elapsed = std::chrono::high_resolution_clock::now() - start;
uint64_t iWaitedUS = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
//DEBUG_PRINTF("waited for fence: %d us\n", iWaitedUS);

            }
            //else
            //{
            //    if(mID == "Graphics Queue Fence")
            //    {
            //        DEBUG_PRINTF("%s fence NO waited value: %d\n", mID.c_str(), iCompletedValue);
            //    }
            //}
        }

        /*
        **
        */
        void CFence::waitCPU2(
            uint64_t iMilliseconds, 
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t iWaitValue)
        {
            uint64_t iCompletedValue = mNativeFence->GetCompletedValue();
            if(iCompletedValue < iWaitValue)
            {
                mNativeFence->SetEventOnCompletion(iWaitValue, mFenceEvent);

                for(uint32_t i = 0; i < 1000; i++)
                {
                    DWORD ret = WaitForSingleObjectEx(mFenceEvent, 1000, FALSE);
                    if(ret == 0xffffffff)
                    {
                        break;
                    }

                    iCompletedValue = mNativeFence->GetCompletedValue();
                    if(iCompletedValue >= iWaitValue)
                    {
                        break;
                    }
                }
                assert(iCompletedValue >= iWaitValue);
                ResetEvent(mFenceEvent);
            }
            
        }

        /*
        **
        */
        void CFence::reset(RenderDriver::Common::CCommandQueue* pCommandQueue)
        {
            //ResetEvent(mFenceEvent);
            //static_cast<ID3D12CommandQueue*>(pCommandQueue->getNativeCommandQueue())->Signal(mNativeFence.Get(), 0);
        }

        /*
        **
        */
        void CFence::waitGPU(
            RenderDriver::Common::CCommandQueue* pCommandQueue, 
            RenderDriver::Common::CFence* pSignalFence,
            uint64_t iWaitValue)
        {
            if(iWaitValue == UINT64_MAX)
            {
                iWaitValue = miFenceValue;
            }

            WTFASSERT(pCommandQueue, "Didn't provide a valid command queue %lld", reinterpret_cast<uint64_t>(pCommandQueue));
            ID3D12Fence* pFenceDX12 = reinterpret_cast<ID3D12Fence*>(getNativeFence());
            static_cast<ID3D12CommandQueue*>(pCommandQueue->getNativeCommandQueue())->Wait(pFenceDX12, iWaitValue);
        }

        /*
        **
        */
        void CFence::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
            mNativeFence->SetName(wName.c_str());
        }

        /*
        **
        */
        uint64_t CFence::getFenceValue()
        {
            ID3D12Fence* pFenceDX12 = reinterpret_cast<ID3D12Fence*>(getNativeFence());
            return pFenceDX12->GetCompletedValue();
        }

        /*
        **
        */
        void* CFence::getNativeFence()
        {
            return mNativeFence.Get();
        }

        /*
        **
        */
        void CFence::signal(
            RenderDriver::Common::CCommandQueue* pCommandQueue, 
            uint64_t const& iWaitFenceValue)
        {
            ID3D12Fence* pFenceDX12 = reinterpret_cast<ID3D12Fence*>(getNativeFence());
            
            ID3D12CommandQueue* pCommandQueueDX12 = static_cast<ID3D12CommandQueue*>(pCommandQueue->getNativeCommandQueue());
            pCommandQueueDX12->Signal(pFenceDX12, iWaitFenceValue);
            
            miFenceValue = iWaitFenceValue;
            mPlaceFenceDesc.mpCommandQueue = pCommandQueue;
            mPlaceFenceDesc.miFenceValue = iWaitFenceValue;
        }



        /*
        **
        */
        void CFence::waitCPU(
            RenderDriver::Common::CCommandQueue* pCommandQueue,
            uint64_t iFenceValue)
        {
            ID3D12Fence* pFenceDX12 = reinterpret_cast<ID3D12Fence*>(getNativeFence());
            ID3D12CommandQueue* pCommandQueueDX12 = static_cast<ID3D12CommandQueue*>(pCommandQueue->getNativeCommandQueue());
            
            UINT64 fenceValue = (UINT64)iFenceValue;
            pCommandQueueDX12->Signal(pFenceDX12, fenceValue);

            UINT64 currFenceValue = pFenceDX12->GetCompletedValue();

            if(currFenceValue < fenceValue)
            {
                pFenceDX12->SetEventOnCompletion(fenceValue, mFenceEvent);
                DWORD ret = WaitForSingleObjectEx(mFenceEvent, INFINITE, FALSE);
            }

            
        }

        /*
        **
        */
        void CFence::waitGPU()
        {
            WTFASSERT(mPlaceFenceDesc.mpCommandQueue != nullptr, "\"%s\"didn\'t set command queue to wait on GPU", mID.c_str());
            ID3D12CommandQueue* pCommandQueueDX12 = static_cast<ID3D12CommandQueue*>(mPlaceFenceDesc.mpCommandQueue->getNativeCommandQueue());
            ID3D12Fence* pFenceDX12 = reinterpret_cast<ID3D12Fence*>(getNativeFence());
            pCommandQueueDX12->Wait(pFenceDX12, mPlaceFenceDesc.miFenceValue);
        }
    }
}