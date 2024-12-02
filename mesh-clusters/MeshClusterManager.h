#pragma once

#include "vec.h"
#include <render-driver/Device.h>
#include <render/Renderer.h>
#include <mesh-clusters/mesh_cluster.h>
#include <mesh-clusters/cluster_tree.h>

#include <string>
#include <vector>
#include <memory>
#include <chrono>

namespace Render
{
    namespace Common
    {
        struct MeshClusterManagerDescriptor
        {
            RenderDriver::Common::CDevice*                                  mpDevice;
            Render::Common::CRenderer*                                      mpRenderer;
        };

        struct MeshInfo
        {
            uint32_t        miID;
            uint32_t        miClusterGroupNodeStartIndex;
            uint32_t        miClusterNodeStartIndex;
            uint32_t        miNumClusterGroups;
            uint32_t        miNumClusters;
            uint32_t        miMaxMIPLevel;
            uint32_t        miClusterRootNodeStartIndex;
            uint32_t        miNumRootClusters;
            uint32_t        miClusterGroupRootNodeStartIndex;
            uint32_t        miNumRootClusterGroups;
            float3          mMinBounds;
            float3          mMaxBounds;
        };

        struct MeshInstanceInfo
        {
            uint32_t                miMesh;
            uint32_t                miInstance;
            float4x4                mTransformMatrix;
            float4x4                mRotationMatrix;
            uint32_t                miClusterStartIndex;
            uint32_t                miClusterGroupStartIndex;
            uint32_t                miRootClusterStartIndex;
            uint32_t                miTotalMeshInstanceIndex;

            MeshInstanceInfo() = default;

            MeshInstanceInfo(
                uint32_t iMesh,
                uint32_t iInstance,
                float4x4 const& transformMatrix,
                float4x4 const& rotationMatrix)
            {
                miMesh = iMesh;
                miInstance = iInstance;
                mTransformMatrix = transformMatrix;
                mRotationMatrix = rotationMatrix;
            }
        };

        struct OctNodeMeshInfo
        {
            uint32_t                        miMesh = UINT32_MAX;
            uint32_t                        miMeshInstance = UINT32_MAX;

            OctNodeMeshInfo() = default;

            OctNodeMeshInfo(uint32_t iMeshInstanceID)
            {
                miMeshInstance = iMeshInstanceID;
            }
        };

        struct OctNode
        {
            float3                          mCenter = float3(0.0f, 0.0f, 0.0f);
            float3                          mMaxBounds = float3(0.0f, 0.0f, 0.0f);
            float3                          mMinBounds = float3(0.0f, 0.0f, 0.0f);
            uint32_t                        maiChildren[8];
            uint32_t                        miNumChildren = 0;
            uint32_t                        miParent = UINT32_MAX;

            uint32_t                        miID;
            
            OctNodeMeshInfo                 maMeshInstanceInfo[256];
            uint32_t                        miNumEmbeddedMeshInstances = 0;

            OctNode()
            {
                memset(maiChildren, 0xff, (sizeof(uint32_t) / sizeof(char)) * 8);
            }
        };

        struct OctTree
        {
            std::vector<OctNode>            maOctNodes;
        };

        class CMeshClusterManager
        {
        public:
            CMeshClusterManager() = default;
            virtual ~CMeshClusterManager();

            void setup(MeshClusterManagerDescriptor const& desc);
            
            void initMeshClusters(uint32_t iMesh);

            virtual void initVertexAndIndexBuffers(
                Render::Common::CRenderer& renderer,
                RenderDriver::Common::CDevice& device) = 0;

            virtual void update();

            virtual void execIndirectDrawCommands(
                RenderDriver::Common::CCommandBuffer& commandBuffer,
                RenderDriver::Common::Utils::TransitionBarrierInfo& barrierInfo,
                Render::Common::RenderJobInfo const& renderJobInfo) = 0;

            inline RenderDriver::Common::CBuffer* getIndirectDrawCommandBuffer()
            {
                return mpIndirectDrawClusterBuffer;
            }

            void loadMeshes(std::string const& directory);

        public:
            static std::unique_ptr<CMeshClusterManager>& instance();
            static std::unique_ptr<CMeshClusterManager>& instance(std::unique_ptr<Render::Common::CMeshClusterManager>& pPtr);

