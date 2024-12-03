#pragma once

#include "mesh-clusters/MeshClusterManager.h"
#include <render-driver/DX12/BufferDX12.h>
#include <render/DX12/RendererDX12.h>

namespace Render
{
    namespace DX12
    {
        class CMeshClusterManager : public Render::Common::CMeshClusterManager
        {
        public:
            CMeshClusterManager() = default;
            virtual ~CMeshClusterManager();

            virtual void initVertexAndIndexBuffers(
                Render::Common::CRenderer& renderer,
                RenderDriver::Common::CDevice& device);

#if 0
            virtual void execIndirectDrawCommands(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::Utils::TransitionBarrierInfo& barrierInfo,
                Render::Common::RenderJobInfo const& renderJobInfo);
#endif // #if 0

        protected:
            std::map<uint64_t, std::vector<D3D12_VERTEX_BUFFER_VIEW>>                                    maaVertexBufferView;
            std::map<uint64_t, std::vector<D3D12_INDEX_BUFFER_VIEW>>                                     maaIndexBufferView;
        
            ComPtr<ID3D12CommandSignature>                                                               mCommandSignature;

            D3D12_VERTEX_BUFFER_VIEW        mVertexBufferView;
            D3D12_INDEX_BUFFER_VIEW         mIndexBufferView;

        protected:
            void platformSetup(Render::Common::MeshClusterManagerDescriptor const& desc);

        };


    }   // DX12

}   // Render