#include "mesh-clusters/DX12/MeshClusterManagerDX12.h"
#include <render-driver/DX12/BufferDX12.h>
#include <render-driver/DX12/CommandAllocatorDX12.h>

#include <sstream>

#include "wtfassert.h"

#define MAX_MESH_INSTANCE_CONSTANT_BUFFERS      128

extern std::atomic<bool>       sbLoading;
extern std::atomic<bool>        sbUploadingToGPU;

namespace Render
{
    namespace DX12
    {
        //struct IndirectDrawCommand
        //{
        //    D3D12_GPU_VIRTUAL_ADDRESS           mConstantBufferViewGPUAddress;
        //    D3D12_GPU_VIRTUAL_ADDRESS           mVertexBufferGPUAddress;
        //    D3D12_GPU_VIRTUAL_ADDRESS           mIndexBufferGPUAddress;
        //    D3D12_DRAW_INDEXED_ARGUMENTS        mDrawArgument;
        //};

        /*
        **
        */
        CMeshClusterManager::~CMeshClusterManager()
        {

        }

        /*
        **
        */
        void CMeshClusterManager::platformSetup(Render::Common::MeshClusterManagerDescriptor const& desc)
        {
            mVertexBufferView.BufferLocation = 0;
            mIndexBufferView.BufferLocation = 0;

            mapMeshInstanceConstantBuffers.resize(MAX_MESH_INSTANCE_CONSTANT_BUFFERS);
            for(uint32_t i = 0; i < static_cast<uint32_t>(mapMeshInstanceConstantBuffers.size()); i++)
            {
                RenderDriver::Common::BufferDescriptor desc;
                desc.miSize = MAX_MESH_INSTANCE_CONSTANT_BUFFERS;
                desc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
                mapMeshInstanceConstantBuffers[i] = std::make_shared<RenderDriver::DX12::CBuffer>();
                uint64_t iHandle = mapMeshInstanceConstantBuffers[i]->create(desc, *mpDevice);
            }

            mUploadCommandAllocator = std::make_shared<RenderDriver::DX12::CCommandAllocator>();
            mUploadCommandBuffer = std::make_shared<RenderDriver::DX12::CCommandBuffer>();

            RenderDriver::Common::CommandAllocatorDescriptor commandAllocatorDesc;
            commandAllocatorDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
            mUploadCommandAllocator->create(commandAllocatorDesc, *mpDevice);
            mUploadCommandAllocator->setID("Cluster Manager Upload Command Allocator");

            RenderDriver::Common::CommandBufferDescriptor commandBufferDesc;
            commandBufferDesc.mpCommandAllocator = mUploadCommandAllocator.get();
            commandBufferDesc.mpPipelineState = nullptr;
            commandBufferDesc.mType = RenderDriver::Common::CommandBufferType::Copy;
            mUploadCommandBuffer->create(commandBufferDesc, *mpDevice);
            mUploadCommandBuffer->setID("Cluster Manager Upload Command Buffer");

            maUploadBuffers.resize(4);
            for(uint32_t i = 0; i < maUploadBuffers.size(); i++)
            {
                maUploadBuffers[i] = std::make_shared<RenderDriver::DX12::CBuffer>();
            }

        }