        protected:
            RenderDriver::Common::CDevice*                                                                     mpDevice;
            Render::Common::CRenderer*                                                                         mpRenderer;

            //std::map<uint64_t, std::vector<std::shared_ptr<RenderDriver::Common::CBuffer>>>                    mapTotalMeshesVertexBuffers;
            //std::map<uint64_t, std::vector<std::shared_ptr<RenderDriver::Common::CBuffer>>>                    mapTotalMeshesIndexBuffers;
            //std::map<uint64_t, std::shared_ptr<RenderDriver::Common::CBuffer>>                                 mapIndirectDrawCommandBuffer;
            //std::map<uint64_t, std::shared_ptr<RenderDriver::Common::CBuffer>>                                 mapIndirectDrawConstantBuffer;

            //std::shared_ptr<RenderDriver::Common::CBuffer> mpTotalVertexBuffer;
            //std::shared_ptr<RenderDriver::Common::CBuffer> mpTotalIndexBuffer;

            std::map<uint64_t, std::vector<uint64_t>>       maaiTotalVertexBufferOffsetBytes;
            std::map<uint64_t, std::vector<uint64_t>>       maaiTotalIndexBufferOffsetBytes;

            std::map<uint32_t, uint32_t>                    maiVertexDataBufferAddress;
            std::map<uint32_t, uint32_t>                    maiIndexBufferAddress;

            uint32_t                                        miCurrVertexDataOffset = 0;
            uint32_t                                        miCurrIndexDataOffset = 0;

            std::map<uint32_t, std::vector<ClusterTreeNode>>             maClusterTreeNodes;
            std::map<uint32_t, std::vector<ClusterGroupTreeNode>>        maClusterGroupTreeNodes;

            std::map<uint32_t, std::vector<MeshCluster>>                 maMeshClusters;

            std::map<uint32_t, std::vector<uint32_t>>                maaiNumClusterVertices;
            std::map<uint32_t, std::vector<uint32_t>>                maaiNumClusterIndices;
            std::map<uint32_t, std::vector<uint64_t>>                maaiVertexBufferArrayOffsets;
            std::map<uint32_t, std::vector<uint64_t>>                maaiIndexBufferArrayOffsets;

            std::map<uint32_t, std::vector<ClusterTreeNode>>         maaRootClusterNodes;

            std::map<uint32_t, std::vector<uint32_t>>                maaiTriangleVertexBufferArrayOffsets;

            std::map<uint32_t, uint32_t>                             maiTriangleDataOffset;
            uint32_t                                                 miCurrDataTOCOffset = 0;

            std::map<uint32_t, uint32_t>                            maiClusterNodeDataOffset;
            uint32_t                                                miCurrClusterNodeDataOffset = 0;

            std::map<uint32_t, uint32_t>                            maiClusterGroupNodeDataOffset;
            uint32_t                                                miCurrClusterGroupNodeDataOffset = 0;

            std::map<std::string, uint32_t>                         maAttachmentToShaderResource;

            RenderDriver::Common::CBuffer*                          mpClusterGroupTreeNodeBuffer;
            RenderDriver::Common::CBuffer*                          mpClusterTreeNodeBuffer;
            RenderDriver::Common::CBuffer*                          mpClusterDrawCommandBuffer;
            RenderDriver::Common::CBuffer*                          mpDrawClusterInfoBuffer;
            RenderDriver::Common::CBuffer*                          mpIndirectDrawClusterBuffer;
            RenderDriver::Common::CBuffer*                          mpClusterDataTOCBuffer;
            RenderDriver::Common::CBuffer*                          mpTotalClusterVertexDataBuffer;
            RenderDriver::Common::CBuffer*                          mpTotalClusterConstantBuffer;
            RenderDriver::Common::CBuffer*                          mpPrevFrameIndirectDrawClusterBuffer;
            RenderDriver::Common::CBuffer*                          mpPostFrameIndiectDrawClusterBuffer;
            RenderDriver::Common::CBuffer*                          mpVisitedBuffer;
            RenderDriver::Common::CBuffer*                          mpClearBuffer;
            RenderDriver::Common::CBuffer*                          mpMeshInstanceInfoBuffer;
            RenderDriver::Common::CBuffer*                          mpConstantBufferInfoAddressBuffer;
            RenderDriver::Common::CBuffer*                          mpPrePassCountBuffer;
            RenderDriver::Common::CBuffer*                          mpPostPassCountBuffer;
            RenderDriver::Common::CBuffer*                          mpMeshInfoBuffer;
            RenderDriver::Common::CBuffer*                          mpRootClusterInfoBuffer;
            RenderDriver::Common::CBuffer*                          mpVisibilityCountBuffer;
            RenderDriver::Common::CBuffer*                          mpClusterQueueBuffer;
            RenderDriver::Common::CBuffer*                          mpClusterTraverseBuffer;
            RenderDriver::Common::CBuffer*                          mpInitialClusterGroupBuffer;
            RenderDriver::Common::CBuffer*                          mpClusterDataIndexHashMap;
            RenderDriver::Common::CBuffer*                          mpTotalTraverseClusterAddressBuffer;
            RenderDriver::Common::CBuffer*                          mpMeshInstancesToDrawBuffer;

