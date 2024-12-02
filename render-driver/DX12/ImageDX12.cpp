#include <render-driver/DX12/ImageDX12.h>
#include <render-driver/DX12/UtilsDX12.h>

#include <serialize_utils.h>
#include <assert.h>

using namespace Microsoft::WRL;

namespace RenderDriver
{
    namespace DX12
    {
        /*
        **
        */
        PLATFORM_OBJECT_HANDLE CImage::create(
            RenderDriver::Common::ImageDescriptor const& desc,
            RenderDriver::Common::CDevice& device)
        {
            bool bIsDepthStencil = (
                desc.mFormat == RenderDriver::Common::Format::D16_UNORM ||
                desc.mFormat == RenderDriver::Common::Format::D24_UNORM_S8_UINT ||
                desc.mFormat == RenderDriver::Common::Format::D32_FLOAT ||
                desc.mFormat == RenderDriver::Common::Format::D32_FLOAT_S8X24_UINT);

            RenderDriver::Common::CImage::create(desc, device);
            
            RenderDriver::DX12::CDevice& dx12Device = static_cast<RenderDriver::DX12::CDevice&>(device);
            ID3D12Device* pNativeDevice = static_cast<ID3D12Device*>(dx12Device.getNativeDevice());
            
            D3D12_MEMORY_POOL memoryPool = D3D12_MEMORY_POOL_UNKNOWN;
            D3D12_HEAP_PROPERTIES heapProperties;
            heapProperties.Type = SerializeUtils::DX12::convert(desc.mHeapType);
            if(heapProperties.Type == D3D12_HEAP_TYPE_CUSTOM)
            {
                memoryPool = D3D12_MEMORY_POOL_L0;
            }

            heapProperties.CPUPageProperty = SerializeUtils::DX12::convert(desc.mCPUPageProperty);
            heapProperties.MemoryPoolPreference = memoryPool;
            heapProperties.CreationNodeMask = 1;
            heapProperties.VisibleNodeMask = 1;

            // create resource
            D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;
            D3D12_TEXTURE_LAYOUT textureLayout = SerializeUtils::DX12::convert(desc.mLayout);

            mResourceDesc.Width = desc.miWidth;
            mResourceDesc.Height = desc.miHeight;
            mResourceDesc.DepthOrArraySize = desc.miNumImages;
            mResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            mResourceDesc.Format = SerializeUtils::DX12::convert(desc.mFormat);
            mResourceDesc.SampleDesc.Count = 1;
            mResourceDesc.SampleDesc.Quality = 0;
            mResourceDesc.MipLevels = 1;
            mResourceDesc.Layout = textureLayout;
            mResourceDesc.Flags = SerializeUtils::DX12::convert(desc.mResourceFlags);

            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_SHARED;
            if(((uint32_t)desc.mStateFlags & (uint32_t)RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess) != 0)
            {
                mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            }
            else if(desc.mLayout == RenderDriver::Common::TextureLayout::RowMajor)
            {
                mResourceDesc.MipLevels = 1;
            }

            HRESULT hr = S_OK;
            if(bIsDepthStencil)
            {
                // clear value
                D3D12_CLEAR_VALUE depthOptimizeClearValue = {};
                depthOptimizeClearValue.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
                depthOptimizeClearValue.DepthStencil.Depth = 1.0;
                depthOptimizeClearValue.DepthStencil.Stencil = 0;
                depthOptimizeClearValue.Color[0] = 1.0f; depthOptimizeClearValue.Color[1] = 0.0f;
                depthOptimizeClearValue.Color[2] = 0.0f; depthOptimizeClearValue.Color[3] = 0.0f;

                hr = pNativeDevice->CreateCommittedResource(
                    &heapProperties,
                    heapFlags,
                    &mResourceDesc,
                    D3D12_RESOURCE_STATE_DEPTH_WRITE,
                    &depthOptimizeClearValue,
                    IID_PPV_ARGS(&mNativeImage));
            }
            else
            {
                // simultaneous access only on color image
                mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
               
                D3D12_CLEAR_VALUE colorClearValue = {};
                colorClearValue.Format = mResourceDesc.Format;
                //colorClearValue.Color[0] = 0.0f; colorClearValue.Color[1] = 0.0f;
                //colorClearValue.Color[2] = 0.3f; colorClearValue.Color[3] = 0.0f;

                colorClearValue.Color[0] = desc.mafClearColor[0];
                colorClearValue.Color[1] = desc.mafClearColor[1];
                colorClearValue.Color[2] = desc.mafClearColor[2];
                colorClearValue.Color[3] = desc.mafClearColor[3];

                hr = pNativeDevice->CreateCommittedResource(
                    &heapProperties,
                    heapFlags,
                    &mResourceDesc,
                    resourceState,
                    (desc.mResourceFlags == RenderDriver::Common::ResourceFlagBits::AllowRenderTarget) ? &colorClearValue : nullptr,
                    IID_PPV_ARGS(&mNativeImage));
            }

            assert(SUCCEEDED(hr));
            assert(mNativeImage);

            return mHandle;
        }

        /*
        **
        */
        void CImage::setID(std::string const& id)
        {
            RenderDriver::Common::CObject::setID(id);
            std::wstring wName = RenderDriver::DX12::Utils::convertToLPTSTR(id);
            mNativeImage->SetName(wName.c_str());
        }

        /*
        **
        */
        void* CImage::getNativeImage()
        {
            return mNativeImage.Get();
        }

    }   // DX12

}   // RenderDriver