        /*
        **
        */
        void CMeshClusterManager::initVertexAndIndexBuffers(
            Render::Common::CRenderer& renderer,
            RenderDriver::Common::CDevice& device)
        {
            std::vector<std::unique_ptr<RenderDriver::Common::CBuffer>>     mapTotalMeshVertexBuffers;
            mapTotalMeshVertexBuffers.emplace_back(std::make_unique<RenderDriver::DX12::CBuffer>());

            std::vector<D3D12_INDIRECT_ARGUMENT_DESC> aIndirectIndexedArgumentDescriptors;
            {
                D3D12_INDIRECT_ARGUMENT_DESC indirectConstantBufferViewArgumentDesc = {};
                indirectConstantBufferViewArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
                indirectConstantBufferViewArgumentDesc.ConstantBufferView.RootParameterIndex = 0;
                aIndirectIndexedArgumentDescriptors.push_back(indirectConstantBufferViewArgumentDesc);

                D3D12_INDIRECT_ARGUMENT_DESC indirectShaderConstantArgumentDesc = {};
                indirectShaderConstantArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
                indirectShaderConstantArgumentDesc.Constant.RootParameterIndex = 1;
                indirectShaderConstantArgumentDesc.Constant.DestOffsetIn32BitValues = 0;
                indirectShaderConstantArgumentDesc.Constant.Num32BitValuesToSet = 4;
                aIndirectIndexedArgumentDescriptors.push_back(indirectShaderConstantArgumentDesc);

                D3D12_INDIRECT_ARGUMENT_DESC indirectVertexBufferViewArgumentDesc = {};
                indirectVertexBufferViewArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
                indirectVertexBufferViewArgumentDesc.VertexBuffer.Slot = 0;
                aIndirectIndexedArgumentDescriptors.push_back(indirectVertexBufferViewArgumentDesc);

                D3D12_INDIRECT_ARGUMENT_DESC indirectIndexBufferViewArgumentDesc = {};
                indirectIndexBufferViewArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;
                aIndirectIndexedArgumentDescriptors.push_back(indirectIndexBufferViewArgumentDesc);

                D3D12_INDIRECT_ARGUMENT_DESC indirectIndexedDrawArgumentDesc = {};
                indirectIndexedDrawArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
                aIndirectIndexedArgumentDescriptors.push_back(indirectIndexedDrawArgumentDesc);
            }

            std::vector<D3D12_INDIRECT_ARGUMENT_DESC> aIndirectArgumentDescriptors;
            {
                D3D12_INDIRECT_ARGUMENT_DESC indirectConstantBufferViewArgumentDesc = {};
                indirectConstantBufferViewArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
                indirectConstantBufferViewArgumentDesc.ConstantBufferView.RootParameterIndex = 0;
                aIndirectArgumentDescriptors.push_back(indirectConstantBufferViewArgumentDesc);

                D3D12_INDIRECT_ARGUMENT_DESC indirectShaderConstantArgumentDesc = {};
                indirectShaderConstantArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
                indirectShaderConstantArgumentDesc.Constant.RootParameterIndex = 1;
                indirectShaderConstantArgumentDesc.Constant.DestOffsetIn32BitValues = 0;
                indirectShaderConstantArgumentDesc.Constant.Num32BitValuesToSet = 4;
                aIndirectArgumentDescriptors.push_back(indirectShaderConstantArgumentDesc);

                D3D12_INDIRECT_ARGUMENT_DESC indirectVertexBufferViewArgumentDesc = {};
                indirectVertexBufferViewArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
                indirectVertexBufferViewArgumentDesc.VertexBuffer.Slot = 0;
                aIndirectArgumentDescriptors.push_back(indirectVertexBufferViewArgumentDesc);

                D3D12_INDIRECT_ARGUMENT_DESC indirectIndexedDrawArgumentDesc = {};
                indirectIndexedDrawArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
                aIndirectArgumentDescriptors.push_back(indirectIndexedDrawArgumentDesc);
            }


            struct IndirectIndexedDrawCommand
            {
                // constant buffer view
                uint64_t                        miConstantBufferAddress;

                // constant 
                float4                          mClusterColor;                                  // 16

                // vertex buffer
                uint64_t                        mVertexBufferAddress;                           // 24
                uint32_t                        miDataSize;                                     // 28
                uint32_t                        miStrideInBytes;                                // 32

                // index buffer
                uint64_t                        mIndexBufferAddress;                            // 40
                uint32_t                        miIndexBufferDataSize;                          // 44
                uint32_t                        miIndexBufferFormat;                            // 48

                // draw indexed
                uint32_t                        miNumIndicesPerInstance;                        // 52
                uint32_t                        miNumInstances;                                 // 56
                uint32_t                        miIndexLocation;                                // 60
                uint32_t                        miVertexLocation;                               // 64
                uint32_t                        miInstanceLocation;                             // 68

                //uint32_t                        miPadding;
                uint32_t                        maiPadding[3];                                  // 72

            };

            struct IndirectDrawCommand
            {
                // constant buffer address
                uint64_t                        miConstantBufferAddress;                        // 8

                // constant 
                float4                          mClusterColor;                                  // 24

                // vertex buffer
                uint64_t                        mVertexBufferAddress;                           // 32
                uint32_t                        miDataSize;                                     // 36
                uint32_t                        miStrideInBytes;                                // 40

                // draw arguments, https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_draw_arguments
                uint32_t                        miVertexCountPerInstance;                       // 44
                uint32_t                        miInstanceCount;                                // 48
                uint32_t                        miStartVertexLocation;                          // 52
                uint32_t                        miStartInstanceLocation;                        // 56

                uint32_t                        maiPadding[2];                                  // 64
            };

            //D3D12_COMMAND_SIGNATURE_DESC indirectIndexedCommandSignatureDesc = {};
            //indirectIndexedCommandSignatureDesc.pArgumentDescs = aIndirectIndexedArgumentDescriptors.data();
            //indirectIndexedCommandSignatureDesc.NumArgumentDescs = static_cast<uint32_t>(aIndirectIndexedArgumentDescriptors.size());
            //indirectIndexedCommandSignatureDesc.ByteStride = static_cast<uint32_t>(sizeof(IndirectIndexedDrawCommand));

            D3D12_COMMAND_SIGNATURE_DESC indirectCommandSignatureDesc = {};
            indirectCommandSignatureDesc.pArgumentDescs = aIndirectArgumentDescriptors.data();
            indirectCommandSignatureDesc.NumArgumentDescs = static_cast<uint32_t>(aIndirectArgumentDescriptors.size());
            indirectCommandSignatureDesc.ByteStride = static_cast<uint32_t>(sizeof(IndirectDrawCommand));

#if 0
            RenderDriver::Common::PipelineInfo const* pPipelineInfo = mpRenderer->getSerializer()->getPipelineInfo("Draw Pre Pass Mesh Cluster Graphics");
            if(pPipelineInfo)
            {
                assert(pPipelineInfo);
                RenderDriver::Common::CDescriptorSet& descriptorSet = *(mpRenderer->getSerializer()->getDescriptorSet(pPipelineInfo->mDescriptorHandle).get());
                ID3D12RootSignature* pRootSignature = static_cast<ID3D12RootSignature*>(descriptorSet.getNativeDescriptorSet());

                ID3D12Device* pDX12Device = static_cast<ID3D12Device*>(mpDevice->getNativeDevice());
                HRESULT hr = pDX12Device->CreateCommandSignature(
                    &indirectCommandSignatureDesc,
                    pRootSignature,
                    IID_PPV_ARGS(&mCommandSignature));
                assert(SUCCEEDED(hr));
            }
#endif // #if 0

        }

#if 0
        /*
        **
        */
        void CMeshClusterManager::execIndirectDrawCommands(
            RenderDriver::Common::CCommandBuffer& commandBuffer,
            RenderDriver::Common::Utils::TransitionBarrierInfo& barrierInfo,
            Render::Common::RenderJobInfo const& renderJobInfo)
        {
            if(mCommandSignature == nullptr)
            {
                return;
            }

            if(mVertexBufferView.BufferLocation == 0)
            {
                mVertexBufferView.BufferLocation = mpTotalClusterVertexDataBuffer->getGPUVirtualAddress();
                mVertexBufferView.SizeInBytes = (sizeof(float4) + sizeof(float4) + sizeof(float4)) * 3;
                mVertexBufferView.StrideInBytes = sizeof(float4) + sizeof(float4) + sizeof(float4);
            }

            ID3D12GraphicsCommandList* commandList = static_cast<ID3D12GraphicsCommandList*>(commandBuffer.getNativeCommandList());
            PIXBeginEvent(
                //static_cast<ID3D12CommandQueue*>(mpRenderer->getCommandQueue(RenderDriver::Common::CCommandQueue::Type::Graphics)->getNativeCommandQueue()),
                0xffff8000, 
                renderJobInfo.mName.c_str());

            std::string constantBufferName = renderJobInfo.mName + ".render-pass-constant-buffer";
            RenderDriver::Common::CBuffer* pBuffer = mpRenderer->getSerializer()->getBuffer(constantBufferName);
            WTFASSERT(pBuffer, "can\'t find \"%s\", maybe it did not get registered in associateConstantBufferToShaderResource() in RenderJobSerializer?", constantBufferName.c_str());
            if(pBuffer)
            {
                commandList->SetGraphicsRootConstantBufferView(
                    0,
                    pBuffer->getGPUVirtualAddress());
            }
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
            commandList->IASetIndexBuffer(&mIndexBufferView);

            RenderDriver::Common::CBuffer* pIndirectDrawCommandBuffer = mpIndirectDrawClusterBuffer;
            uint32_t iNumDrawClusters = max(miMaxDrawIndex, miNumDrawClusters);
            if(renderJobInfo.mName == "Draw Pre Pass Mesh Cluster Graphics")
            {
                pIndirectDrawCommandBuffer = mpPrevFrameIndirectDrawClusterBuffer;
                iNumDrawClusters = max(miLastMaxDrawIndex, miLastNumDrawClusters);
            }
            else if(renderJobInfo.mName == "Draw Post Pass Mesh Cluster Graphics")
            {
                pIndirectDrawCommandBuffer = mpPostFrameIndiectDrawClusterBuffer;
                iNumDrawClusters = max(miMaxDrawIndex, miLastMaxDrawIndex);
            }
            else
            {
                pIndirectDrawCommandBuffer = mpPrevFrameIndirectDrawClusterBuffer;
            }

            WTFASSERT(pIndirectDrawCommandBuffer, "no indirect draw command buffer, perhaps not set with the render pass?");
            RenderDriver::DX12::Utils::transitionBarrier(
                *pIndirectDrawCommandBuffer,
                commandBuffer,
                RenderDriver::Common::ResourceStateFlagBits::Common,
                RenderDriver::Common::ResourceStateFlagBits::IndirectArgument);

            commandList->ExecuteIndirect(
                mCommandSignature.Get(),
                //(1 << 16) - 20000,
                1 << 16,
                static_cast<ID3D12Resource*>(pIndirectDrawCommandBuffer->getNativeBuffer()),
                sizeof(uint32_t),
                static_cast<ID3D12Resource*>(pIndirectDrawCommandBuffer->getNativeBuffer()),
                0);

            barrierInfo.mBefore = RenderDriver::Common::ResourceStateFlagBits::IndirectArgument;
            barrierInfo.mAfter = RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess;
            barrierInfo.mpBuffer = pIndirectDrawCommandBuffer;

            //RenderDriver::DX12::Utils::transitionBarrier(
            //    *pIndirectDrawCommandBuffer,
            //    commandBuffer,
            //    RenderDriver::Common::ResourceStateFlagBits::IndirectArgument,
            //    RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess);

            //RenderDriver::DX12::Utils::transitionBarrier(
            //    *pIndirectDrawCommandBuffer,
            //    commandBuffer,
            //    RenderDriver::Common::ResourceStateFlagBits::Common,
            //    RenderDriver::Common::ResourceStateFlagBits::UnorderedAccess);

            PIXEndEvent(
                //static_cast<ID3D12CommandQueue*>(mpRenderer->getCommandQueue(RenderDriver::Common::CCommandQueue::Type::Graphics)->getNativeCommandQueue())
            );
        }
#endif // #if 0

    }   // DX12

}   // Render