            RenderDriver::Common::CBuffer*                          mpPrePassDebugBuffer;
            RenderDriver::Common::CBuffer*                          mpPostPassDebugBuffer;
            RenderDriver::Common::CBuffer*                          mpVisibilityDebugBuffer;

            RenderDriver::Common::CBuffer*                          mpInitialMeshInstanceRootClusterBuffer;
            
            RenderDriver::Common::CBuffer*                          mpIndirectDrawCommandBuffer;

            // read back data from GPU
            struct DrawClusterInfo
            {
                uint32_t            miMesh;
                uint32_t            miInstance;
                uint32_t            miCluster;
                uint32_t            miTreeNode;
                uint32_t            miTree;
                uint32_t            miVisibilityBits;
                uint32_t            miTotalMeshInstanceIndex;
            };

            std::vector<DrawClusterInfo>                            maDrawClusterInfo;
            uint32_t                                                miNumDrawClusters = 0;

            struct RenderJobCallBackData
            {
                Render::Common::CRenderer* mpRenderer;
                Render::Common::CMeshClusterManager* mpMeshClusterManager;
            };

            RenderJobCallBackData                                   mRenderJobCallbackData;

            struct UploadClusterInfo
            {
                uint32_t                miFrameIndex = 0;
                uint32_t                miMesh = 0;
                uint32_t                miCluster = 0;
                uint32_t                miVertexBufferSize = 0;
                uint32_t                miVertexBufferOffset = 0;
                uint32_t                miAllocatedVertexBufferSize = 0;
                uint32_t                miLoaded = 0;
            };

            std::vector<UploadClusterInfo> maUpdatingClusterInfo;
            std::vector<UploadClusterInfo> maUploadClusterInfoGPU;

            std::vector<UploadClusterInfo> maClusterInfoToLoad;

            std::vector<std::vector<ConvertedMeshVertexFormat>>   maaQueuedClusterTriangleVertexData;
            std::vector<std::vector<uint32_t>>                  maaQueuedClusterTriangleIndexData;
            std::vector<std::pair<uint32_t, uint32_t>>          maQueuedDataOffsets;

            std::vector<std::pair<uint32_t, uint32_t>>          maUploadDataOffsets;
            std::vector<std::vector<ConvertedMeshVertexFormat>>   maaUploadClusterTriangleVertexData;
            std::vector<std::vector<uint32_t>>                  maaUploadClusterTriangleIndexData;
            
            uint32_t miMaxDrawIndex = 0;
            
            uint32_t miLastMaxDrawIndex = 0;
            uint32_t miLastNumDrawClusters;

            float4x4 mLastViewMatrix;

            
            
            std::vector<MeshInfo>                                                  maMeshInfo;
            std::vector<MeshInstanceInfo>                                          maMeshInstanceInfo;

            std::vector<std::shared_ptr<RenderDriver::Common::CBuffer>>          mapMeshInstanceConstantBuffers;
            uint32_t                                                             miNumMeshInstances;

            std::string                                                          mAssetDirectory;

            std::vector<std::string>                                             maMeshFileDirectories;

            struct MeshClusterTreeQueueInfo
            {
                uint32_t                miMesh;
                uint32_t                miInstance;
                uint32_t                miClusterAddress;
                uint32_t                miAbsoluteInstanceIndex;
            };
            std::vector<MeshClusterTreeQueueInfo>                              maClusterQueueInfo;
            std::vector<std::vector<MeshClusterTreeQueueInfo>>                 maaClusterQueueInfo;

            struct MeshInstanceClusterGroupInfo
            {
                uint32_t                miClusterGroupAddress;
                uint32_t                miMeshInstanceIndex;
            };
            std::vector<MeshInstanceClusterGroupInfo>   maTotalClusterGroupIndices;

            uint32_t            miUploadStartIndex;

            std::string         mFirstRenderJobName;

            uint32_t    miNumSwapped = 0;

            std::vector<uint8_t> macUploadConstantBufferData;
            std::vector<uint64_t> maiConstantBufferAddress;
            std::vector<uint32_t> maiVisibilityCounterData;

            struct UpdateDataInfo
            {
                RenderDriver::Common::CBuffer*      mpBuffer;
                uint32_t                            miSrcDataSize;
                uint32_t                            miSrcDataOffset;
                uint32_t                            miDestDataOffset;
                void*                               mpSrcData;
            };

            std::vector<UpdateDataInfo> maPreCacheDataInfo;
            std::vector<std::vector<uint8_t>> maacMeshInstanceConstantBufferData;

            std::shared_ptr<RenderDriver::Common::CCommandAllocator> mUploadCommandAllocator;
            std::shared_ptr<RenderDriver::Common::CCommandBuffer> mUploadCommandBuffer;
            std::vector<std::shared_ptr<RenderDriver::Common::CBuffer>> maUploadBuffers;

            std::vector<uint8_t>    maTempDrawClusterInfo;
            std::vector<uint8_t>    maTempDrawClusterInfo2;

            std::vector<char>       maQueuedClusterHashMap;
            std::vector<char>       maLoadedClusterHashMap;

            std::vector<DrawClusterInfo> maNewClustersToLoad;
            std::vector<DrawClusterInfo> maQueuedNewClusterToLoad;

            std::vector<std::vector< DrawClusterInfo>> maaNewClustersToLoadQueue;

            uint32_t    miNumQueueThreads = 0;

            std::chrono::high_resolution_clock::time_point mLastLoadTime;

        protected:
            bool addUploadClusterData(
                uint32_t& iVertexBufferDataOffset,
                uint32_t& iIndexBufferDataOffset,
                std::vector<ConvertedMeshVertexFormat> const& aClusterTriangleVertices,
                std::vector<uint32_t> const& aiClusterTriangleIndices,
                uint32_t iMesh,
                uint32_t iCluster,
                uint32_t iClusterVertexDataMemoryLimit);


            virtual void platformSetup(MeshClusterManagerDescriptor const& desc) = 0;

            static void loadClusterData(void* pData, uint32_t iWorkerID);
            static void finishRenderJobCallback(std::string const& renderJobName, void* pData);
            static void beginRenderJobCallback(std::string const& renderJobName, void* pData);
            static void finishLoadClusterData(void* pData, uint32_t iWorkerID);

            static void prevRenderJobCallback(std::string const& renderJobName, void* pData);

            void startUploadClusterData();
            void updateConstantBuffer();
            void updateGPUMeshData();
            void updateMeshInstanceInfo();
            void updateRootClusterInfo();
            void updateClusterAddressQueue();

            void getGPUBuffers();

            static void queueNewClustersToLoad(
                void* pCallbackData,
                uint32_t iWorker);

            void outputDebugClusterOBJ(uint32_t iMesh);

            void testIndirectDrawCommands();
            void comparePIXDrawClusterFile(
                std::string const& filePath0,
                std::string const& filePath1);

            protected:
                void buildOctTree(
                    OctTree& octTree,
                    std::vector<OctNode>& aOctNodes,
                    uint32_t iParentNodeID, 
                    std::vector<MeshInstanceInfo> const& aMeshInstances,
                    std::vector<MeshInfo> const& aMeshes,
                    float3 const& minBounds,
                    float3 const& maxBounds,
                    uint32_t iLevel);

                static void traverseOctTree(
                    std::vector<MeshInstanceInfo>& aMeshInstances,
                    std::vector<OctNode> const& aOctNodes,
                    uint32_t iStartingNode,
                    OctTree const& octTree,
                    uint32_t iThread);

                void traverseOctTreeMT(
                    std::vector<MeshInstanceInfo>& aMeshInstances,
                    std::vector<OctNode> const& aOctNodes,
                    OctTree const& octTree);

        };
    
    }   // Render 

}   // Common