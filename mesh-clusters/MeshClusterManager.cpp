#include "MeshClusterManager.h"
#include "Utils/Utils.h"
#include <JobManager.h>

#include <LogPrint.h>

#include <algorithm>
#include <filesystem>
#include <sstream>

#include "wtfassert.h"

#define VALIDITY_BIT_VALUE 0x800
#define MAX_NUM_CLUSTER_VERTICES    384

std::mutex sMeshClusterMutex;

std::atomic<bool>       sbLoading = false;
std::atomic<bool>       sbUploadingToGPU = false;
std::atomic<bool>       sbCopyingDataFromMainThread = false;

static float sfZ = 2.06f;
static float sfInc = 1.0f;

namespace Render
{
    namespace Common
    {
        /*
        **
        */
        static std::unique_ptr<Render::Common::CMeshClusterManager> spInstance = nullptr;
        std::unique_ptr<CMeshClusterManager>& CMeshClusterManager::instance()
        {
            return spInstance;
        }

        /*
        **
        */
        std::unique_ptr<CMeshClusterManager>& CMeshClusterManager::instance(std::unique_ptr<Render::Common::CMeshClusterManager>& pPtr)
        {
            spInstance = std::move(pPtr);
            return spInstance;
        }

        /*
        **
        */
        CMeshClusterManager::~CMeshClusterManager()
        {

        }

        /*
        **
        */
        void CMeshClusterManager::setup(MeshClusterManagerDescriptor const& desc)
        {
            //float fConeW = clusterNode.mNormalCone.w <= 0.0f ? 1.0f : sqrt(1.0f - clusterNode.mNormalCone.w * clusterNode.mNormalCone.w);

            //float fConeW = -sqrtf(1.0f - 0.220631f * 0.220631f);
            //float3 xformConeNormal = float3(-0.0251317f, -0.0664604f, 0.611623f); //float3(0.611623f, -0.0664604f, -0.0251317f);
            //float fDP = dot(xformConeNormal, float3(0.0f, 0.0f, 1.0f));

            //float3 normal = normalize(float3(1.0f, 0.0f, 1.0f));
            //float3 viewDir = float3(0.0f, 0.0f, 1.0f);
            //float fDP = dot(normal, viewDir);
            //float fNormalCone = 0.7f;
            //
            //
            //bool bDraw = (fDP - (1.0f - fNormalCone)) <= 0.0f;
            //int iDebug = 1;


            //float3 pos0 = float3(0.59319472f, 0.78070223f, 0.11037300f);
            //float3 pos1 = float3(0.62597001f, 0.71901298f, 0.26786000f);
            //float3 pos2 = float3(0.70764804f, 0.53377497f, 0.01304600f);
            //float3 diff0 = normalize(pos2 - pos0);
            //float3 diff1 = normalize(pos1 - pos0);
            //float3 faceNormal = cross(diff0, diff1);
            //
            //float3 checkPos0 = float3(0.56396598f, 0.17383300f, 0.71106303f);
            //float3 checkPos1 = float3(0.50115103f, 0.09743800f, 0.72194803f);
            //float3 checkPos2 = float3(0.60545301f, 0.13462400f, 0.65481597f);
            //float3 checkDiff0 = normalize(checkPos2 - checkPos0);
            //float3 checkDiff1 = normalize(checkPos1 - checkPos0);
            //float3 checkNormal = cross(checkDiff0, checkDiff1);
            //float fPlaneD = dot(checkNormal, checkPos0) * -1.0f;
            //float fT = Render::Common::Utils::rayPlaneIntersection(
            //    pos0,
            //    pos0 + faceNormal * 100.0f,
            //    checkNormal,
            //    fPlaneD);
            
            //float3 intersectionPt   = float3(2.22333670f, 1.55106866f, 0.07287760f);
            //float3 checkPos0        = float3(0.56396598f, 0.17383300f, 0.71106303f);
            //float3 checkPos1        = float3(0.50115103f, 0.09743800f, 0.72194803f);
            //float3 checkPos2        = float3(0.60545301f, 0.13462400f, 0.65481597f);
            //float3 barycentricPt = Render::Common::Utils::barycentric(intersectionPt, checkPos0, checkPos1, checkPos2);

            mpDevice = desc.mpDevice;
            mpRenderer = desc.mpRenderer;

            mFirstRenderJobName = "Mesh Cluster Visibility Compute";

            mRenderJobCallbackData.mpMeshClusterManager = this;
            mRenderJobCallbackData.mpRenderer = mpRenderer;
            mpRenderer->registerRenderJobFinishCallback(
                mFirstRenderJobName,
                finishRenderJobCallback,
                &mRenderJobCallbackData);

            maDrawClusterInfo.resize(1048576);

            mpRenderer->registerRenderJobBeginCallback(
                "Mesh Cluster Indirect Draw List Creation Compute",
                beginRenderJobCallback,
                &mRenderJobCallbackData);

            mpRenderer->registerRenderJobBeginCallback(
                "Draw Mesh Cluster Graphics",
                beginRenderJobCallback,
                &mRenderJobCallbackData);

            miNumMeshInstances = static_cast<uint32_t>(maMeshInstanceInfo.size());
            
            platformSetup(desc);

            miUploadStartIndex = 0;

            maClusterQueueInfo.resize(1024);

            maQueuedClusterHashMap.resize(1 << 24);
            maLoadedClusterHashMap.resize(1 << 24);

            mLastLoadTime = std::chrono::high_resolution_clock::now();
        }

        /*
        **
        */
        void CMeshClusterManager::loadMeshes(std::string const& directory)
        {
            mAssetDirectory = directory;

            uint32_t iMesh = 0;
            uint32_t iNumTotalClusterGroups = 0, iNumTotalClusters = 0, iNumMeshes = 0;
            for(auto const& dir : std::filesystem::directory_iterator(directory))
            {
                if(dir.is_directory())
                {
                    std::string fullDirectoryPath = reinterpret_cast<char const*>(dir.path().u8string().c_str());
                    Render::Common::loadClusterTreeNodes(
                        maClusterTreeNodes[iMesh],
                        fullDirectoryPath + "//cluster-tree.bin",
                        iMesh);

                    Render::Common::loadClusterGroupTreeNodes(
                        maClusterGroupTreeNodes[iMesh],
                        fullDirectoryPath + "//cluster-group-tree.bin",
                        iMesh);

                    maMeshFileDirectories.push_back(fullDirectoryPath);

                    auto const& testClusterTreeNodes = maClusterTreeNodes[iMesh];

                    uint32_t iRootNodeLevel = maClusterGroupTreeNodes[iMesh].back().miLevel;

                    // add total array offset to cluster
                    uint32_t iRootNodeStart = UINT32_MAX;
                    uint32_t iClusterIndex = iNumTotalClusters;
                    for(auto& clusterTreeNode : maClusterTreeNodes[iMesh])
                    {
                        clusterTreeNode.miClusterAddress += iNumTotalClusters;
                        clusterTreeNode.miClusterGroupAddress += iNumTotalClusterGroups;

                        for(uint32_t i = 0; i < clusterTreeNode.miNumChildren; i++)
                        {
                            clusterTreeNode.maiParentAddress[i] += iNumTotalClusters;
                            clusterTreeNode.maiChildrenAddress[i] += iNumTotalClusters;
                        }

                        // save the root node start
                        if(clusterTreeNode.miLevel == iRootNodeLevel && iRootNodeStart == UINT32_MAX)
                        {
                            iRootNodeStart = iClusterIndex;
                        }

                        ++iClusterIndex;
                    }
                    uint32_t iNumRootNodes = iClusterIndex - iRootNodeStart;

                    // add total array offset to cluster group
                    uint32_t iRootGroupNodeStart = UINT32_MAX;
                    uint32_t iClusterGroupIndex = iNumTotalClusterGroups;
                    for(auto& clusterGroupTreeNode : maClusterGroupTreeNodes[iMesh])
                    {
                        clusterGroupTreeNode.miClusterGroupAddress += iNumTotalClusterGroups;
                        
                        for(uint32_t i = 0; i < clusterGroupTreeNode.miNumChildClusters; i++)
                        {
                            clusterGroupTreeNode.maiClusterAddress[i] += iNumTotalClusters;
                        }

                        if(clusterGroupTreeNode.miLevel == iRootNodeLevel && iRootGroupNodeStart == UINT32_MAX)
                        {
                            iRootGroupNodeStart = iClusterGroupIndex;
                        }

                        ++iClusterGroupIndex;
                    }
                    uint32_t iNumRootGroupNodes = iClusterGroupIndex - iRootGroupNodeStart;

                    std::vector<uint32_t> aiNumClusterVertices;
                    std::vector<uint32_t> aiNumClusterIndices;
                    std::vector<uint64_t> aiVertexBufferArrayOffsets;
                    std::vector<uint64_t> aiIndexBufferArrayOffsets;
                    loadMeshClusterTriangleDataTableOfContent(
                        aiNumClusterVertices,
                        aiNumClusterIndices,
                        aiVertexBufferArrayOffsets,
                        aiIndexBufferArrayOffsets,
                        fullDirectoryPath + "\\mesh-cluster-triangle-vertex-data.bin",
                        fullDirectoryPath + "\\mesh-cluster-triangle-index-data.bin");

                    for(uint32_t i = 0; i < static_cast<uint32_t>(aiNumClusterIndices.size()); i++)
                    {
                        WTFASSERT(aiNumClusterIndices[i] <= 384, "too large cluster indices %d", aiNumClusterIndices[i]);
                    }

                    maaiNumClusterVertices[iMesh] = aiNumClusterVertices;
                    maaiNumClusterIndices[iMesh] = aiNumClusterIndices;
                    maaiVertexBufferArrayOffsets[iMesh] = aiVertexBufferArrayOffsets;
                    maaiIndexBufferArrayOffsets[iMesh] = aiIndexBufferArrayOffsets;

#if 0
                    {
                        std::string fullPath = fullDirectoryPath + "\\mesh-cluster-triangle-vertex-only-data.bin";
                        FILE* fp = fopen(fullPath.c_str(), "rb");
                        uint32_t iNumClusters = 0;
                        fread(&iNumClusters, sizeof(uint32_t), 1, fp);
                        maaiTriangleVertexBufferArrayOffsets[iMesh].resize(iNumClusters);
                        fread(
                            maaiTriangleVertexBufferArrayOffsets[iMesh].data(),
                            sizeof(uint32_t),
                            iNumClusters,
                            fp);
                        fclose(fp);
                    }
#endif // #if 0

                    MeshInfo meshInfo;
                    meshInfo.miID = iMesh;
                    meshInfo.miClusterGroupNodeStartIndex = iNumTotalClusterGroups;
                    meshInfo.miClusterNodeStartIndex = iNumTotalClusters;
                    meshInfo.miNumClusterGroups = static_cast<uint32_t>(maClusterGroupTreeNodes[iMesh].size());
                    meshInfo.miNumClusters = static_cast<uint32_t>(maClusterTreeNodes[iMesh].size());
                    meshInfo.miMaxMIPLevel = maClusterGroupTreeNodes[iMesh][maClusterGroupTreeNodes[iMesh].size() - 1].miLevel;
                    meshInfo.miClusterRootNodeStartIndex = iRootNodeStart;
                    meshInfo.miNumRootClusters = iNumRootNodes;
                    meshInfo.miClusterGroupRootNodeStartIndex = iRootGroupNodeStart;
                    meshInfo.miNumRootClusterGroups = iNumRootGroupNodes;

                    // get mesh bounds
                    float3 minBounds = float3(0.0f, 0.0f, 0.0f);
                    float3 maxBounds = float3(0.0f, 0.0f, 0.0f);
                    {
                        std::string filePath = fullDirectoryPath + "\\mesh-info.bin";
                        FILE* fp = fopen(filePath.c_str(), "rb");
                        WTFASSERT(fp, "Can\'t open %s\n", filePath.c_str());
                        uint32_t iNumLODLevels = 0;
                        fread(&iNumLODLevels, sizeof(uint32_t), 1, fp);
                        std::vector<float3> aMinBounds(iNumLODLevels);
                        fread(aMinBounds.data(), sizeof(float3), iNumLODLevels, fp);
                        std::vector<float3> aMaxBounds(iNumLODLevels);
                        fread(aMaxBounds.data(), sizeof(float3), iNumLODLevels, fp);
                        fclose(fp);

                        for(auto const& bound : aMinBounds)
                        {
                            minBounds = fminf(minBounds, bound);
                        }

                        for(auto const& bound : aMaxBounds)
                        {
                            maxBounds = fmaxf(maxBounds, bound);
                        }
                    }
                    meshInfo.mMinBounds = minBounds;
                    meshInfo.mMaxBounds = maxBounds;

                    maMeshInfo.push_back(meshInfo);

                    iNumTotalClusterGroups += static_cast<uint32_t>(maClusterGroupTreeNodes[iMesh].size());
                    iNumTotalClusters += static_cast<uint32_t>(maClusterTreeNodes[iMesh].size());
                    ++iMesh;
                    
                    ++iNumMeshes;
                }

            }   // for directories in parent directory
            
            struct DrawInstanceInfo
            {
                std::string         mMeshName;
                float4x4            mTransformMatrix;
                float4x4            mRotationMatrix;
            };

#if 0
            DrawInstanceInfo aDrawInstances[] =
            {
                { "guan-yu-full", translate(0.0f, 1.5f, 0.0f) * rotateMatrixY(3.14159f * -0.5f), rotateMatrixY(3.14159f * -0.5f) },
                { "guan-yu-full", translate(1.0f, 0.0f, 00.1f) * rotateMatrixY(3.14159f * -0.5f), rotateMatrixY(3.14159f * -0.5f) },
                { "guan-yu-full", translate(-1.5f, 1.5f, -0.8f) * rotateMatrixY(3.14159f * -0.5f), rotateMatrixY(3.14159f * -0.5f) },
                { "guan-yu-full", translate(-2.5f, -1.5f, 0.1f) * rotateMatrixY(3.14159f * -0.5f), rotateMatrixY(3.14159f * -0.5f) },
                { "guan-yu-full", translate(-1.0f, -1.5f, -0.5f) * rotateMatrixY(3.14159f * -0.5f), rotateMatrixY(3.14159f * -0.5f) },
                { "guan-yu-full", translate(2.0f, -1.0f, 0.0f) * rotateMatrixY(3.14159f * -0.5f), rotateMatrixY(3.14159f * -0.5f) },
                { "guan-yu-full", translate(2.0f, 1.0f, 0.0f) * rotateMatrixY(3.14159f * -0.5f), rotateMatrixY(3.14159f * -0.5f) },
               
                { "dragon", translate(-1.5f, 0.0f, 0.0f) * rotateMatrixY(3.14159f * -0.25f), rotateMatrixY(3.14159f * -0.25f) },
                { "dragon", translate(0.0f, 0.0f, 0.0f) * rotateMatrixY(3.14159f * -0.25f), rotateMatrixY(3.14159f * -0.25f) },
                { "dragon", translate(0.0f, -1.0f, 0.0f) * rotateMatrixY(3.14159f * -0.25f), rotateMatrixY(3.14159f * -0.25f) },
                { "dragon", translate(1.0f, 0.0f, -3.0f) * rotateMatrixY(3.14159f * -0.25f), rotateMatrixY(3.14159f * -0.25f) },
                { "dragon", translate(2.0f, 1.0f, -2.0f) * rotateMatrixY(3.14159f * -0.25f), rotateMatrixY(3.14159f * -0.25f) },
                { "dragon", translate(-2.0f, -1.0f, -0.4f) * rotateMatrixY(3.14159f * -0.25f), rotateMatrixY(3.14159f * -0.25f) },
            };
#endif // #if 0

            uint32_t const kiNumX = 1;
            uint32_t const kiNumY = 1;
            uint32_t const kiNumZ = 1;

            srand(0);
            std::vector<DrawInstanceInfo> aDrawInstances;
            float fStartX = -0.5f/* -2.0f*/, fStartY = 1.0f/*-2.0f*/, fStartZ = 0.5f; // -1.0f;
            for(uint32_t iZ = 0; iZ < kiNumZ; iZ++)
            {
                for(uint32_t iY = 0; iY < kiNumY; iY++)
                {
                    for(uint32_t iX = 0; iX < kiNumX; iX++)
                    {
                        uint32_t iMesh = rand() % 2;

                        DrawInstanceInfo drawInstanceInfo;
                        drawInstanceInfo.mMeshName = "dragon";

                        uint32_t iTotalIndex = iZ * 2 * 2 + iY * 2 + iX;
                        if(iMesh == 1)
                        {
                            drawInstanceInfo.mMeshName = "guan-yu-full";
                        }

                        drawInstanceInfo.mTransformMatrix =
                            translate(fStartX + float(iX) * 1.5f, fStartY + float(iY) * 1.5f, fStartZ + float(iZ) * 1.5f) *
                            rotateMatrixY(3.14159f * -0.5f);
                        drawInstanceInfo.mRotationMatrix = rotateMatrixY(3.14159f * -0.5f);
                        aDrawInstances.push_back(drawInstanceInfo);
                    }
                }
            }

            srand(static_cast<uint32_t>(time(nullptr)));

            std::map<std::string, uint32_t> aMeshInstanceCount;
            //for(uint32_t iDrawMesh = 0; iDrawMesh < sizeof(aDrawInstances) / sizeof(*aDrawInstances); iDrawMesh++)
            for(uint32_t iDrawMesh = 0; iDrawMesh < aDrawInstances.size(); iDrawMesh++)
            {
                for(uint32_t iDir = 0; iDir < static_cast<uint32_t>(maMeshFileDirectories.size()); iDir++)
                {
                    auto const& directory = maMeshFileDirectories[iDir];
                    if(directory.find(aDrawInstances[iDrawMesh].mMeshName) != std::string::npos)
                    {
                        uint32_t iInstanceIndex = 0;
                        if(aMeshInstanceCount.find(aDrawInstances[iDrawMesh].mMeshName) == aMeshInstanceCount.end())
                        {
                            aMeshInstanceCount[aDrawInstances[iDrawMesh].mMeshName] = 1;
                        }
                        else
                        {
                            iInstanceIndex = aMeshInstanceCount[aDrawInstances[iDrawMesh].mMeshName];
                            aMeshInstanceCount[aDrawInstances[iDrawMesh].mMeshName] += 1;
                        }

                        maMeshInstanceInfo.emplace_back(
                            iDir, iInstanceIndex, aDrawInstances[iDrawMesh].mTransformMatrix, aDrawInstances[iDrawMesh].mRotationMatrix);
                        
                        break;
                    }

                }
            }

            miNumMeshInstances = static_cast<uint32_t>(maMeshInstanceInfo.size());

            // total offsets for cluster, cluster groups, root clusters
            uint32_t iNumTotalClusterGroupIndices = 0;
            uint32_t iNumTotalMeshClusters = 0, iNumTotalMeshClusterGroups = 0, iNumTotalRootClusters = 0;
            uint32_t iNumTotalMeshInstances = 0;
            for(auto& instanceInfo : maMeshInstanceInfo)
            {
                instanceInfo.miClusterStartIndex = iNumTotalMeshClusters;
                instanceInfo.miClusterGroupStartIndex = iNumTotalMeshClusterGroups;
                instanceInfo.miRootClusterStartIndex = iNumTotalRootClusters;
                instanceInfo.miTotalMeshInstanceIndex = iNumTotalMeshInstances;

                uint32_t iMeshIndex = instanceInfo.miMesh;
                iNumTotalMeshClusters += maMeshInfo[iMeshIndex].miNumClusters;
                iNumTotalMeshClusterGroups += maMeshInfo[iMeshIndex].miNumClusterGroups;
                iNumTotalRootClusters += maMeshInfo[iMeshIndex].miNumRootClusters;
                iNumTotalMeshInstances += 1;

                auto const& meshInfo = maMeshInfo[iMeshIndex];
                for(uint32_t iClusterGroup = 0; iClusterGroup < meshInfo.miNumClusterGroups; iClusterGroup++)
                {
                    ++iNumTotalClusterGroupIndices;
                }
            }

            maTotalClusterGroupIndices.resize(iNumTotalClusterGroupIndices);

            for(uint32_t iMesh = 0; iMesh < static_cast<uint32_t>(maMeshInfo.size()); iMesh++)
            {
                auto const& meshInfo = maMeshInfo[iMesh];
                uint32_t iStart = meshInfo.miClusterRootNodeStartIndex;
                uint32_t iEnd = meshInfo.miClusterNodeStartIndex + meshInfo.miNumClusters;
                for(uint32_t iCluster = iStart; iCluster < iEnd; iCluster++)
                {
                    uint32_t iClusterIndex = iCluster - meshInfo.miClusterNodeStartIndex;
                    maaRootClusterNodes[iMesh].push_back(maClusterTreeNodes[iMesh][iClusterIndex]);
                }
            }

            initVertexAndIndexBuffers(
                *mpRenderer,
                *mpDevice);

            getGPUBuffers();

            // upload to GPU: mesh info
            if(mpMeshInfoBuffer)
            {
                std::vector<uint8_t> acUploadData(sizeof(uint32_t) + iNumMeshes * sizeof(MeshInfo));
                uint32_t* pDataInt = reinterpret_cast<uint32_t*>(acUploadData.data());
                *pDataInt++ = iNumMeshes;
                MeshInfo* pDataMeshInfo = reinterpret_cast<MeshInfo*>(pDataInt);

                for(uint32_t iMesh = 0; iMesh < iNumMeshes; iMesh++)
                {
                    *pDataMeshInfo++ = maMeshInfo[iMesh];
                }

                pDataMeshInfo = reinterpret_cast<MeshInfo*>(pDataInt);

                mpRenderer->copyCPUToBuffer(
                    mpMeshInfoBuffer,
                    acUploadData.data(),
                    0,
                    static_cast<uint32_t>(acUploadData.size()));

                mpRenderer->executeUploadCommandBufferAndWait();
            }

            // upload to GPU: cluster node
            if(mpClusterTreeNodeBuffer)
            {
                std::vector<uint8_t> acUploadData(sizeof(uint32_t) + iNumTotalClusters * sizeof(ClusterTreeNode));
                uint32_t* paiUploadData = reinterpret_cast<uint32_t*>(acUploadData.data());
                *paiUploadData++ = iNumTotalClusters;
                ClusterTreeNode* paClusterTreeNode = reinterpret_cast<ClusterTreeNode*>(paiUploadData);
                for(uint32_t iMesh = 0; iMesh < iNumMeshes; iMesh++)
                {
                    memcpy(paClusterTreeNode, maClusterTreeNodes[iMesh].data(), maClusterTreeNodes[iMesh].size() * sizeof(ClusterTreeNode));
                    paClusterTreeNode += maClusterTreeNodes[iMesh].size();
                }

                mpRenderer->copyCPUToBuffer(
                    mpClusterTreeNodeBuffer,
                    acUploadData.data(),
                    0,
                    static_cast<uint32_t>(acUploadData.size()));

                mpRenderer->executeUploadCommandBufferAndWait();
            }

            // upload to GPU: cluster group node
            if(mpClusterGroupTreeNodeBuffer)
            {
                std::vector<uint8_t> acUploadData(sizeof(uint32_t) + iNumTotalClusterGroups * sizeof(ClusterGroupTreeNode));
                uint32_t* paiUploadData = reinterpret_cast<uint32_t*>(acUploadData.data());
                *paiUploadData++ = iNumTotalClusterGroups;
                ClusterGroupTreeNode* paClusterGroupTreeNode = reinterpret_cast<ClusterGroupTreeNode*>(paiUploadData);
                for(uint32_t iMesh = 0; iMesh < iNumMeshes; iMesh++)
                {
                    memcpy(paClusterGroupTreeNode, maClusterGroupTreeNodes[iMesh].data(), maClusterGroupTreeNodes[iMesh].size() * sizeof(ClusterGroupTreeNode));
                    paClusterGroupTreeNode += maClusterGroupTreeNodes[iMesh].size();
                }
                mpRenderer->copyCPUToBuffer(
                    mpClusterGroupTreeNodeBuffer,
                    acUploadData.data(),
                    0,
                    static_cast<uint32_t>(acUploadData.size()));

                mpRenderer->executeUploadCommandBufferAndWait();
            }

            maaClusterQueueInfo.resize(maMeshInstanceInfo.size());

            updateConstantBuffer();
            updateMeshInstanceInfo();
            updateRootClusterInfo();

            OctTree octTree;
            std::vector<OctNode> aOctNodes;
            buildOctTree(
                octTree,
                aOctNodes,
                UINT32_MAX,
                maMeshInstanceInfo,
                maMeshInfo,
                float3(-10.0f, -10.0f, -10.0f),
                float3(10.0f, 10.0f, 10.0f),
                0);

            traverseOctTreeMT(
                maMeshInstanceInfo,
                aOctNodes,
                octTree);

            //outputDebugClusterOBJ(0);
        }

        /*
        **
        */
        void CMeshClusterManager::getGPUBuffers()
        {
            auto pSerializer = mpRenderer->getSerializer();

            mpClusterGroupTreeNodeBuffer = pSerializer->getBuffer("Cluster Group Tree Nodes");
            //WTFASSERT(mpClusterGroupTreeNodeBuffer, "can\'t find \"Cluster Group Tree Nodes\"");

            mpClusterTreeNodeBuffer = pSerializer->getBuffer("Cluster Tree Nodes");
            //WTFASSERT(mpClusterTreeNodeBuffer, "can\'t find \"Cluster Tree Nodes\"");

            mpDrawClusterInfoBuffer = pSerializer->getBuffer("Total Draw Cluster Info");
            WTFASSERT(mpDrawClusterInfoBuffer, "can\'t find \"Total Draw Cluster Info\"");

            //mpIndirectDrawClusterBuffer = pSerializer->getBuffer("Total Cluster Indirect Draw Commands");
            //WTFASSERT(mpIndirectDrawClusterBuffer, "can\'t find \"Total Cluster Indirect Draw Commands\"");

            mpClusterDataTOCBuffer = pSerializer->getBuffer("Total Cluster Data Table of Content");
            //WTFASSERT(mpClusterDataTOCBuffer, "can\'t find \"Total Cluster Data Table of Content\"");

            mpTotalClusterVertexDataBuffer = pSerializer->getBuffer("Total Cluster Vertex Data");
            //WTFASSERT(mpTotalClusterVertexDataBuffer, "can\'t find \"Total Cluster Vertex Data\"");

            //mpTotalClusterIndexDataBuffer = pSerializer->getBuffer("Total Cluster Index Data");
            //WTFASSERT(mpTotalClusterIndexDataBuffer, "can\'t find \"Total Cluster Index Data\"");

            mpPrevFrameIndirectDrawClusterBuffer = pSerializer->getBuffer("Previous Indirect Command Buffer");

            //mpTotalClusterConstantBuffer = pSerializer->getBuffer("Cluster Constant Buffer Info");
            //WTFASSERT(mpTotalClusterConstantBuffer, "can\'t find \"Total Cluster Constant Buffer Info\"");

            mpPostFrameIndiectDrawClusterBuffer = pSerializer->getBuffer("Post Draw Indirect Draw Command Buffer");

            mpVisitedBuffer = pSerializer->getBuffer("Cluster Visited Address Buffer");

            mpClearBuffer = pSerializer->getBuffer("Clear Buffer");

            mpMeshInstanceInfoBuffer = pSerializer->getBuffer("Mesh Instance Info Buffer");

            mpConstantBufferInfoAddressBuffer = pSerializer->getBuffer("Mesh Instance Constant Buffer Address Buffer");
            WTFASSERT(mpConstantBufferInfoAddressBuffer, "no buffer named \"Mesh Instance Constant Buffer Address Buffer\"");

            mpPrePassCountBuffer = pSerializer->getBuffer("Pre Pass Count Buffer");
            //WTFASSERT(mpPrePassCountBuffer, "no buffer named \"Pre Pass Count Buffer\"");

            mpPostPassCountBuffer = pSerializer->getBuffer("Post Pass Count Buffer");
            //WTFASSERT(mpPostPassCountBuffer, "no buffer named \"Post Pass Count Buffer\"");

            mpMeshInfoBuffer = pSerializer->getBuffer("Mesh Info Buffer");
            WTFASSERT(mpMeshInfoBuffer, "no buffer named \"Mesh Info Buffer\"");

            mpRootClusterInfoBuffer = pSerializer->getBuffer("Root Cluster Info Buffer");
            //WTFASSERT(mpRootClusterInfoBuffer, "no buffer named \"Root Cluster Info Buffer\"");

            mpVisibilityCountBuffer = pSerializer->getBuffer("Visibility Count Buffer");
            WTFASSERT(mpVisibilityCountBuffer, "no buffer named \"Visibility Count Buffer\"");
        
            mpClusterQueueBuffer = pSerializer->getBuffer("Cluster Queued Address Buffer");
            WTFASSERT(mpClusterQueueBuffer, "no buffer named \"Cluster Queued Address Buffer\"");

            mpClusterTraverseBuffer = pSerializer->getBuffer("Cluster Traverse Address Buffer");
            WTFASSERT(mpClusterTraverseBuffer, "no buffer named \"Cluster Traverse Address Buffer\"");

            mpInitialClusterGroupBuffer = pSerializer->getBuffer("Root Cluster Group Buffer");
            WTFASSERT(mpInitialClusterGroupBuffer, "no buffer named \"Root Cluster Group Buffer\"");

            mpClusterDataIndexHashMap = pSerializer->getBuffer("Cluster Data Index Hash Map Buffer");
            WTFASSERT(mpClusterDataIndexHashMap, "no buffer named \"Cluster Data Index Hash Map Buffer\"");

            mpTotalTraverseClusterAddressBuffer = pSerializer->getBuffer("Total Traverse Cluster Address Buffer");
            //WTFASSERT(mpTotalTraverseClusterAddressBuffer, "no buffer named \"Total Traverse Cluster Address Buffer\"");

            mpVisibilityDebugBuffer = pSerializer->getBuffer("Visibility Debug Buffer 0");
            mpPrePassDebugBuffer = pSerializer->getBuffer("Pre Pass Debug Buffer");
            mpPostPassDebugBuffer = pSerializer->getBuffer("Post Pass Debug Buffer");

            mpInitialMeshInstanceRootClusterBuffer = pSerializer->getBuffer("Initial Mesh Instance Root Cluster Buffer");
            WTFASSERT(mpInitialMeshInstanceRootClusterBuffer, "no buffer named \"Initial Mesh Instance Root Cluster Buffer\"");

            macUploadConstantBufferData.resize(1024);

            mpIndirectDrawCommandBuffer = pSerializer->getBuffer("Previous Indirect Command Buffer");
            WTFASSERT(mpIndirectDrawCommandBuffer, "wtf");

            mpMeshInstancesToDrawBuffer = pSerializer->getBuffer("Mesh Instance To Draw Buffer");
            WTFASSERT(mpMeshInstancesToDrawBuffer, "no buffer named \"Mesh Instance To Draw Buffer\"");

            struct CopyJobInfo
            {
                RenderDriver::Common::CBuffer* mpBuffer;
                uint32_t miNumLoops;
                uint32_t miDestDataOffset;
            };

            // buffers to clear
            CopyJobInfo apBuffers[] =
            {
                { mpPrevFrameIndirectDrawClusterBuffer,         1,          1024 },
                { mpDrawClusterInfoBuffer,                      10,         1 << 16 },
                { mpPrePassCountBuffer,                         1,          1024 },
                { mpPostPassCountBuffer,                        1,          1024 },
                { mpVisitedBuffer,                              10,         1 << 16 },
                { mpClusterQueueBuffer,                         10,         1 << 16 }
            };


            // add buffers to update to cache list
            {
                std::vector<UpdateDataInfo> aUpdateDataInfo;

                // update mesh instance's constant buffer on the gpu
                if(miNumMeshInstances > 0)
                {
                    maiConstantBufferAddress.resize(maMeshInstanceInfo.size());

                    // constant buffer address
                    uint32_t iMeshInstanceInfoIndex = 0;
                    for(auto const& meshInstanceInfo : maMeshInstanceInfo)
                    {
                        RenderDriver::Common::CBuffer* pBuffer = mapMeshInstanceConstantBuffers[iMeshInstanceInfoIndex].get();
                        uint64_t iConstantBufferGPUAddress = pBuffer->getGPUVirtualAddress();
                        maiConstantBufferAddress[iMeshInstanceInfoIndex] = iConstantBufferGPUAddress;
                        ++iMeshInstanceInfoIndex;
                    }

                    // constant buffer address buffer for all the mesh instances
                    UpdateDataInfo dataInfo;
                    dataInfo.mpBuffer = mpConstantBufferInfoAddressBuffer;
                    dataInfo.miSrcDataSize = static_cast<uint32_t>(maiConstantBufferAddress.size() * sizeof(uint64_t));
                    dataInfo.miSrcDataOffset = 0;
                    dataInfo.miDestDataOffset = 0;
                    dataInfo.mpSrcData = maiConstantBufferAddress.data();
                    aUpdateDataInfo.push_back(dataInfo);

                    maPreCacheDataInfo.push_back(dataInfo);
                }

                // mesh instance's constant buffer data
                maacMeshInstanceConstantBufferData.resize(maMeshInstanceInfo.size());
                
                // mesh instance constant buffers
                for(uint32_t iMeshInstanceInfo = 0; iMeshInstanceInfo < maMeshInstanceInfo.size(); iMeshInstanceInfo++)
                {

                    maacMeshInstanceConstantBufferData[iMeshInstanceInfo] = macUploadConstantBufferData;

                    UpdateDataInfo dataInfo;
                    dataInfo.mpBuffer = mapMeshInstanceConstantBuffers[iMeshInstanceInfo].get();
                    dataInfo.miSrcDataSize = static_cast<uint32_t>(maacMeshInstanceConstantBufferData[iMeshInstanceInfo].size());
                    dataInfo.miSrcDataOffset = 0;
                    dataInfo.miDestDataOffset = 0;
                    dataInfo.mpSrcData = maacMeshInstanceConstantBufferData[iMeshInstanceInfo].data();
                    aUpdateDataInfo.push_back(dataInfo);

                    maPreCacheDataInfo.push_back(dataInfo);
                }

                std::vector<std::string> aRenderJobNames =
                {
                    "Mesh Instance Visibility Compute",
                    "Mesh Cluster Visibility Compute",
                    "Pre Pass Mesh Cluster Visibility Compute",
                    "Post Pass Mesh Cluster Visibility Compute",
                    "Draw Pre Pass Mesh Cluster Graphics",
                    "Draw Post Pass Mesh Cluster Graphics",
                    "Over Draw Graphics"
                };

                // render jobs' constant buffers
                for(auto const& renderJobName : aRenderJobNames)
                {
                    std::string constantBufferName = renderJobName + ".render-pass-constant-buffer";
                    RenderDriver::Common::CBuffer* pBuffer = mpRenderer->getSerializer()->getBuffer(constantBufferName);
                    if(pBuffer)
                    {
                        UpdateDataInfo dataInfo;
                        dataInfo.mpBuffer = pBuffer;
                        dataInfo.miSrcDataSize = static_cast<uint32_t>(macUploadConstantBufferData.size());
                        dataInfo.miSrcDataOffset = 0;
                        dataInfo.miDestDataOffset = 0;
                        dataInfo.mpSrcData = macUploadConstantBufferData.data();
                        aUpdateDataInfo.push_back(dataInfo);

                        maPreCacheDataInfo.push_back(dataInfo);
                    }
                }

                // create upload buffers
                {
                    RenderDriver::Common::CBuffer* apBuffers[] =
                    {
                        mpClusterTraverseBuffer,
                        mpInitialClusterGroupBuffer,
                        mpVisibilityCountBuffer,
                        mpInitialMeshInstanceRootClusterBuffer,
                    };

                    for(uint32_t i = 0; i < sizeof(apBuffers) / sizeof(*apBuffers); i++)
                    {
                        RenderDriver::Common::BufferDescriptor bufferDesc;
                        bufferDesc.mFormat = RenderDriver::Common::Format::R32_FLOAT;
                        bufferDesc.mHeapType = RenderDriver::Common::HeapType::Upload;
                        bufferDesc.miSize = apBuffers[i]->getDescriptor().miSize;
                        maUploadBuffers[i]->create(bufferDesc, *mpDevice);

                        maUploadBuffers[i]->setID(apBuffers[i]->getID() + " Upload Buffer");
                    }
                }

            }
        }


        /*
        **
        */
        //void CMeshClusterManager::platformSetup(MeshClusterManagerDescriptor const& desc)
        //{
        //    
        //}

        /*
        **
        */
        void CMeshClusterManager::update()
        {
            mpRenderer->beginDebugMarker("Cluster Manager Update");
            
            // determine how many dispatches needed for all the mesh instances
            {
                uint32_t const kiNumMeshInstancesPerPass = 16;
                uint32_t iNumClusterTreeTraversePasses = static_cast<uint32_t>(ceil(static_cast<float>(maaClusterQueueInfo.size()) / static_cast<float>(kiNumMeshInstancesPerPass)));

                if(iNumClusterTreeTraversePasses <= 0)
                {
                    mpRenderer->endDebugMarker();
                    return;
                }

                std::vector<uint3> aDispatches(iNumClusterTreeTraversePasses);
                for(uint32_t i = 0; i < iNumClusterTreeTraversePasses - 1; i++)
                {
                    aDispatches[i] = uint3(kiNumMeshInstancesPerPass, 1, 1);
                }
                aDispatches[iNumClusterTreeTraversePasses - 1] = uint3(static_cast<uint32_t>(maaClusterQueueInfo.size() % kiNumMeshInstancesPerPass), 1, 1);

                mpRenderer->addDuplicateRenderJobs(
                    "Mesh Cluster Visibility", 
                    iNumClusterTreeTraversePasses,
                    aDispatches);
            }


            // copy over loaded data and upload to gpu
            if(sbCopyingDataFromMainThread.load() == true)
            {  
                mpRenderer->beginDebugMarker("GPU Data Transfer Pre-process");
                
                auto start = std::chrono::high_resolution_clock::now();

                maaUploadClusterTriangleVertexData = maaQueuedClusterTriangleVertexData;
                maaUploadClusterTriangleIndexData = maaQueuedClusterTriangleIndexData;

                for(auto& aQueuedClusterTriangleVertexData : maaQueuedClusterTriangleVertexData)
                {
                    aQueuedClusterTriangleVertexData.clear();
                }
                maaQueuedClusterTriangleVertexData.clear();

                for(auto& aQueuedClusterTriangleIndexData : maaQueuedClusterTriangleIndexData)
                {
                    aQueuedClusterTriangleIndexData.clear();
                }
                maaQueuedClusterTriangleIndexData.clear();

                uint64_t iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

                maUploadDataOffsets = maQueuedDataOffsets;
                maUploadClusterInfoGPU = maUpdatingClusterInfo;
                miUploadStartIndex = 0;
                maQueuedDataOffsets.clear();

                WTFASSERT(
                    maaQueuedClusterTriangleVertexData.size() == maaQueuedClusterTriangleIndexData.size(),
                    "mis-matched vertex (%d) and index (%d) data size",
                    maaQueuedClusterTriangleVertexData.size(),
                    maaQueuedClusterTriangleIndexData.size());

                sbCopyingDataFromMainThread.store(false);

                mpRenderer->endDebugMarker();
            }

            updateConstantBuffer();
            updateGPUMeshData();
            updateClusterAddressQueue();

            {
                if(mpVisibilityDebugBuffer)
                {
                    mpRenderer->copyBufferToBuffer(
                        mpVisibilityDebugBuffer,
                        mpClearBuffer,
                        0,
                        0,
                        1 << 16);
                }

                if(mpPrePassDebugBuffer)
                {
                    mpRenderer->copyBufferToBuffer(
                        mpPrePassDebugBuffer,
                        mpClearBuffer,
                        0,
                        0,
                        1 << 16);
                }

                if(mpPostPassDebugBuffer)
                {
                    mpRenderer->copyBufferToBuffer(
                        mpPostPassDebugBuffer,
                        mpClearBuffer,
                        0,
                        0,
                        1 << 16);
                }

                if(mpMeshInstancesToDrawBuffer)
                {
                    mpRenderer->copyBufferToBuffer(
                        mpMeshInstancesToDrawBuffer,
                        mpClearBuffer,
                        0,
                        0,
                        1 << 16);
                }
            }

            Render::Common::Serializer* pSerializer = mpRenderer->getSerializer();
            RenderJobInfo& renderJob = pSerializer->getRenderJob(mFirstRenderJobName);
            renderJob.miNumDispatchX = static_cast<uint32_t>(maaClusterQueueInfo.size());
            renderJob.miNumDispatchY = 1;
            renderJob.miNumDispatchZ = 1;

#if 0
            // esimated dispatches needed for pre and post culling
            {
                uint32_t iNumClusters = 0;
                for(auto const& meshInstance : maMeshInstanceInfo)
                {
                    auto const& mesh = maMeshInfo[meshInstance.miMesh];
                    iNumClusters += mesh.miNumClusters / 2;
                }

                uint32_t const kiNumLocalThreadGroups = 256;
                uint32_t const kiMaxWarpPerDispatch = 8;
                uint32_t iNumCullingDispatches = static_cast<uint32_t>(ceil(float(iNumClusters) / float(kiNumLocalThreadGroups * kiMaxWarpPerDispatch)));
                auto& preCullingRenderJob = pSerializer->getRenderJob("Pre Pass Mesh Cluster Visibility Compute");
                preCullingRenderJob.miNumDispatchX = min(iNumCullingDispatches, 128);
                preCullingRenderJob.miNumDispatchY = 1;
                preCullingRenderJob.miNumDispatchZ = 1;


                auto& postCullingRenderJob = pSerializer->getRenderJob("Post Pass Mesh Cluster Visibility Compute");
                postCullingRenderJob.miNumDispatchX = min(iNumCullingDispatches, 128);
                postCullingRenderJob.miNumDispatchY = 1;
                postCullingRenderJob.miNumDispatchZ = 1;
            }
#endif // #if 0

            mpRenderer->endDebugMarker();
        }

        /*
        **
        */
        void CMeshClusterManager::loadClusterData(
            void* pData,
            uint32_t iWorkerID)
        {
            RenderJobCallBackData* pCallbackData = reinterpret_cast<RenderJobCallBackData*>(pData);
            Render::Common::CRenderer* pRenderer = pCallbackData->mpRenderer;
            Render::Common::CMeshClusterManager* pClusterManager = pCallbackData->mpMeshClusterManager;
            uint32_t iFrameIndex = pRenderer->getFrameIndex();

            uint32_t iTotalClusterVertexDataBufferSize = static_cast<uint32_t>(pClusterManager->mpTotalClusterVertexDataBuffer->getDescriptor().miSize);

            uint32_t iNumClusterAdded = 0;
            uint32_t iStartVertexDataOffset = UINT32_MAX, iTotalUploadDataSize = 0;
            uint32_t iNumSwapped = 0;
            //for(uint32_t iDrawClusterInfo = 0; iDrawClusterInfo < pClusterManager->miNumDrawClusters; iDrawClusterInfo++)
            for(uint32_t iDrawClusterInfo = 0; iDrawClusterInfo < static_cast<uint32_t>(pClusterManager->maNewClustersToLoad.size()); iDrawClusterInfo++)
            {
                // see if the draw cluster has been loaded already
                //DrawClusterInfo const& drawClusterInfo = pClusterManager->maDrawClusterInfo[iDrawClusterInfo];
                DrawClusterInfo const& drawClusterInfo = pClusterManager->maNewClustersToLoad[iDrawClusterInfo];

                bool bClusterLoaded = true;
                uint32_t iLocalClusterIndex = drawClusterInfo.miCluster - pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex;
                uint32_t iUpdateInfoIndex = Render::Common::Utils::getFromHashMap(
                    uint2(drawClusterInfo.miMesh, iLocalClusterIndex),
                    pClusterManager->maLoadedClusterHashMap,
                    static_cast<uint32_t>(pClusterManager->maLoadedClusterHashMap.size()));
                if(iUpdateInfoIndex == UINT32_MAX)
                {
                    Render::Common::Utils::addToHashMap(
                        uint2(drawClusterInfo.miMesh, iLocalClusterIndex),
                        static_cast<uint32_t>(pClusterManager->maUpdatingClusterInfo.size()),
                        pClusterManager->maLoadedClusterHashMap,
                        static_cast<uint32_t>(pClusterManager->maLoadedClusterHashMap.size()));
                    bClusterLoaded = false;
                }

                //if(iter == pClusterManager->maUpdatingClusterInfo.end())
                if(!bClusterLoaded)
                {
                    // new data to be loaded

                    uint32_t const kiMaxVertexBufferSize = MAX_NUM_CLUSTER_VERTICES;
                    uint32_t const kiMaxIndexBufferSize = 128 * 3;

                    std::string meshFileDirectory = pClusterManager->maMeshFileDirectories[drawClusterInfo.miMesh];

                    // load data chunk using the mesh's info from initial table of content
                    std::vector<ConvertedMeshVertexFormat> aClusterTriangleVertices(kiMaxVertexBufferSize);
                    std::vector<uint32_t> aiClusterTriangleIndices(kiMaxIndexBufferSize);
                    memset(aClusterTriangleVertices.data(), 0, kiMaxVertexBufferSize * sizeof(MeshClusterVertexFormat));
                    memset(aiClusterTriangleIndices.data(), 0, kiMaxIndexBufferSize * sizeof(uint32_t));

                    std::string vertexDataFilePath = meshFileDirectory + "\\mesh-cluster-triangle-vertex-data.bin";
                    std::string indexDataFilePath = meshFileDirectory + "\\mesh-cluster-triangle-index-data.bin";

                    bool bValid = (
                        drawClusterInfo.miCluster < pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex + pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miNumClusters &&
                        drawClusterInfo.miCluster >= pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex
                    );

                    if(!bValid)
                    {
                        DEBUG_PRINTF("!!! while loading, incorrect cluster index (%d) for mesh %d (%d) !!!\n",
                            drawClusterInfo.miCluster,
                            drawClusterInfo.miMesh,
                            pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex + pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miNumClusters);
                        continue;
                    }

                    WTFASSERT(drawClusterInfo.miCluster < pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex + pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miNumClusters,
                        "incorrect cluster index (%d) for mesh %d (%d)",
                        drawClusterInfo.miCluster,
                        drawClusterInfo.miMesh,
                        pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex + pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miNumClusters);

                    uint32_t iClusterVertexBufferSize = 0, iClusterIndexBufferSize = 0;
                    uint32_t iLocalClusterIndex = drawClusterInfo.miCluster - pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex;
                    WTFASSERT(
                        drawClusterInfo.miCluster >= pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex,
                        "Incorrect cluster index (%d) for mesh %d (%d)",
                        drawClusterInfo.miCluster,
                        drawClusterInfo.miMesh,
                        pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex);

                    WTFASSERT(
                        pClusterManager->maaiNumClusterVertices[drawClusterInfo.miMesh][iLocalClusterIndex] <= kiMaxVertexBufferSize, 
                        "number of max vertex buffer size out of bounds %d (%d)\n",
                        pClusterManager->maaiNumClusterVertices[drawClusterInfo.miMesh][iLocalClusterIndex],
                        kiMaxVertexBufferSize);

                    loadMeshClusterTriangleDataChunk(
                        aClusterTriangleVertices,
                        aiClusterTriangleIndices,
                        iClusterVertexBufferSize,
                        iClusterIndexBufferSize,
                        vertexDataFilePath,
                        indexDataFilePath,
                        pClusterManager->maaiNumClusterVertices[drawClusterInfo.miMesh],
                        pClusterManager->maaiNumClusterIndices[drawClusterInfo.miMesh],
                        pClusterManager->maaiVertexBufferArrayOffsets[drawClusterInfo.miMesh],
                        pClusterManager->maaiIndexBufferArrayOffsets[drawClusterInfo.miMesh],
                        iLocalClusterIndex);

                    // flatten to only vertex buffer
                    bool bSwapped = false;
                    uint32_t iVertexBufferDataOffset = 0, iIndexBufferDataOffset = 0;
                    {
                        std::vector<ConvertedMeshVertexFormat> aTriangleVertices(kiMaxIndexBufferSize);
                        std::vector<uint32_t> aiTriangleIndices(kiMaxIndexBufferSize);
                        uint32_t iTri = 0;
                        {
                            for(iTri = 0; iTri < aiClusterTriangleIndices.size(); iTri += 3)
                            {
                                uint32_t iV0 = aiClusterTriangleIndices[iTri];
                                uint32_t iV1 = aiClusterTriangleIndices[iTri + 1];
                                uint32_t iV2 = aiClusterTriangleIndices[iTri + 2];

                                if(iV0 == 0 && iV1 == 0 && iV2 == 0)
                                {
                                    break;
                                }

                                aTriangleVertices[iTri] = aClusterTriangleVertices[iV0];
                                aTriangleVertices[iTri + 1] = aClusterTriangleVertices[iV1];
                                aTriangleVertices[iTri + 2] = aClusterTriangleVertices[iV2];

                                aTriangleVertices[iTri].mNormal = normalize(aClusterTriangleVertices[iV0].mNormal);
                                aTriangleVertices[iTri + 1].mNormal = normalize(aClusterTriangleVertices[iV1].mNormal);
                                aTriangleVertices[iTri + 2].mNormal = normalize(aClusterTriangleVertices[iV2].mNormal);

                                WTFASSERT(iTri + 2 < aiTriangleIndices.size(), "out of bounds %d", iTri + 2);

                                aiTriangleIndices[iTri] = iTri;
                                aiTriangleIndices[iTri + 1] = iTri + 1;
                                aiTriangleIndices[iTri + 2] = iTri + 2;
                            }

                            iClusterVertexBufferSize = iTri * sizeof(ConvertedMeshVertexFormat);
                            iClusterIndexBufferSize = iTri * sizeof(uint32_t);
                        }

                        // add data to upload queue to be processed on the main thread
                        uint32_t iBufferSizeLimit = static_cast<uint32_t>(pClusterManager->mpTotalClusterVertexDataBuffer->getDescriptor().miSize);
                        bSwapped = pClusterManager->addUploadClusterData(
                            iVertexBufferDataOffset,
                            iIndexBufferDataOffset,
                            aTriangleVertices,
                            aiTriangleIndices,
                            drawClusterInfo.miMesh,
                            drawClusterInfo.miCluster,
                            iBufferSizeLimit);
                        pClusterManager->maQueuedDataOffsets.push_back(std::make_pair(iVertexBufferDataOffset, iIndexBufferDataOffset));
                    }

                    if(bSwapped)
                    {
                        ++iNumSwapped;
                    }

                    // check index validity
                    {
                        uint32_t iNumTriangleVertices = 0;
                        for(auto const& vert : aClusterTriangleVertices)
                        {
                            if(lengthSquared(vert.mPosition) <= 0.000001f)
                            {
                                break;
                            }
                            ++iNumTriangleVertices;
                        }
                        for(auto const& iIndex : aiClusterTriangleIndices)
                        {
                            WTFASSERT(iIndex < iNumTriangleVertices, "invalid index %d with num vertices %d", iIndex, iNumTriangleVertices);
                        }
                    }

                    iTotalUploadDataSize += MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat);
                    if(iStartVertexDataOffset > iVertexBufferDataOffset)
                    {
                        iStartVertexDataOffset = iVertexBufferDataOffset;
                    }

                    WTFASSERT(
                        iStartVertexDataOffset + iTotalUploadDataSize < iTotalClusterVertexDataBufferSize,
                        "upload data out of bounds for total cluster vertex data buffer, start %d size %d cluster vertex buffer size %d (%d)",
                        iStartVertexDataOffset,
                        iTotalUploadDataSize,
                        iTotalClusterVertexDataBufferSize,
                        iStartVertexDataOffset + iTotalUploadDataSize);

                    // add the upload info 
                    UploadClusterInfo uploadClusterInfo;
                    uploadClusterInfo.miFrameIndex = iFrameIndex;
                    uploadClusterInfo.miMesh = drawClusterInfo.miMesh;
                    uploadClusterInfo.miCluster = drawClusterInfo.miCluster;
                    uploadClusterInfo.miVertexBufferSize = iClusterVertexBufferSize;
                    uploadClusterInfo.miVertexBufferOffset = iVertexBufferDataOffset;
                    uploadClusterInfo.miAllocatedVertexBufferSize = static_cast<uint32_t>(kiMaxVertexBufferSize * sizeof(ConvertedMeshVertexFormat));
                    uploadClusterInfo.miLoaded = 1;
                    pClusterManager->maUpdatingClusterInfo.push_back(uploadClusterInfo);

                    if(bSwapped)
                    {
                        DEBUG_PRINTF("!!! add swapped data info to index %lld\n", pClusterManager->maUpdatingClusterInfo.size() - 1);
                    }

                    ++iNumClusterAdded;
                }
                else
                {
                    // update accessed frame for potential swapping of data
                    //uint32_t iArrayIndex = static_cast<uint32_t>(std::distance(pClusterManager->maUpdatingClusterInfo.begin(), iter));
                    //pClusterManager->maUpdatingClusterInfo[iArrayIndex].miFrameIndex = iFrameIndex;

                    pClusterManager->maUpdatingClusterInfo[iUpdateInfoIndex].miFrameIndex = iFrameIndex;
                }

            }   // for cluster in clusters to draw 

            pClusterManager->miNumSwapped = max(pClusterManager->miNumSwapped, iNumSwapped);
            
            if(iNumClusterAdded > 0)
            {
                DEBUG_PRINTF("*** start loading %d clusters ***\n", iNumClusterAdded);
            }

            //if(iNumClusterAdded > 0)
            //{
            //    std::sort(
            //        pClusterManager->maUpdatingClusterInfo.begin(),
            //        pClusterManager->maUpdatingClusterInfo.end(),
            //        [](UploadClusterInfo const& left, UploadClusterInfo const& right)
            //        {
            //            return (left.miCluster < right.miCluster) && (left.miMesh <= right.miMesh);
            //        }
            //    );
            //}

            sbLoading.store(false);
        }

        /*
        **
        */
        void CMeshClusterManager::queueNewClustersToLoad(
            void* pData,
            uint32_t iWorker)
        {
 auto start = std::chrono::high_resolution_clock::now();
            RenderJobCallBackData* pCallbackData = reinterpret_cast<RenderJobCallBackData*>(pData);
            Render::Common::CMeshClusterManager* pClusterManager = pCallbackData->mpMeshClusterManager;
            Render::Common::CRenderer* pRenderer = pClusterManager->mpRenderer;

            // get the number of clusters to load, cull out invalid cluster data
            uint32_t const* pDataInt = reinterpret_cast<uint32_t const*>(pClusterManager->maTempDrawClusterInfo2.data());
            uint32_t iMaxDrawIndex = *pDataInt++;
            uint32_t iNumClusters = 0;
            DrawClusterInfo const* paDrawClusterInfo = reinterpret_cast<DrawClusterInfo const*>(pDataInt);
            for(uint32_t i = 0; i < iMaxDrawIndex; i++)
            {
                DrawClusterInfo const& drawClusterInfo = paDrawClusterInfo[i];

                // check for valid draw clusters
                bool bValid = (drawClusterInfo.miCluster >= pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex &&
                    drawClusterInfo.miVisibilityBits > 0);
                if(bValid && (drawClusterInfo.miVisibilityBits & VALIDITY_BIT_VALUE) > 0)
                {
                    // check if already queued, add to new cluster to load if not
                    uint32_t iLocalClusterIndex = drawClusterInfo.miCluster - pClusterManager->maMeshInfo[drawClusterInfo.miMesh].miClusterNodeStartIndex;
                    uint2 clusterKey(drawClusterInfo.miMesh, iLocalClusterIndex);
                    uint32_t iValue = Render::Common::Utils::getFromHashMap(
                        clusterKey,
                        pClusterManager->maQueuedClusterHashMap,
                        static_cast<uint32_t>(pClusterManager->maQueuedClusterHashMap.size()));
                    if(iValue == UINT32_MAX)
                    {
                        pClusterManager->maQueuedNewClusterToLoad.push_back(drawClusterInfo);
                    }

                    // add and update frame index for cluster
                    Render::Common::Utils::addToHashMap(
                        clusterKey,
                        pClusterManager->mpRenderer->getFrameIndex(),
                        pClusterManager->maQueuedClusterHashMap,
                        static_cast<uint32_t>(pClusterManager->maQueuedClusterHashMap.size()));
                }
            }

            // add to the list of new clusters
            if(pClusterManager->maQueuedNewClusterToLoad.size() > 0)
            {
                std::lock_guard<std::mutex> lock(sMeshClusterMutex);
                pClusterManager->maaNewClustersToLoadQueue.push_back(pClusterManager->maQueuedNewClusterToLoad);
            }
            pClusterManager->maQueuedNewClusterToLoad.clear();

            pClusterManager->miNumDrawClusters = static_cast<uint32_t>(pClusterManager->maQueuedNewClusterToLoad.size());
            pClusterManager->miMaxDrawIndex = iMaxDrawIndex;

            pClusterManager->miNumQueueThreads -= 1;

uint64_t iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();
int iDebug = 1;
        }

        

        /*
        **
        */
        void CMeshClusterManager::finishRenderJobCallback(
            std::string const& renderJobName, 
            void* pData)
        {
            RenderJobCallBackData* pCallbackData = reinterpret_cast<RenderJobCallBackData*>(pData);
            Render::Common::CMeshClusterManager* pClusterManager = pCallbackData->mpMeshClusterManager;
            
            auto start = std::chrono::high_resolution_clock::now();
            if(pClusterManager->maTempDrawClusterInfo.size() < pClusterManager->maDrawClusterInfo.size() * sizeof(DrawClusterInfo))
            {
                pClusterManager->maTempDrawClusterInfo.resize(pClusterManager->maDrawClusterInfo.size() * sizeof(DrawClusterInfo));
                pClusterManager->maTempDrawClusterInfo2.resize(pClusterManager->maDrawClusterInfo.size() * sizeof(DrawClusterInfo));
            }
            
            if(renderJobName == pClusterManager->mFirstRenderJobName)
            {
                Render::Common::CRenderer* pRenderer = pCallbackData->mpRenderer;
                
                uint64_t const kiTimeSliceLoadMicroSeconds = 50000;
                uint32_t const kiNumQueueThreads = 4;

                uint64_t iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - pClusterManager->mLastLoadTime).count();

                // need to draw cluster info
                if(pClusterManager->maQueuedNewClusterToLoad.size() <= 0 && 
                   pClusterManager->miNumQueueThreads < kiNumQueueThreads &&
                    iElapsed >= kiTimeSliceLoadMicroSeconds)
                {
                    ++pClusterManager->miNumQueueThreads;
                    pClusterManager->mLastLoadTime = std::chrono::high_resolution_clock::now();
                    
                    pRenderer->copyBufferToCPUMemory(
                        pClusterManager->mpDrawClusterInfoBuffer,
                        pClusterManager->maTempDrawClusterInfo2.data(),
                        0,
                        pClusterManager->mpDrawClusterInfoBuffer->getDescriptor().miSize);

                    JobManager::instance()->addToWorkerQueue(
                        CMeshClusterManager::queueNewClustersToLoad,
                        &pClusterManager->mRenderJobCallbackData,
                        sizeof(mRenderJobCallbackData),
                        true,
                        nullptr);
                }

                if(sbLoading.load() == false && pClusterManager->mpDrawClusterInfoBuffer)
                {
                    // load cluster files or update the time stamp on the cluster
                    if(!sbUploadingToGPU.load() && 
                       !sbCopyingDataFromMainThread.load() && 
                       pClusterManager->maaNewClustersToLoadQueue.size() > 0)
                    {
                        {
                            std::lock_guard<std::mutex> lock(sMeshClusterMutex);
                            for(uint32_t i = 0; i < static_cast<uint32_t>(pClusterManager->maaNewClustersToLoadQueue.size()); i++)
                            {
                                pClusterManager->maNewClustersToLoad.insert(
                                    pClusterManager->maNewClustersToLoad.end(),
                                    pClusterManager->maaNewClustersToLoadQueue[i].begin(),
                                    pClusterManager->maaNewClustersToLoadQueue[i].end());
                            }

                            pClusterManager->maaNewClustersToLoadQueue.clear();
                            pClusterManager->maaNewClustersToLoadQueue.clear();
                        }

                        sbLoading.store(true);
                        JobManager::instance()->addToWorkerQueue(
                            CMeshClusterManager::loadClusterData,
                            &pClusterManager->mRenderJobCallbackData,
                            sizeof(mRenderJobCallbackData),
                            true,
                            CMeshClusterManager::finishLoadClusterData);
                    }

                }   // if !loading cluster data
                

            }   // if job == "mesh visibility"

        }

        /*
        **
        */
        void CMeshClusterManager::beginRenderJobCallback(std::string const& renderJobName, void* pData)
        {
        }

        /*
        **
        */
        bool CMeshClusterManager::addUploadClusterData(
            uint32_t& iVertexBufferDataOffset,
            uint32_t& iIndexBufferDataOffset,
            std::vector<ConvertedMeshVertexFormat> const& aClusterTriangleVertices,
            std::vector<uint32_t> const& aiClusterTriangleIndices,
            uint32_t iMesh,
            uint32_t iCluster,
            uint32_t iClusterVertexDataMemoryLimit)
        {
            uint32_t iStartCluster = maMeshInfo[iMesh].miClusterNodeStartIndex;

            uint32_t const kiUnusedFrameThreshold = 4;
            uint32_t iCurrFrameIndex = mpRenderer->getFrameIndex();

            // get the last cluster slot or the oldest
            iVertexBufferDataOffset = UINT32_MAX;
            iIndexBufferDataOffset = UINT32_MAX;
            UploadClusterInfo const* pLastUploadClusterInfo = nullptr;
            UploadClusterInfo const* pOldestUsedClusterInfo = nullptr;
            uint32_t iOldestClusterInfoIndex = UINT32_MAX;
            for(uint32_t iClusterInfoIndex = 0; iClusterInfoIndex < static_cast<uint32_t>(maUpdatingClusterInfo.size()); iClusterInfoIndex++)
            {
                auto const& uploadClusterInfo = maUpdatingClusterInfo[iClusterInfoIndex];
                if(iVertexBufferDataOffset == UINT32_MAX || uploadClusterInfo.miVertexBufferOffset > iVertexBufferDataOffset)
                {
                    pLastUploadClusterInfo = &uploadClusterInfo;
                    iVertexBufferDataOffset = uploadClusterInfo.miVertexBufferOffset;
                }

                //if(pOldestUsedClusterInfo == nullptr || uploadClusterInfo.miFrameIndex <= pOldestUsedClusterInfo->miFrameIndex - kiUnusedFrameThreshold)
                if(pOldestUsedClusterInfo == nullptr || uploadClusterInfo.miFrameIndex < pOldestUsedClusterInfo->miFrameIndex - 1)
                {
                    pOldestUsedClusterInfo = &uploadClusterInfo;
                    iOldestClusterInfoIndex = iClusterInfoIndex;
                }
            }

            if(iVertexBufferDataOffset != UINT32_MAX)
            {
                WTFASSERT(iVertexBufferDataOffset + MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat) < iClusterVertexDataMemoryLimit,
                    "vertex data out of bounds start: %d with size %d total: %d\n",
                    iVertexBufferDataOffset,
                    MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat),
                    iVertexBufferDataOffset + MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat));
            }

            // swap out oldest cluster if memory over budget
            bool bSwapped = false;
            if(pLastUploadClusterInfo && 
                pLastUploadClusterInfo->miVertexBufferOffset + pLastUploadClusterInfo->miAllocatedVertexBufferSize >= iClusterVertexDataMemoryLimit - 2 * MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat))
            {
                uint32_t iSwapStartCluster = maMeshInfo[pOldestUsedClusterInfo->miMesh].miClusterNodeStartIndex;

                DEBUG_PRINTF("!!! swap out mesh %d cluster %d (%d) for mesh %d cluster %d (%d) at index %d, curr frame: %d last used frame: %d vertex offset: %d !!!\n",
                    pOldestUsedClusterInfo->miMesh,
                    pOldestUsedClusterInfo->miCluster,
                    pOldestUsedClusterInfo->miCluster - iSwapStartCluster,
                    iMesh,
                    iCluster,
                    iCluster - iStartCluster,
                    iOldestClusterInfoIndex,
                    iCurrFrameIndex,
                    pOldestUsedClusterInfo->miFrameIndex,
                    pOldestUsedClusterInfo->miVertexBufferOffset);

                WTFASSERT(pOldestUsedClusterInfo, "don\'t have oldest cluster info");
                iVertexBufferDataOffset = pOldestUsedClusterInfo->miVertexBufferOffset;
                pLastUploadClusterInfo = pOldestUsedClusterInfo;

                // erase from hash map indicating it has been swapped out 
                uint32_t iLocalClusterIndex = pOldestUsedClusterInfo->miCluster - maMeshInfo[pOldestUsedClusterInfo->miMesh].miClusterNodeStartIndex;
                Render::Common::Utils::eraseFromHashMap(
                    uint2(pOldestUsedClusterInfo->miMesh, iLocalClusterIndex),
                    maLoadedClusterHashMap,
                    static_cast<uint32_t>(maLoadedClusterHashMap.size()));
                
                Render::Common::Utils::eraseFromHashMap(
                    uint2(pOldestUsedClusterInfo->miMesh, iLocalClusterIndex),
                    maQueuedClusterHashMap,
                    static_cast<uint32_t>(maQueuedClusterHashMap.size()));

                maUpdatingClusterInfo.erase(maUpdatingClusterInfo.begin() + iOldestClusterInfoIndex);
                bSwapped = true;
            }

            if(iVertexBufferDataOffset == UINT32_MAX)
            {
                iVertexBufferDataOffset = 0;
                iIndexBufferDataOffset = 0;
            }
            else if(!bSwapped)
            {
                WTFASSERT(pLastUploadClusterInfo, "no upload cluster info");
                iVertexBufferDataOffset += pLastUploadClusterInfo->miAllocatedVertexBufferSize;
            }
            
            WTFASSERT(iVertexBufferDataOffset + 2 * MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat) < iClusterVertexDataMemoryLimit,
                "vertex data out of bounds start: %d with size %d total: %d\n",
                iVertexBufferDataOffset,
                MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat),
                iVertexBufferDataOffset + MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat));

            // add to upload queue
            maaQueuedClusterTriangleVertexData.push_back(aClusterTriangleVertices);
            maaQueuedClusterTriangleIndexData.push_back(aiClusterTriangleIndices);
            
            assert(maaQueuedClusterTriangleVertexData.size() == maaQueuedClusterTriangleIndexData.size());

            return bSwapped;
        }

        /*
        **
        */
        void CMeshClusterManager::finishLoadClusterData(void* pData, uint32_t iWorkerID)
        {
            RenderJobCallBackData* pCallbackData = reinterpret_cast<RenderJobCallBackData*>(pData);
            Render::Common::CMeshClusterManager* pClusterManager = pCallbackData->mpMeshClusterManager;
            Render::Common::CRenderer* pRenderer = pCallbackData->mpRenderer;

            {
                std::lock_guard<std::mutex> lock(sMeshClusterMutex);
                pClusterManager->maNewClustersToLoad.clear();
            }

            if(pClusterManager->maaQueuedClusterTriangleVertexData.size() > 0 && pClusterManager->maaUploadClusterTriangleVertexData.size() <= 0)
            {
                pClusterManager->startUploadClusterData();
            }
        }

        /*
        **
        */
        void CMeshClusterManager::startUploadClusterData()
        {
            sbCopyingDataFromMainThread.store(true);
        }

        /*
        **
        */
        void CMeshClusterManager::updateConstantBuffer()
        {
            mpRenderer->beginDebugMarker("Update Cluster Constant Buffers");

            // update to GPU: cluster constant buffer info
            //if(mpTotalClusterConstantBuffer)
            {
                float const kfClusterErrorThreshold = 2.0f;

#if 0
                if(sfZ >= 2.07f)
                {
                    sfInc = -1.0f;
                }
                else if(sfZ <= 2.06f)
                {
                    sfInc = 1.0f;
                }
#endif // #if 0

                if(sfZ >= 5.2f)
                {
                    sfInc = -1.0f;
                }
                else if(sfZ <= 1.0f)
                {
                    sfInc = 1.0f;
                }

//DEBUG_PRINTF("z: %.4f\n", sfZ);

auto start = std::chrono::high_resolution_clock::now();

                //std::vector<uint8_t> acUploadData(CONSTANT_BUFFER_SIZE);
                CCamera camera;
                camera.setPosition(float3(0.0f, 0.0f, sfZ += sfInc * 0.005f));
                //camera.setPosition(float3(0.0f, 0.0f, sfZ += sfInc * 0.0008f));
                //camera.setPosition(float3(0.0f, 0.0f, 1.35f));
                camera.setLookAt(float3(0.0f, 0.0f, 0.0f));
                camera.setProjectionType(PROJECTION_PERSPECTIVE);

                CameraUpdateInfo cameraInfo;
                cameraInfo.mfFieldOfView = 3.14159f * 0.5f;
                cameraInfo.mfViewHeight = 1024.0f;
                cameraInfo.mfViewWidth = 1024.0f;
                cameraInfo.mUp = vec3(0.0f, 1.0f, 0.0f);
                cameraInfo.mfNear = 0.1f;
                cameraInfo.mfFar = 100.0f;
                camera.update(cameraInfo);

                float4x4 const& viewMatrix = camera.getViewMatrix();
                float4x4 const& projectionMatrix = camera.getProjectionMatrix();

                float4x4 viewProjection = projectionMatrix * viewMatrix;

                float4x4* pTransformMatrix = nullptr;

                struct ConstantBufferInfo
                {
                    float4x4        mViewMatrix;
                    float4x4        mProjectionMatrix;
                    float4x4        mModelMatrix;
                    float4x4        mRotationMatrix;
                    float4x4        mLastViewMatrix;
                    uint32_t        miGeneralInfoAddress;
                    uint32_t        miDrawClusterIndexBufferSize;
                    float           mfClusterErrorThreshold;
                    uint32_t        miScreenWidth;
                    uint32_t        miScreenHeight;
                };

                // fill data
                {
                    float4x4* pMatrix = reinterpret_cast<float4x4*>(macUploadConstantBufferData.data());
                    *pMatrix++ = viewMatrix;
                    *pMatrix++ = projectionMatrix;

                    pTransformMatrix = pMatrix;

                    *pMatrix++ = float4x4();
                    *pMatrix++ = float4x4();
                    *pMatrix++ = mLastViewMatrix;
                    
                    // TODO: get the address here
                    uint64_t iConstantBufferAddress = 0;

                    uint32_t* pDataInt = reinterpret_cast<uint32_t*>(pMatrix);
                    *pDataInt++ = (uint32_t)iConstantBufferAddress;
                    *pDataInt++ = static_cast<uint32_t>(maDrawClusterInfo.size());

                    float* pDataFloat = reinterpret_cast<float*>(pDataInt);
                    *pDataFloat++ = kfClusterErrorThreshold;
                
                    pDataInt = reinterpret_cast<uint32_t*>(pDataFloat);
                    *pDataInt++ = mpRenderer->getDescriptor().miScreenWidth;
                    *pDataInt++ = mpRenderer->getDescriptor().miScreenHeight;

                }   // fill data
                
                // mesh instance's constant buffer data
                uint32_t iTransformMatrixOffset = static_cast<uint32_t>(sizeof(float4x4) * 2);
                uint32_t iRotationMatrixOffset = static_cast<uint32_t>(sizeof(float4x4) * 3);
                for(uint32_t i = 0; i < static_cast<uint32_t>(maMeshInstanceInfo.size()); i++)
                {
                    maacMeshInstanceConstantBufferData[i] = macUploadConstantBufferData;
                    float4x4* pTransformMatrix = reinterpret_cast<float4x4*>(maacMeshInstanceConstantBufferData[i].data() + iTransformMatrixOffset);
                    float4x4* pRotationMatrix = reinterpret_cast<float4x4*>(maacMeshInstanceConstantBufferData[i].data() + iRotationMatrixOffset);

                    *pTransformMatrix = maMeshInstanceInfo[i].mTransformMatrix;
                    *pRotationMatrix = maMeshInstanceInfo[i].mRotationMatrix;
                }

                ConstantBufferInfo* pConstantBufferInfo = (ConstantBufferInfo*)macUploadConstantBufferData.data();

                mLastViewMatrix = viewMatrix;

uint64_t iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

                
            }

            mpRenderer->endDebugMarker();
        }

        /*
        **
        */
        void CMeshClusterManager::updateGPUMeshData()
        {
            mpRenderer->beginDebugMarker("Update Cluster GPU Data");

            // check any vertex and index data need to be uploaded
            if(maaUploadClusterTriangleVertexData.size() > 0 && sbCopyingDataFromMainThread.load() == false)
            {
                sbUploadingToGPU.store(true);

                uint32_t const kiNumUploadClustersPerFrame = 3000;
                uint32_t iUploadEndClusterIndex = miUploadStartIndex + kiNumUploadClustersPerFrame;

                std::vector<std::vector<ConvertedMeshVertexFormat>> aaTotalUploadClusterTriangleVertexData(1);
                std::vector<std::vector<uint32_t>> aaiTotalUploadClusterTriangleIndexData(1);
                std::vector<std::pair<uint32_t, uint32_t>> aVertexBufferDataChunkRanges(1);
                std::vector<std::pair<uint32_t, uint32_t>> aIndexBufferDataChunkRanges(1);

                // upload to gpu
                uint32_t iNumClustersToUpload = static_cast<uint32_t>(maaUploadClusterTriangleVertexData.size());
                {
                    DEBUG_PRINTF("upload cluster %d of %d\n", miUploadStartIndex, iNumClustersToUpload);

                    WTFASSERT(maUploadClusterInfoGPU.size() > 0, "empty cluster info for uploading to gpu");
                    WTFASSERT(maUploadDataOffsets.size() > 0, "empty upload data offset for uploading to gpu");

                    // group into one big data chunk to be uploaded
                    iUploadEndClusterIndex = min(iNumClustersToUpload - 1, iUploadEndClusterIndex);
                    uint32_t iCurrEndVertexDataOffset = UINT32_MAX, iCurrEndIndexDataOffset = UINT32_MAX;
                    uint32_t iVertexDataChunkIndex = 0, iIndexDataChunkIndex = 0;
                    for(uint32_t i = miUploadStartIndex; i <= iUploadEndClusterIndex; i++)
                    {
                        // check contiguous array
                        if(iCurrEndVertexDataOffset == maUploadDataOffsets[i].first || iCurrEndVertexDataOffset == UINT32_MAX)
                        {
                            aaTotalUploadClusterTriangleVertexData[iVertexDataChunkIndex].insert(
                                aaTotalUploadClusterTriangleVertexData[iVertexDataChunkIndex].end(),
                                maaUploadClusterTriangleVertexData[i].begin(),
                                maaUploadClusterTriangleVertexData[i].end());

                            if(iCurrEndVertexDataOffset == UINT32_MAX)
                            {
                                iCurrEndVertexDataOffset = maUploadDataOffsets[i].first;
                                aVertexBufferDataChunkRanges[iVertexDataChunkIndex].first = maUploadDataOffsets[i].first;
                            }

                            iCurrEndVertexDataOffset += static_cast<uint32_t>(MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat));
                            aVertexBufferDataChunkRanges[iVertexDataChunkIndex].second = iCurrEndVertexDataOffset;
                        }
                        else
                        {
                            aaTotalUploadClusterTriangleVertexData.resize(aaTotalUploadClusterTriangleVertexData.size() + 1);
                            iVertexDataChunkIndex = static_cast<uint32_t>(aaTotalUploadClusterTriangleVertexData.size() - 1);

                            aaTotalUploadClusterTriangleVertexData[iVertexDataChunkIndex].insert(
                                aaTotalUploadClusterTriangleVertexData[iVertexDataChunkIndex].end(),
                                maaUploadClusterTriangleVertexData[i].begin(),
                                maaUploadClusterTriangleVertexData[i].end());

                            aVertexBufferDataChunkRanges.resize(aVertexBufferDataChunkRanges.size() + 1);
                            aVertexBufferDataChunkRanges[iVertexDataChunkIndex].first = maUploadDataOffsets[i].first;
                            iCurrEndVertexDataOffset = maUploadDataOffsets[i].first + static_cast<uint32_t>(MAX_NUM_CLUSTER_VERTICES * sizeof(ConvertedMeshVertexFormat));
                            aVertexBufferDataChunkRanges[iVertexDataChunkIndex].second = iCurrEndVertexDataOffset;
                        }
                    }

                    uint32_t iVertexDataStartOffset = maUploadDataOffsets[miUploadStartIndex].first;

                    for(uint32_t iDataChunk = 0; iDataChunk < static_cast<uint32_t>(aaTotalUploadClusterTriangleVertexData.size()); iDataChunk++)
                    {
                        uint32_t iDataSize = static_cast<uint32_t>(aaTotalUploadClusterTriangleVertexData[iDataChunk].size() * sizeof(ConvertedMeshVertexFormat));
                        uint32_t iVertexDataStartOffset = aVertexBufferDataChunkRanges[iDataChunk].first;
                        WTFASSERT(aVertexBufferDataChunkRanges[iDataChunk].second - aVertexBufferDataChunkRanges[iDataChunk].first == iDataSize, "incorrect data size %d to %d",
                            iDataSize,
                            aVertexBufferDataChunkRanges[iDataChunk].second - aVertexBufferDataChunkRanges[iDataChunk].first);

                        WTFASSERT(
                            iVertexDataStartOffset + iDataSize < mpTotalClusterVertexDataBuffer->getDescriptor().miSize,
                            "vertex data offset: %d size: %d (%d) is out of bounds %d / %d",
                            iVertexDataStartOffset,
                            iDataSize,
                            iVertexDataStartOffset + iDataSize,
                            iVertexDataStartOffset + iDataSize,
                            mpTotalClusterVertexDataBuffer->getDescriptor().miSize);

                        mpRenderer->copyCPUToBuffer(
                            mpTotalClusterVertexDataBuffer,
                            aaTotalUploadClusterTriangleVertexData[iDataChunk].data(),
                            iVertexDataStartOffset,
                            iDataSize);
                    }

#if 0
                    for(uint32_t iDataChunk = 0; iDataChunk < static_cast<uint32_t>(aaiTotalUploadClusterTriangleIndexData.size()); iDataChunk++)
                    {
                        uint32_t iDataSize = static_cast<uint32_t>(aaiTotalUploadClusterTriangleIndexData[iDataChunk].size() * sizeof(uint32_t));
                        uint32_t iIndexDataStartOffset = aIndexBufferDataChunkRanges[iDataChunk].first;
                        WTFASSERT(aIndexBufferDataChunkRanges[iDataChunk].second - aIndexBufferDataChunkRanges[iDataChunk].first == iDataSize, "incorrect data size %d to %d",
                            iDataSize,
                            aIndexBufferDataChunkRanges[iDataChunk].second - aIndexBufferDataChunkRanges[iDataChunk].first);

                        WTFASSERT(
                            iIndexDataStartOffset + iDataSize < mpTotalClusterIndexDataBuffer->getDescriptor().miSize,
                            "index data offset: %d size: %d (%d) is out of bounds %d / %d",
                            iIndexDataStartOffset,
                            iDataSize,
                            iIndexDataStartOffset + iDataSize,
                            iIndexDataStartOffset + iDataSize,
                            mpTotalClusterIndexDataBuffer->getDescriptor().miSize);

                        mpRenderer->copyCPUToBuffer(
                            mpTotalClusterIndexDataBuffer,
                            aaiTotalUploadClusterTriangleIndexData[iDataChunk].data(),
                            iIndexDataStartOffset,
                            iDataSize);
                    }
#endif // #if 0

                    mpRenderer->executeUploadCommandBufferAndWait();
                }

                if(iUploadEndClusterIndex >= iNumClustersToUpload - 1)
                {
                    // done loading all the cluster vertex and index data

                    uint64_t iVertexDataBufferGPUAddress = mpTotalClusterVertexDataBuffer->getGPUVirtualAddress();

                    // copy cluster table of content 
                    {
                        uint64_t iTotalUploadClusterSize = maUploadClusterInfoGPU.size() * sizeof(UploadClusterInfo);
                        std::vector<uint8_t> aUploadBuffer(iTotalUploadClusterSize + sizeof(uint32_t) + 2 * sizeof(uint64_t));
                        uint32_t* piData = reinterpret_cast<uint32_t*>(aUploadBuffer.data());
                        *piData++ = static_cast<uint32_t>(maUploadClusterInfoGPU.size());
                        uint64_t* piData64 = reinterpret_cast<uint64_t*>(piData);
                        *piData64++ = iVertexDataBufferGPUAddress;

                        uint8_t* pcData = reinterpret_cast<uint8_t*>(piData64);
                        memcpy(pcData, maUploadClusterInfoGPU.data(), iTotalUploadClusterSize);
                        
                        mpRenderer->copyCPUToBuffer(
                            mpClusterDataTOCBuffer,
                            aUploadBuffer.data(),
                            0,
                            static_cast<uint32_t>(aUploadBuffer.size()));
                        mpRenderer->executeUploadCommandBufferAndWait();

                        DEBUG_PRINTF("%lld bytes uploaded\n", aUploadBuffer.size());
                    }

                    // hash map for getting the location of the clusters' vertex and index data
                    {
                        std::vector<char> acHashMap(1 << 22);
                        for(uint32_t i = 0; i < static_cast<uint32_t>(maUpdatingClusterInfo.size()); i++)
                        {
                            auto const& uploadClusterInfo = maUpdatingClusterInfo[i];
                            Render::Common::Utils::addToHashMap(
                                uint2(uploadClusterInfo.miMesh, uploadClusterInfo.miCluster),
                                i,
                                acHashMap,
                                static_cast<uint32_t>(acHashMap.size())
                            );

                        }   // for uploadClusterInfo in maUpdatingClusterInfo

                        mpRenderer->copyCPUToBuffer(
                            mpClusterDataIndexHashMap,
                            acHashMap.data(),
                            0,
                            static_cast<uint32_t>(acHashMap.size()));
                        mpRenderer->executeUploadCommandBufferAndWait();
                    }


                    // clear upload buffers
                    for(auto& aUploadClusterTriangleVertexData : maaUploadClusterTriangleVertexData)
                    {
                        aUploadClusterTriangleVertexData.clear();
                    }
                    maaUploadClusterTriangleVertexData.clear();

                    for(auto& aUploadClusterTriangleIndexData : maaUploadClusterTriangleIndexData)
                    {
                        aUploadClusterTriangleIndexData.clear();
                    }
                    maaUploadClusterTriangleIndexData.clear();

                    maUploadClusterInfoGPU.clear();
                    maUploadDataOffsets.clear();

                    miUploadStartIndex = 0;

                    sbUploadingToGPU.store(false);
                }
                else
                {
                    // still loading, upload the next start index  
                    miUploadStartIndex += kiNumUploadClustersPerFrame;
                }
            }

            mpRenderer->endDebugMarker();
        }

        /*
        **
        */
        void CMeshClusterManager::updateMeshInstanceInfo()
        {
            mpRenderer->beginDebugMarker("Update Mesh Instance Info");
            {
                uint32_t iNumMeshInstances = static_cast<uint32_t>(maMeshInstanceInfo.size());
                std::vector<uint8_t> acData(1 << 16);
                uint32_t* pDataInt = reinterpret_cast<uint32_t*>(acData.data());
                uint32_t* pStart = pDataInt;
                *pDataInt++ = iNumMeshInstances;            // num mesh instance info

                for(uint32_t iMeshInstance = 0; iMeshInstance < iNumMeshInstances; iMeshInstance++)
                {
                    *pDataInt++ = maMeshInstanceInfo[iMeshInstance].miMesh;
                    *pDataInt++ = maMeshInstanceInfo[iMeshInstance].miInstance;
                    
                    float4x4* pDataFloat4x4 = reinterpret_cast<float4x4*>(pDataInt);
                    *pDataFloat4x4++ = maMeshInstanceInfo[iMeshInstance].mTransformMatrix;
                    *pDataFloat4x4++ = maMeshInstanceInfo[iMeshInstance].mRotationMatrix;

                    pDataInt = reinterpret_cast<uint32_t*>(pDataFloat4x4);
                    *pDataInt++ = maMeshInstanceInfo[iMeshInstance].miClusterStartIndex;
                    *pDataInt++ = maMeshInstanceInfo[iMeshInstance].miClusterGroupStartIndex;
                    *pDataInt++ = maMeshInstanceInfo[iMeshInstance].miRootClusterStartIndex;
                    *pDataInt++ = maMeshInstanceInfo[iMeshInstance].miTotalMeshInstanceIndex;

                    uint64_t iSize = reinterpret_cast<uint64_t>(pDataInt) - reinterpret_cast<uint64_t>(pStart);
                    WTFASSERT(iSize < acData.size(), "mesh instance info buffer out of bounds %d (%d)", iSize, acData.size());
                }

                uint32_t iFlags =
                    static_cast<uint32_t>(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) |
                    static_cast<uint32_t>(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION) |
                    static_cast<uint32_t>(Render::Common::CopyBufferFlags::BEGIN_MARKER) | 
                    static_cast<uint32_t>(Render::Common::CopyBufferFlags::END_MARKER);
                mpRenderer->copyCPUToBuffer2(
                    mUploadCommandBuffer.get(),
                    mpMeshInstanceInfoBuffer,
                    maUploadBuffers[0].get(),
                    acData.data(),
                    0,
                    0,
                    static_cast<uint32_t>(acData.size()),
                    iFlags);
                mUploadCommandAllocator->reset();
                mUploadCommandBuffer->reset();
            }
            mpRenderer->endDebugMarker();
        }

        /*
        **
        */
        void CMeshClusterManager::updateRootClusterInfo()
        {
            if(mpRootClusterInfoBuffer == nullptr)
            {
                return;
            }

            std::vector<ClusterTreeNode> aTotalRootClusterNodes;
            for(auto const& aRootClusterNodes : maaRootClusterNodes)
            {
                aTotalRootClusterNodes.insert(
                    aTotalRootClusterNodes.end(), 
                    aRootClusterNodes.second.begin(), 
                    aRootClusterNodes.second.end());
            }

            std::vector<uint8_t> acUploadData(sizeof(uint32_t) + sizeof(uint32_t) * (2 * (maaRootClusterNodes.size() + 1)) + sizeof(ClusterTreeNode) * aTotalRootClusterNodes.size());
            uint32_t* pDataInt = reinterpret_cast<uint32_t*>(acUploadData.data());
            *pDataInt++ = static_cast<uint32_t>(maaRootClusterNodes.size());
            
            uint32_t iDataOffset = 0;
            for(uint32_t i = 0; i < static_cast<uint32_t>(maaRootClusterNodes.size()); i++)
            {
                // start
                *pDataInt++ = iDataOffset;

                iDataOffset += static_cast<uint32_t>(maaRootClusterNodes[i].size());

                *pDataInt++ = (iDataOffset - 1);
                
            }

            memcpy(
                pDataInt, 
                aTotalRootClusterNodes.data(), 
                sizeof(ClusterTreeNode) * aTotalRootClusterNodes.size());

            uint32_t iFlags =
                static_cast<uint32_t>(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) |
                static_cast<uint32_t>(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION) |
                static_cast<uint32_t>(Render::Common::CopyBufferFlags::BEGIN_MARKER) |
                static_cast<uint32_t>(Render::Common::CopyBufferFlags::END_MARKER);
            mpRenderer->copyCPUToBuffer2(
                mUploadCommandBuffer.get(),
                mpRootClusterInfoBuffer,
                maUploadBuffers[0].get(),
                acUploadData.data(),
                0,
                0,
                static_cast<uint32_t>(acUploadData.size()),
                iFlags);
            mUploadCommandAllocator->reset();
            mUploadCommandBuffer->reset();
        }

        /*
        **
        */
        void CMeshClusterManager::updateClusterAddressQueue()
        {
            uint32_t const kiNumMeshInstancesPerDispatchGroup = 20;

            mpRenderer->beginDebugMarker("Update Cluster Address Queue");

            if(maiVisibilityCounterData.size() <= 0)
            {
                maiVisibilityCounterData.resize(mpVisibilityCountBuffer->getDescriptor().miSize / sizeof(uint32_t));
            }

            uint64_t aiElapsed[10];
            for(uint32_t i = 0; i < sizeof(aiElapsed) / sizeof(*aiElapsed); i++)
            {
                aiElapsed[i] = 0;
            }

            mpRenderer->beginDebugMarker("Add Initial Root Clusters");
            
            auto start = std::chrono::high_resolution_clock::now();
            
            uint32_t iNumTotalClusterGroupIndices = 0;
            uint32_t iMeshInstanceIndex = 0, iNumTotalClusters = 0, iNumTotalClusterGroups = 0;
            uint32_t iNumTotalClusterQueueInfo = 0;
            for(auto const& meshInstanceInfo : maMeshInstanceInfo)
            {
                uint32_t iMesh = meshInstanceInfo.miMesh;
                auto const& meshInfo = maMeshInfo[iMesh];
                uint32_t iNumMeshRootClusters = static_cast<uint32_t>((meshInfo.miNumClusters + meshInfo.miClusterNodeStartIndex) - meshInfo.miClusterRootNodeStartIndex);
                uint32_t iLocalRootNodeStartIndex = meshInfo.miClusterRootNodeStartIndex - meshInfo.miClusterNodeStartIndex;
                
                uint32_t iNumClusterQueueInfo = 0;
                if(maaClusterQueueInfo[iMeshInstanceIndex].size() < iNumMeshRootClusters)
                {
                    maaClusterQueueInfo[iMeshInstanceIndex].resize(iNumMeshRootClusters);
                }

                auto start0 = std::chrono::high_resolution_clock::now();

                for(uint32_t iRootCluster = 0; iRootCluster < iNumMeshRootClusters; iRootCluster++)
                {
                    MeshClusterTreeQueueInfo clusterTreeQueueInfo;
                    clusterTreeQueueInfo.miMesh = meshInstanceInfo.miMesh;
                    clusterTreeQueueInfo.miInstance = meshInstanceInfo.miInstance;
                    
                    WTFASSERT(iLocalRootNodeStartIndex + iRootCluster < maClusterTreeNodes[iMesh].size(), "out of bounds root node index %d (%d)", iLocalRootNodeStartIndex, maClusterTreeNodes[iMesh].size());
                    clusterTreeQueueInfo.miClusterAddress = maClusterTreeNodes[iMesh][iLocalRootNodeStartIndex + iRootCluster].miClusterAddress;
                    clusterTreeQueueInfo.miAbsoluteInstanceIndex = iMeshInstanceIndex;
                    if(maClusterQueueInfo.size() <= iNumTotalClusterQueueInfo)
                    {
                        maClusterQueueInfo.resize(maClusterQueueInfo.size() + 1024);
                    }
                    maClusterQueueInfo[iNumTotalClusterQueueInfo] = clusterTreeQueueInfo;
                    ++iNumTotalClusterQueueInfo;

                    maaClusterQueueInfo[iMeshInstanceIndex][iNumClusterQueueInfo] = clusterTreeQueueInfo;
                    ++iNumClusterQueueInfo;
                }

                uint64_t iElapsed0 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start0).count();
                aiElapsed[0] += iElapsed0;

                auto start1 = std::chrono::high_resolution_clock::now();

                // all the cluster group address for calculating cluster error terms
                uint32_t iStartClusterGroupIndex = iNumTotalClusterGroupIndices;
                iNumTotalClusterGroupIndices += meshInfo.miNumClusterGroups;
#if 0
                for(uint32_t iClusterGroup = 0; iClusterGroup < meshInfo.miNumClusterGroups; iClusterGroup++)
                {
                    if(maTotalClusterGroupIndices.size() <= iNumTotalClusterGroupIndices)
                    {
                        maTotalClusterGroupIndices.resize(maTotalClusterGroupIndices.size() + 4096);
                    }

                    MeshInstanceClusterGroupInfo& info = *(maTotalClusterGroupIndices.data() + iNumTotalClusterGroupIndices);
                    ++iNumTotalClusterGroupIndices;
                    info.miClusterGroupAddress = meshInfo.miClusterGroupNodeStartIndex + iClusterGroup;
                    info.miMeshInstanceIndex = iMeshInstanceIndex;
                }
#endif // #if 0

                uint64_t iElapsed1 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start1).count();
                aiElapsed[1] += iElapsed1;

                auto start2 = std::chrono::high_resolution_clock::now();

                // mesh instance cluster queue info
                {
                    MeshClusterTreeQueueInfo const& clusterTreeQueueInfo = maaClusterQueueInfo[iMeshInstanceIndex][0];

                    maiVisibilityCounterData[(iMeshInstanceIndex + 1) * 8] = static_cast<uint32_t>(maaClusterQueueInfo[iMeshInstanceIndex].size());
                    maiVisibilityCounterData[(iMeshInstanceIndex + 1) * 8 + 1] = 0;
                    maiVisibilityCounterData[(iMeshInstanceIndex + 1) * 8 + 2] = 0;
                    maiVisibilityCounterData[(iMeshInstanceIndex + 1) * 8 + 3] = maMeshInfo[clusterTreeQueueInfo.miMesh].miNumClusters;
                    maiVisibilityCounterData[(iMeshInstanceIndex + 1) * 8 + 4] = maMeshInfo[clusterTreeQueueInfo.miMesh].miNumClusterGroups;
                    maiVisibilityCounterData[(iMeshInstanceIndex + 1) * 8 + 5] = 0;
                    maiVisibilityCounterData[(iMeshInstanceIndex + 1) * 8 + 6] = iStartClusterGroupIndex;
                    maiVisibilityCounterData[(iMeshInstanceIndex + 1) * 8 + 7] = iNumTotalClusterGroupIndices - iStartClusterGroupIndex;
                }
                
                uint64_t iElapsed2 = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start2).count();
                aiElapsed[2] += iElapsed2;

                iNumTotalClusters += meshInfo.miNumClusters;
                iNumTotalClusterGroups += meshInfo.miNumClusterGroups;

                ++iMeshInstanceIndex;
            
            }   // for mesh instance to num mesh instances      

            uint64_t iElapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start).count();

            mpRenderer->endDebugMarker();

            mpRenderer->beginDebugMarker("Upload Data To GPU");
            maiVisibilityCounterData[0] = iNumTotalClusterQueueInfo;
            maiVisibilityCounterData[1] = 0;
            maiVisibilityCounterData[2] = 0;
            maiVisibilityCounterData[3] = iNumTotalClusters;
            maiVisibilityCounterData[4] = iNumTotalClusterGroupIndices; 
            maiVisibilityCounterData[5] = 0;
            maiVisibilityCounterData[6] = kiNumMeshInstancesPerDispatchGroup;

            uint32_t iFlags = static_cast<uint32_t>(Render::Common::CopyBufferFlags::BEGIN_MARKER);
            uint32_t iNumPartionedCommands = static_cast<uint32_t>(ceil(static_cast<float>(maaClusterQueueInfo.size()) / static_cast<float>(kiNumMeshInstancesPerDispatchGroup)));

#if 0
            // TODO: change this to thread's own memory space for queue instead of each cluster memory space
            // ie. 64 threads => 64 * max memory size space per threads, instead of
            //     100000 mesh instances => 100000 * num clusters in mesh instance
            // Probably don't need this later on, going to let gpu fill out the queue with entries in: 
            // mpInitialMeshInstanceRootClusterBuffer
            // using groupID.x to indicate the initial root clusters to use 
            for(uint32_t iCommand = 0; iCommand < iNumPartionedCommands; iCommand++)
            {
                uint32_t iSrcDataOffset = 0;
                uint32_t iAbsoluteInstanceIndexStart = maaClusterQueueInfo[iCommand * kiNumMeshInstancesPerDispatchGroup][0].miAbsoluteInstanceIndex;
                for(uint32_t iClusterQueueInfo = 0; iClusterQueueInfo < static_cast<uint32_t>(maaClusterQueueInfo.size()); iClusterQueueInfo++)
                {
                    if(iClusterQueueInfo > 0)
                    {
                        iFlags = 0;
                    }

                    // multiple uploads require multiple upload buffers

                    MeshClusterTreeQueueInfo const& queueInfo = maaClusterQueueInfo[iClusterQueueInfo][0];
                    uint32_t iClusterDataOffset = (maMeshInstanceInfo[queueInfo.miAbsoluteInstanceIndex].miClusterStartIndex - iAbsoluteInstanceIndexStart);
                    uint32_t iDestOffset = iClusterDataOffset * sizeof(MeshClusterTreeQueueInfo);
                    //mpRenderer->copyCPUToBuffer3(
                    //    mUploadCommandBuffer.get(),
                    //    mpClusterTraverseBuffer,
                    //    maUploadBuffers[0].get(),
                    //    maClusterQueueInfo.data(),
                    //    iSrcDataOffset,
                    //    iDestOffset,
                    //    static_cast<uint32_t>(maaClusterQueueInfo[iClusterQueueInfo].size() * sizeof(MeshClusterTreeQueueInfo)),
                    //    static_cast<uint32_t>(maClusterQueueInfo.size() * sizeof(MeshClusterTreeQueueInfo)),
                    //    iFlags);

                    iSrcDataOffset += static_cast<uint32_t>(maaClusterQueueInfo[iClusterQueueInfo].size() * sizeof(MeshClusterTreeQueueInfo));
                }
            }
#endif // #if 0

#if 0
            mpRenderer->copyCPUToBuffer2(
                mUploadCommandBuffer.get(),
                mpInitialClusterGroupBuffer,
                maUploadBuffers[1].get(),
                maTotalClusterGroupIndices.data(),
                0,
                0,
                uint32_t(sizeof(MeshInstanceClusterGroupInfo) * iNumTotalClusterGroupIndices), //uint32_t(sizeof(MeshInstanceClusterGroupInfo)* maTotalClusterGroupIndices.size()),
                0);
#endif // #if 0

            mpRenderer->beginDebugMarker("Upload Table of Content for Cluster Queue");

            // cluster queue info: number of root clusters follow by the root cluster index
            uint32_t iDestDataOffset = 0;
            std::vector<uint32_t> aiSizes(maaClusterQueueInfo.size());
            std::vector<char> acUploadData(1 << 16);
            uint32_t* piData = reinterpret_cast<uint32_t*>(acUploadData.data());
            uint32_t* pStart = piData;

            // total number of mesh instances
            *piData++ = static_cast<uint32_t>(maaClusterQueueInfo.size());

            struct TOCEntry
            {
                uint32_t        miNumInitialRootClusters;
                uint32_t        miDataOffset;
            };

            // build table of content with number of initial root clusters and data offset
            uint32_t iTotalNumInitialClusters = 0;
            TOCEntry* pTOCEntry = reinterpret_cast<TOCEntry*>(piData);
            for(auto const& aClusterQueueInfo : maaClusterQueueInfo)
            {
                pTOCEntry->miNumInitialRootClusters = static_cast<uint32_t>(aClusterQueueInfo.size());
                pTOCEntry->miDataOffset = static_cast<uint32_t>(iTotalNumInitialClusters * sizeof(MeshClusterTreeQueueInfo) + maaClusterQueueInfo.size() * sizeof(TOCEntry) + sizeof(uint32_t));
                iTotalNumInitialClusters += static_cast<uint32_t>(aClusterQueueInfo.size());
                ++pTOCEntry;
            }

            // initial root clusters
            MeshClusterTreeQueueInfo* pData = reinterpret_cast<MeshClusterTreeQueueInfo*>(pTOCEntry);
            for(uint32_t iIndex = 0; iIndex < static_cast<uint32_t>(maaClusterQueueInfo.size()); iIndex++)
            {
                for(uint32_t i = 0; i < maaClusterQueueInfo[iIndex].size(); i++)
                {
                    *pData++ = maaClusterQueueInfo[iIndex][i];
                }
            }
            uint32_t iDataSize = static_cast<uint32_t>(reinterpret_cast<uint64_t>(pData) - reinterpret_cast<uint64_t>(pStart));
            mpRenderer->copyCPUToBuffer2(
                mUploadCommandBuffer.get(),
                mpInitialMeshInstanceRootClusterBuffer,
                maUploadBuffers[3].get(),
                acUploadData.data(),
                0,
                0,
                iDataSize,
                0);

            mpRenderer->endDebugMarker();
#if 0
            // test for new method of filling out traversal cluster queue
            {
#define in 
#define out

                std::vector<char> acTraversalBuffer(1 << 20);

                std::vector<char> acMeshInfoBuffer(1 << 16);
                {
                    uint32_t* piData = reinterpret_cast<uint32_t*>(acMeshInfoBuffer.data());
                    *piData++ = static_cast<uint32_t>(maMeshInfo.size());
                    memcpy(piData, maMeshInfo.data(), maMeshInfo.size() * sizeof(MeshInfo));
                }

                std::vector<char> acMeshInstanceInfoBuffer(1 << 16);
                {
                    uint32_t* piData = reinterpret_cast<uint32_t*>(acMeshInstanceInfoBuffer.data());
                    *piData++ = static_cast<uint32_t>(maMeshInstanceInfo.size());
                    memcpy(piData, maMeshInstanceInfo.data(), maMeshInstanceInfo.size() * sizeof(MeshInstanceInfo));
                }

                uint3 groupID = uint3(6, 0, 0);
                uint3 localThreadGroup = uint3(0, 0, 0);

                using namespace Test::HLSL;
                ByteAddressBuffer TOCBuffer = reinterpret_cast<ByteAddressBuffer>(pStart);
                ByteAddressBuffer meshInfoBuffer = reinterpret_cast<ByteAddressBuffer>(acMeshInfoBuffer.data());
                ByteAddressBuffer meshInstanceInfoBuffer = reinterpret_cast<ByteAddressBuffer>(acMeshInstanceInfoBuffer.data());
                ByteAddressBuffer traverseAddressBuffer = reinterpret_cast<ByteAddressBuffer>(acTraversalBuffer.data());

                auto loadMeshInfo = [](MeshInfo& meshInfo,
                    out uint32_t& iNumMeshes,
                    in ByteAddressBuffer buffer,
                    in uint32_t iMesh)
                {
                    ByteAddress address = { 0 };
                    iNumMeshes = loadUInt(buffer, address);

                    address.miPtr += iMesh * sizeof(MeshInfo);
                    meshInfo.miID = loadUInt(buffer, address);
                    meshInfo.miClusterGroupNodeStartIndex = loadUInt(buffer, address);
                    meshInfo.miClusterNodeStartIndex = loadUInt(buffer, address);
                    meshInfo.miNumClusterGroups = loadUInt(buffer, address);
                    meshInfo.miNumClusters = loadUInt(buffer, address);
                    meshInfo.miMaxMIPLevel = loadUInt(buffer, address);
                    meshInfo.miClusterRootNodeStartIndex = loadUInt(buffer, address);
                    meshInfo.miNumRootClusters = loadUInt(buffer, address);
                    meshInfo.miClusterGroupRootNodeStartIndex = loadUInt(buffer, address);
                    meshInfo.miNumRootClusterGroups = loadUInt(buffer, address);
                };

                auto loadMeshInstanceInfo2 = [](
                    out MeshInstanceInfo& meshInstanceInfo,
                    out uint32_t& iNumMeshInstanceInfo,
                    in ByteAddressBuffer meshInstanceInfoBuffer,
                    in uint32_t iIndex)
                {
                    ByteAddress address = { 0 };
                    iNumMeshInstanceInfo = loadUInt(meshInstanceInfoBuffer, address);

                    address.miPtr += sizeof(MeshInstanceInfo) * iIndex;
                    meshInstanceInfo.miMesh = loadUInt(meshInstanceInfoBuffer, address);
                    meshInstanceInfo.miInstance = loadUInt(meshInstanceInfoBuffer, address);
                    meshInstanceInfo.mTransformMatrix = loadMatrix(meshInstanceInfoBuffer, address);
                    meshInstanceInfo.mRotationMatrix = loadMatrix(meshInstanceInfoBuffer, address);
                    meshInstanceInfo.miClusterStartIndex = loadUInt(meshInstanceInfoBuffer, address);
                    meshInstanceInfo.miClusterGroupStartIndex = loadUInt(meshInstanceInfoBuffer, address);
                    meshInstanceInfo.miRootClusterStartIndex = loadUInt(meshInstanceInfoBuffer, address);
                    meshInstanceInfo.miTotalMeshInstanceIndex = loadUInt(meshInstanceInfoBuffer, address);
                };

                auto getQueueDataAddress = [loadMeshInstanceInfo2, loadMeshInfo](
                    in ByteAddressBuffer meshInstanceInfoBuffer,
                    in ByteAddressBuffer meshInfoBuffer,
                    in uint32_t iMeshInstanceIndex,
                    in uint32_t iNumMeshesPerWorkGroupDispatch)
                {
                    uint32_t iWorkGroupMeshInstance = iMeshInstanceIndex % iNumMeshesPerWorkGroupDispatch;
                    uint32_t iStartingMeshInstanceIndex = (iMeshInstanceIndex / iNumMeshesPerWorkGroupDispatch) * iNumMeshesPerWorkGroupDispatch;
                    uint32_t iDataAddress = 0;
                    for(uint32_t iMeshInstanceInfo = 0; iMeshInstanceInfo < iWorkGroupMeshInstance; iMeshInstanceInfo++)
                    {
                        // get mesh instance and mesh of the mesh instance to get the number of clusters in the mesh for data offset
                        // since the max number of entries in the queue is the number of clusters multiply by size of Cluster Queue Info
                        MeshInstanceInfo meshInstanceInfo;
                        uint32_t iNumMeshInstanceInfo = 0;
                        loadMeshInstanceInfo2(
                            meshInstanceInfo,
                            iNumMeshInstanceInfo,
                            meshInstanceInfoBuffer,
                            iStartingMeshInstanceIndex + iMeshInstanceInfo);
                        
                        MeshInfo meshInfo;
                        uint32_t iNumMeshInfo = 0;
                        loadMeshInfo(
                            meshInfo,
                            iNumMeshInfo,
                            meshInfoBuffer,
                            meshInstanceInfo.miMesh);

                        iDataAddress += meshInfo.miNumClusters * sizeof(MeshClusterTreeQueueInfo);
                    }

                    return iDataAddress;
                };

                uint32_t const kiNumMeshesPerWorkGroupDispatch = 5;

                auto setupClusterQueue = [getQueueDataAddress](
                    out RWByteAddressBuffer& traverseAddressBuffer,
                    in uint32_t iNumMeshesPerWorkGroupDispatch,
                    in ByteAddressBuffer TOCBuffer,
                    in ByteAddressBuffer meshInstanceInfoBuffer,
                    in ByteAddressBuffer meshInfoBuffer,
                    in uint3 groupID)
                {
                    // total number of mesh instances
                    ByteAddress address = { 0 };
                    uint32_t iNumMeshInstances = loadUInt(TOCBuffer, address);

                    // table of content entry
                    address.miPtr += groupID.x * sizeof(TOCEntry);
                    TOCEntry entry;
                    entry.miNumInitialRootClusters = loadUInt(TOCBuffer, address);
                    entry.miDataOffset = loadUInt(TOCBuffer, address);

                    // where to put the queue entries in the shared workgroup queue buffer
                    uint32_t iQueueDataAddress = getQueueDataAddress(
                        meshInstanceInfoBuffer,
                        meshInfoBuffer,
                        groupID.x,
                        iNumMeshesPerWorkGroupDispatch);

                    // place initial root cluster address into workgroup's own queue
                    for(uint32_t i = 0; i < entry.miNumInitialRootClusters; i++)
                    {
                        address.miPtr = (entry.miDataOffset + i * sizeof(MeshClusterTreeQueueInfo));
                        MeshClusterTreeQueueInfo clusterTreeQueueInfo;
                        clusterTreeQueueInfo.miMesh = loadUInt(TOCBuffer, address);
                        clusterTreeQueueInfo.miInstance = loadUInt(TOCBuffer, address);
                        clusterTreeQueueInfo.miClusterAddress = loadUInt(TOCBuffer, address);
                        clusterTreeQueueInfo.miAbsoluteInstanceIndex = loadUInt(TOCBuffer, address);

                        // write to queue
                        ByteAddress saveQueueAddress = { iQueueDataAddress + i * sizeof(MeshClusterTreeQueueInfo) };
                        storeUInt(clusterTreeQueueInfo.miMesh, traverseAddressBuffer, saveQueueAddress);
                        storeUInt(clusterTreeQueueInfo.miInstance, traverseAddressBuffer, saveQueueAddress);
                        storeUInt(clusterTreeQueueInfo.miClusterAddress, traverseAddressBuffer, saveQueueAddress);
                        storeUInt(clusterTreeQueueInfo.miAbsoluteInstanceIndex, traverseAddressBuffer, saveQueueAddress);
                    }
                };

                if(localThreadGroup.x == 0)
                {
                    setupClusterQueue(
                        traverseAddressBuffer,
                        kiNumMeshesPerWorkGroupDispatch,
                        TOCBuffer,
                        meshInstanceInfoBuffer,
                        meshInfoBuffer,
                        groupID);
                }

                GroupMemoryBarrierWithGroupSync();

#undef in
#undef out

            }
#endif // #if 0

            mpRenderer->beginDebugMarker("Upload All Data To GPU");

            iFlags = 
                static_cast<uint32_t>(Render::Common::CopyBufferFlags::EXECUTE_RIGHT_AWAY) |
                static_cast<uint32_t>(Render::Common::CopyBufferFlags::WAIT_AFTER_EXECUTION) |
                static_cast<uint32_t>(Render::Common::CopyBufferFlags::END_MARKER);
            mpRenderer->copyCPUToBuffer2(
                mUploadCommandBuffer.get(),
                mpVisibilityCountBuffer,
                maUploadBuffers[2].get(),
                maiVisibilityCounterData.data(),
                0,
                0,
                static_cast<uint32_t>(maiVisibilityCounterData.size() * sizeof(uint32_t)),
                iFlags);
            
            mUploadCommandAllocator->reset();
            mUploadCommandBuffer->reset();

            mpRenderer->endDebugMarker();

            mpRenderer->endDebugMarker();

            mpRenderer->endDebugMarker();
#if 0
            {
                std::vector<uint32_t> aiQueue;
                std::vector<uint32_t> aiTraversed;
                uint32_t iCurrIndex = 0;
                uint32_t iCluster = maClusterQueueInfo[0].miClusterAddress;
                aiQueue.push_back(iCluster);
                std::vector<uint32_t> aiVisited(3000);
                for(;;)
                {
                    if(iCurrIndex >= aiQueue.size())
                    {
                        std::sort(
                            aiTraversed.begin(),
                            aiTraversed.end(),
                            [](uint32_t const& left, uint32_t const& right)
                            {
                                return left < right;
                            }
                        );

                        break;
                    }

                    uint32_t iCurrClusterAddress = aiQueue[iCurrIndex];
                    if(aiVisited[iCurrClusterAddress] == 0)
                    {
                        aiTraversed.push_back(iCurrClusterAddress);

                        aiVisited[iCurrClusterAddress] = 1;
                        auto const& cluster = maClusterTreeNodes[0][iCurrClusterAddress];
                        for(uint32_t i = 0; i < cluster.miNumChildren; i++)
                        {
                            uint32_t iChildAddress = cluster.maiChildrenAddress[i];
                            if(aiVisited[iChildAddress] == 0)
                            {
                                aiQueue.push_back(iChildAddress);
                            }
                        }
                    }

                    ++iCurrIndex;
                }
            }
#endif // #if 0

        }

        /*
        **
        */
        void CMeshClusterManager::outputDebugClusterOBJ(
            uint32_t iMesh)
        {
            uint32_t const kiMaxVertexBufferSize = MAX_NUM_CLUSTER_VERTICES;
            uint32_t const kiMaxIndexBufferSize = 128 * 3;

            std::string vertexDataFilePath = "c:\\Users\\Dingwings\\demo-models\\debug-output\\dragon\\mesh-cluster-triangle-vertex-data.bin";
            std::string indexDataFilePath = "c:\\Users\\Dingwings\\demo-models\\debug-output\\dragon\\mesh-cluster-triangle-index-data.bin";

            std::string homeDirectory = "c:\\Users\\Dingwings\\demo-models\\run-time-clusters\\dragon\\";
            if(!std::filesystem::exists(homeDirectory))
            {
                std::filesystem::create_directory(homeDirectory);
            }

            std::ostringstream clusterOutputPath;
            clusterOutputPath << homeDirectory;
            clusterOutputPath << "mesh-clusters" << iMesh << "-lod0" << ".obj";
            FILE* fp = fopen(clusterOutputPath.str().c_str(), "wb");
            fprintf(fp, "o mesh-clusters\n");

            std::vector<ConvertedMeshVertexFormat> aClusterTriangleVertices(kiMaxVertexBufferSize);
            std::vector<uint32_t> aiClusterTriangleIndices(kiMaxIndexBufferSize);
            uint32_t iClusterVertexBufferSize = 0, iClusterIndexBufferSize = 0;
            uint32_t iLastLOD = 0;
            uint32_t iNumTotalVertices = 0;
            uint32_t iNumTotalIndices = 0;
            for(uint32_t iCluster = 0; iCluster < maClusterTreeNodes[iMesh].size(); iCluster++)
            {
                auto const& clusterTreeNode = maClusterTreeNodes[iMesh][iCluster];
                if(clusterTreeNode.miLevel != iLastLOD)
                {
                    fclose(fp);
                    std::ostringstream outputPath;
                    outputPath << homeDirectory;
                    outputPath << "mesh-clusters" << iMesh << "-lod" << clusterTreeNode.miLevel << ".obj";
                    fp = fopen(outputPath.str().c_str(), "wb");
                    fprintf(fp, "o mesh-clusters\n");
                    iLastLOD = clusterTreeNode.miLevel;

                    iNumTotalVertices = 0;
                    iNumTotalIndices = 0;
                }

                loadMeshClusterTriangleDataChunk(
                    aClusterTriangleVertices,
                    aiClusterTriangleIndices,
                    iClusterVertexBufferSize,
                    iClusterIndexBufferSize,
                    vertexDataFilePath,
                    indexDataFilePath,
                    maaiNumClusterVertices[iMesh],
                    maaiNumClusterIndices[iMesh],
                    maaiVertexBufferArrayOffsets[iMesh],
                    maaiIndexBufferArrayOffsets[iMesh],
                    iCluster);

                uint32_t iNumVertices = static_cast<uint32_t>(iClusterVertexBufferSize / sizeof(ConvertedMeshVertexFormat));
                uint32_t iNumIndices = static_cast<uint32_t>(iClusterIndexBufferSize / sizeof(uint32_t));

                WTFASSERT(iNumVertices <= aClusterTriangleVertices.size(), "out of bounds number of vertices %d", iNumVertices);
                WTFASSERT(iNumIndices <= aiClusterTriangleIndices.size(), "out of bounds number of vertices %d", iNumIndices);

                std::ostringstream meshName;
                meshName << "mesh" << iMesh << "-lod" << clusterTreeNode.miLevel << "-group" << clusterTreeNode.miClusterGroupAddress << "-cluster" << clusterTreeNode.miClusterAddress;

                std::ostringstream clusterMaterialOutputPath;
                clusterMaterialOutputPath << homeDirectory;
                clusterMaterialOutputPath << meshName.str() << ".mtl";

                fprintf(fp, "g %s\n", meshName.str().c_str());
                fprintf(fp, "mtllib %s\n", clusterMaterialOutputPath.str().c_str());
                fprintf(fp, "usemtl %s\n", meshName.str().c_str());
                fprintf(fp, "# num vertices = %d num faces = %d\n", iNumVertices, iNumIndices / 3);
                for(uint32_t i = 0; i < iNumVertices; i++)
                {
                    fprintf(fp, "v %.4f %.4f %.4f\n",
                        aClusterTriangleVertices[i].mPosition.x,
                        aClusterTriangleVertices[i].mPosition.y,
                        aClusterTriangleVertices[i].mPosition.z);
                }

                for(uint32_t i = 0; i < iNumVertices; i++)
                {
                    fprintf(fp, "vt %.4f %.4f\n",
                        aClusterTriangleVertices[i].mUV.x,
                        aClusterTriangleVertices[i].mUV.y);
                }

                for(uint32_t i = 0; i < iNumVertices; i++)
                {
                    fprintf(fp, "vn %.4f %.4f %.4f\n",
                        aClusterTriangleVertices[i].mNormal.x,
                        aClusterTriangleVertices[i].mNormal.y,
                        aClusterTriangleVertices[i].mNormal.z);
                }

                for(uint32_t iTri = 0; iTri < iNumIndices; iTri += 3)
                {
                    fprintf(fp, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
                        aiClusterTriangleIndices[iTri] + 1 + iNumTotalVertices,
                        aiClusterTriangleIndices[iTri] + 1 + iNumTotalVertices,
                        aiClusterTriangleIndices[iTri] + 1 + iNumTotalVertices,

                        aiClusterTriangleIndices[iTri + 1] + 1 + iNumTotalVertices,
                        aiClusterTriangleIndices[iTri + 1] + 1 + iNumTotalVertices,
                        aiClusterTriangleIndices[iTri + 1] + 1 + iNumTotalVertices,

                        aiClusterTriangleIndices[iTri + 2] + 1 + iNumTotalVertices,
                        aiClusterTriangleIndices[iTri + 2] + 1 + iNumTotalVertices,
                        aiClusterTriangleIndices[iTri + 2] + 1 + iNumTotalVertices);
                }

                iNumTotalVertices += iNumVertices;
                iNumTotalIndices += iNumIndices;

                float fRand0 = float(rand() % 255) / 255.0f;
                float fRand1 = float(rand() % 255) / 255.0f;
                float fRand2 = float(rand() % 255) / 255.0f;

                FILE* pMaterialFile = fopen(clusterMaterialOutputPath.str().c_str(), "wb");
                fprintf(pMaterialFile, "newmtl %s\n", meshName.str().c_str());
                fprintf(pMaterialFile, "Kd %.4f %.4f %.4f\n",
                    fRand0,
                    fRand1,
                    fRand2);
                fclose(pMaterialFile);
            }

            fclose(fp);
            
        }

        /*
        **
        */
        void CMeshClusterManager::testIndirectDrawCommands()
        {
            std::vector<uint8_t> acIndirectDrawCommands(1 << 16);
            uint32_t* pDataInt = reinterpret_cast<uint32_t*>(acIndirectDrawCommands.data());
            *pDataInt++ = 10;
            for(uint32_t iCluster = 0; iCluster < 50; iCluster++)
            {
                uint64_t* pDataInt64 = reinterpret_cast<uint64_t*>(pDataInt);
                *pDataInt64++ = mpTotalClusterConstantBuffer->getGPUVirtualAddress();

                auto const& clusterInfo = maUpdatingClusterInfo[iCluster];
                float* pDataFloat = reinterpret_cast<float*>(pDataInt64);
                *pDataFloat++ = 1.0f;
                *pDataFloat++ = 0.0f;
                *pDataFloat++ = 0.0f;
                *pDataFloat++ = 1.0f;

                pDataInt64 = reinterpret_cast<uint64_t*>(pDataFloat);
                *pDataInt64++ = mpTotalClusterVertexDataBuffer->getGPUVirtualAddress() + clusterInfo.miVertexBufferOffset;
                pDataInt = reinterpret_cast<uint32_t*>(pDataInt64);
                *pDataInt++ = clusterInfo.miVertexBufferSize;
                *pDataInt++ = sizeof(ConvertedMeshVertexFormat);

                pDataInt64 = reinterpret_cast<uint64_t*>(pDataInt);
                //*pDataInt64++ = mpTotalClusterIndexDataBuffer->getGPUVirtualAddress() + clusterInfo.miIndexBufferOffset;
                pDataInt = reinterpret_cast<uint32_t*>(pDataInt64);
                //*pDataInt++ = clusterInfo.miIndexBufferSize;
                *pDataInt++ = 42;

                //*pDataInt++ = clusterInfo.miIndexBufferSize / sizeof(uint32_t);
                *pDataInt++ = 1;
                *pDataInt++ = 0;
                *pDataInt++ = 0;
                *pDataInt++ = 0;

                *pDataInt++ = 0;
                *pDataInt++ = 0;
                *pDataInt++ = 0;
            }

            //mpRenderer->copyCPUToBuffer(
            //    mpIndirectDrawClusterBuffer,
            //    acIndirectDrawCommands.data(),
            //    0,
            //    1 << 16);
        }

        /*
        **
        */
        void CMeshClusterManager::comparePIXDrawClusterFile(
            std::string const& filePath0,
            std::string const& filePath1)
        {
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

            FILE* fp0 = fopen(filePath0.c_str(), "rb");
            fseek(fp0, 0, SEEK_END);
            uint64_t iFileSize0 = ftell(fp0);
            fseek(fp0, 0, SEEK_SET);
            std::vector<uint8_t> acFileContent0(iFileSize0);
            fread(acFileContent0.data(), sizeof(char), iFileSize0, fp0);
            fclose(fp0);

            std::vector<DrawClusterInfo> aIndirectCommands0;
            {
                uint32_t const* pDataInt = reinterpret_cast<uint32_t*>(acFileContent0.data());
                uint32_t iNumDrawCommands = *pDataInt++;
                aIndirectCommands0.resize(iNumDrawCommands);
                DrawClusterInfo const* pDrawCommand = reinterpret_cast<DrawClusterInfo const*>(pDataInt);
                for(uint32_t i = 0; i < iNumDrawCommands; i++)
                {
                    aIndirectCommands0[i] = *pDrawCommand;
                    ++pDrawCommand;
                }
            }

            FILE* fp1 = fopen(filePath1.c_str(), "rb");
            fseek(fp1, 0, SEEK_END);
            uint64_t iFileSize1 = ftell(fp1);
            fseek(fp1, 0, SEEK_SET);
            std::vector<uint8_t> acFileContent1(iFileSize1);
            fread(acFileContent1.data(), sizeof(char), iFileSize1, fp1);
            fclose(fp1);

            std::vector<DrawClusterInfo> aIndirectCommands1;
            {
                uint32_t const* pDataInt = reinterpret_cast<uint32_t*>(acFileContent1.data());
                uint32_t iNumDrawCommands = *pDataInt++;
                aIndirectCommands1.resize(iNumDrawCommands);
                DrawClusterInfo const* pDrawCommand = reinterpret_cast<DrawClusterInfo const*>(pDataInt);
                for(uint32_t i = 0; i < iNumDrawCommands; i++)
                {
                    aIndirectCommands1[i] = *pDrawCommand;
                    ++pDrawCommand;
                }
            }

            uint32_t iCount = 0;
            if(aIndirectCommands0.size() > aIndirectCommands1.size())
            {
                for(auto const& command : aIndirectCommands1)
                {
                    auto iter = std::find_if(
                        aIndirectCommands0.begin(),
                        aIndirectCommands0.end(),
                        [command](DrawClusterInfo const& info)
                        {
                            return info.miCluster == command.miCluster;
                        }
                    );

                    if(iter == aIndirectCommands0.end())
                    {
                        DEBUG_PRINTF("!!! %d can\'t find cluster %d from list 0 (%lld) to list 1 (%lld)\n",
                            iCount,
                            command.miCluster,
                            aIndirectCommands0.size(),
                            aIndirectCommands1.size());
                        ++iCount;
                    }
                    else
                    {
                        if(iter->miVisibilityBits != command.miVisibilityBits)
                        {
                            DEBUG_PRINTF("*** visibility bits for cluster %d don\'t match %d to %d\n",
                                command.miCluster,
                                command.miVisibilityBits,
                                iter->miVisibilityBits);
                        }
                    }
                }
            }
            else
            {
                for(auto const& command : aIndirectCommands0)
                {
                    auto iter = std::find_if(
                        aIndirectCommands1.begin(),
                        aIndirectCommands1.end(),
                        [command](DrawClusterInfo const& info)
                        {
                            return info.miCluster == command.miCluster;
                        }
                    );

                    if(iter == aIndirectCommands1.end())
                    {
                        DEBUG_PRINTF("!!! %d can\'t find cluster %d from list 1 (%lld) to list 0 (%lld)\n",
                            iCount,
                            command.miCluster,
                            aIndirectCommands1.size(),
                            aIndirectCommands0.size());
                        ++iCount;
                    }
                    else
                    {
                        if(iter->miVisibilityBits != command.miVisibilityBits)
                        {
                            DEBUG_PRINTF("*** visibility bits for cluster %d don\'t match %d to %d\n",
                                command.miCluster,
                                command.miVisibilityBits,
                                iter->miVisibilityBits);
                        }
                    }
                }
            }

            int iDebug = 1;
        }

#define MAX_TRAVERSE_THREADS     16

        static uint32_t saiStack[MAX_TRAVERSE_THREADS][256];
        static int32_t saiStackTop[MAX_TRAVERSE_THREADS];
        static int32_t saiNumInStack[MAX_TRAVERSE_THREADS];
        static std::vector<OctNodeMeshInfo> saMeshInstanceInView;
        static std::mutex sTraverseMutex;

        bool isOctNodeVisible(OctNode const& octNode)
        {
            return true;
        }

        bool hasAddedMeshToList(OctNodeMeshInfo const& meshInstanceInfo)
        {
            auto iter = std::find_if(
                saMeshInstanceInView.begin(),
                saMeshInstanceInView.end(),
                [meshInstanceInfo](auto const& checkMeshInstanceInfo)
                {
                    return meshInstanceInfo.miMeshInstance == checkMeshInstanceInfo.miMeshInstance;
                }
            );

            return (iter != saMeshInstanceInView.end());
        }

        void addToMeshList(
            OctNodeMeshInfo const& meshInstanceInfo)
        {
            saMeshInstanceInView.push_back(meshInstanceInfo);
        }

        /*
        **
        */
        void CMeshClusterManager::traverseOctTree(
            std::vector<MeshInstanceInfo>& aMeshInstances,
            std::vector<OctNode> const& aOctNodes,
            uint32_t iStartingNode,
            OctTree const& octTree,
            uint32_t iThreadID)
        {
            saiStackTop[iThreadID] = 0;
            saiNumInStack[iThreadID] = 1;
            saiStack[iThreadID][0] = iStartingNode;

            uint32_t iNumInStack = saiNumInStack[iThreadID];
            for(uint32_t i = 0; i < 1000; i++)
            {
                if(saiNumInStack[iThreadID] <= 0)
                {
                    break;
                }

                int32_t iStackTop = saiStackTop[iThreadID];
                uint32_t iOctNodeID = saiStack[iThreadID][iStackTop];

                saiStackTop[iThreadID] -= 1;
                saiNumInStack[iThreadID] -= 1;
                
                OctNode const& octNode = aOctNodes[iOctNodeID];

                {
                    std::lock_guard<std::mutex> lock(sTraverseMutex);
                    DEBUG_PRINTF("thread %d visit node %d (%.4f, %.4f, %.4f) size %.4f\n",
                        iThreadID,
                        iOctNodeID,
                        octNode.mCenter.x,
                        octNode.mCenter.y,
                        octNode.mCenter.z,
                        octNode.mMaxBounds.x - octNode.mMinBounds.x);
                }

                if(!isOctNodeVisible(octNode))
                {
                    break;
                }

                for(uint32_t iMeshInstance = 0; iMeshInstance < octNode.miNumEmbeddedMeshInstances; iMeshInstance++)
                {
                    if(!hasAddedMeshToList(octNode.maMeshInstanceInfo[iMeshInstance]))
                    {
                        addToMeshList(octNode.maMeshInstanceInfo[iMeshInstance]);
                    }
                }

                for(uint32_t iChild = 0; iChild < 8; iChild++)
                {
                    if(octNode.maiChildren[iChild] != UINT32_MAX)
                    {
                        saiStackTop[iThreadID] += 1;
                        saiNumInStack[iThreadID] += 1;

                        iNumInStack = saiNumInStack[iThreadID];
                        uint32_t iStackTop = saiStackTop[iThreadID];
                        auto const& childNode = aOctNodes[octNode.maiChildren[iChild]];
                        saiStack[iThreadID][iStackTop] = childNode.miID;
                    }
                }
            }
        }

        /*
        **
        */
        void CMeshClusterManager::traverseOctTreeMT(
            std::vector<MeshInstanceInfo>& aMeshInstances,
            std::vector<OctNode> const& aOctNodes,
            OctTree const& octTree)
        {
            uint32_t const kiNumThreads = 8;
            std::vector<std::unique_ptr<std::thread>> apThreads(kiNumThreads);

            uint32_t iStartingNode = 0;
            for(uint32_t iThread = 0; iThread < kiNumThreads; iThread++)
            {
                for(;iStartingNode < static_cast<uint32_t>(aOctNodes.size()); iStartingNode++)
                {
                    if((aOctNodes[iStartingNode].mMaxBounds.x - aOctNodes[iStartingNode].mMinBounds.x) == 10.0f)
                    {
                        break;
                    }
                }

                apThreads[iThread] = std::make_unique<std::thread>(
                    [&aMeshInstances,
                     aOctNodes,
                     octTree,
                     iStartingNode,
                     iThread]()
                    {
                        CMeshClusterManager::traverseOctTree(
                            aMeshInstances,
                            aOctNodes,
                            iStartingNode,
                            octTree,
                            iThread);
                    });

                ++iStartingNode;
            }

            for(uint32_t iThread = 0; iThread < kiNumThreads; iThread++)
            {
                if(apThreads[iThread]->joinable())
                {
                    apThreads[iThread]->join();
                }
            }
        }

        /*
        **
        */
        void CMeshClusterManager::buildOctTree(
            OctTree& octTree,
            std::vector<OctNode>& aOctNodes,
            uint32_t iParentNodeID,
            std::vector<MeshInstanceInfo> const& aMeshInstances,
            std::vector<MeshInfo> const& aMeshes,
            float3 const& minBounds,
            float3 const& maxBounds,
            uint32_t iLevel)
        {
            auto insideBounds = [](
                float3 const& minBounds0,
                float3 const& maxBounds0,
                float3 const& minBounds1,
                float3 const& maxBounds1)
            {
                bool bRet = (
                    minBounds0.x <= maxBounds1.x &&
                    maxBounds0.x >= minBounds1.x &&
                    minBounds0.y <= maxBounds1.y &&
                    maxBounds0.y >= minBounds1.y &&
                    minBounds0.z <= maxBounds1.z &&
                    maxBounds0.z >= minBounds1.z);

                return bRet;
            };

            float3 halfBoundSize = (maxBounds - minBounds) * 0.5f;
            if(lengthSquared(halfBoundSize) <= 1.0f)
            {
                return;
            }

            float3 center = minBounds + halfBoundSize;
            float fDimensionSize = maxBounds.x - minBounds.x;
            float fHalfDimensionSize = fDimensionSize * 0.5f;
            {
                PrintOptions opt = { false };
                
                float const kfSphereSize = 0.1f;

                DEBUG_PRINTF_SET_OPTIONS(opt);
                
                std::ostringstream collectionName;
                collectionName << "oct@";
                collectionName << int32_t(center.x) << "@";
                collectionName << int32_t(center.y) << "@";
                collectionName << int32_t(center.z) << "@";
                collectionName << int32_t(fDimensionSize);

                DEBUG_PRINTF("create_collection(\'%s\')\n", collectionName.str().c_str());
                
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 0, 0, \'%s\')\n",
                    center.x,
                    center.y,
                    center.z,
                    kfSphereSize,
                    collectionName.str().c_str());

                // top
                float3 pt = center + float3(-fHalfDimensionSize, fHalfDimensionSize, -fHalfDimensionSize);
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 255, 0, \'%s\')\n",
                    pt.x,
                    pt.y,
                    pt.z,
                    kfSphereSize,
                    collectionName.str().c_str());
                
                pt = center + float3(-fHalfDimensionSize, fHalfDimensionSize, fHalfDimensionSize);
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 255, 0, \'%s\')\n",
                    pt.x,
                    pt.y,
                    pt.z,
                    kfSphereSize,
                    collectionName.str().c_str());

                pt = center + float3(fHalfDimensionSize, fHalfDimensionSize, -fHalfDimensionSize);
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 255, 0, \'%s\')\n",
                    pt.x,
                    pt.y,
                    pt.z,
                    kfSphereSize,
                    collectionName.str().c_str());

                pt = center + float3(fHalfDimensionSize, fHalfDimensionSize, fHalfDimensionSize);
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 255, 0, \'%s\')\n",
                    pt.x,
                    pt.y,
                    pt.z,
                    kfSphereSize,
                    collectionName.str().c_str());

                // bottom
                pt = center + float3(-fHalfDimensionSize, -fHalfDimensionSize, -fHalfDimensionSize);
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 255, 0, \'%s\')\n",
                    pt.x,
                    pt.y,
                    pt.z,
                    kfSphereSize,
                    collectionName.str().c_str());

                pt = center + float3(-fHalfDimensionSize, -fHalfDimensionSize, fHalfDimensionSize);
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 255, 0, \'%s\')\n",
                    pt.x,
                    pt.y,
                    pt.z,
                    kfSphereSize,
                    collectionName.str().c_str());

                pt = center + float3(fHalfDimensionSize, -fHalfDimensionSize, -fHalfDimensionSize);
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 255, 0, \'%s\')\n",
                    pt.x,
                    pt.y,
                    pt.z,
                    kfSphereSize,
                    collectionName.str().c_str());

                pt = center + float3(fHalfDimensionSize, -fHalfDimensionSize, fHalfDimensionSize);
                DEBUG_PRINTF("create_sphere_in_collection([%.4f, %.4f, %.4f], %.4f, 255, 255, 0, \'%s\')\n",
                    pt.x,
                    pt.y,
                    pt.z,
                    kfSphereSize,
                    collectionName.str().c_str());
 
                DEBUG_PRINTF("create_box_in_collection([%.4f, %.4f, %.4f], %.4f, random.randint(0, 255), random.randint(0, 255), random.randint(0, 255), 80, \'%s\')\n",
                    center.x,
                    center.y,
                    center.z,
                    fDimensionSize,
                    collectionName.str().c_str());

                opt.mbDisplayTime = true;
                DEBUG_PRINTF_SET_OPTIONS(opt);
            }

            OctNode node;
            node.mCenter = (maxBounds + minBounds) * 0.5f;
            node.mMaxBounds = maxBounds;
            node.mMinBounds = minBounds;
            float fNodeSize = length(maxBounds - minBounds);
            bool bHasMeshInstance = false;
            for(uint32_t iMeshInstance = 0; iMeshInstance < static_cast<uint32_t>(aMeshInstances.size()); iMeshInstance++)
            {
                auto const& meshInstance = aMeshInstances[iMeshInstance];
                auto const& mesh = aMeshes[meshInstance.miMesh];

                // transform min and max bounds of mesh instance
                float4 xformMeshMaxBounds = mul(
                    float4(mesh.mMaxBounds.x, mesh.mMaxBounds.y, mesh.mMaxBounds.z, 1.0f),
                    meshInstance.mTransformMatrix);
               
                float4 xformMeshMinBounds = mul(
                    float4(mesh.mMinBounds.x, mesh.mMinBounds.y, mesh.mMinBounds.z, 1.0f),
                    meshInstance.mTransformMatrix);

                float fMeshSize = length(xformMeshMinBounds - xformMeshMaxBounds);
                float3 meshMinBounds = fminf(mesh.mMinBounds, mesh.mMaxBounds);
                float3 meshMaxBounds = fmaxf(mesh.mMinBounds, mesh.mMaxBounds);

                bool bInsideNode = insideBounds(
                    meshMinBounds,
                    meshMaxBounds,
                    minBounds,
                    maxBounds);

                if(bInsideNode)
                {
                    bHasMeshInstance = true;
                }

                // mesh instance fits inside node 
                if(fMeshSize <= fNodeSize &&
                   fMeshSize >= fNodeSize * 0.5f && 
                   bInsideNode)
                {
                    OctNodeMeshInfo octNodeMeshInstanceInfo;
                    octNodeMeshInstanceInfo.miMesh = meshInstance.miMesh;
                    octNodeMeshInstanceInfo.miMeshInstance = meshInstance.miInstance;
                    node.maMeshInstanceInfo[node.miNumEmbeddedMeshInstances] = octNodeMeshInstanceInfo;
                    ++node.miNumEmbeddedMeshInstances;
                }
                else if(bInsideNode)
                {
                    ++node.miNumEmbeddedMeshInstances;
                }
            }

            if(!bHasMeshInstance)
            {
                return;
            }

            // add valid node intersecting one or more mesh instances
            node.miID = static_cast<uint32_t>(aOctNodes.size());
            node.miParent = iParentNodeID;
            aOctNodes.push_back(node);
            if(iParentNodeID != UINT32_MAX)
            {
                OctNode* pParentNode = &aOctNodes[iParentNodeID];
                WTFASSERT(pParentNode->miNumChildren < 8, "number of children out of bounds");
                pParentNode->maiChildren[pParentNode->miNumChildren] = node.miID;
                ++pParentNode->miNumChildren;
            }

            float fQuarterDimensionSize = fDimensionSize * 0.25f;

            float3 topLeftCenterFront = center + float3(-fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize);
            float3 topRightCenterFront = center + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize);
            float3 topLeftCenterBack = center + float3(-fQuarterDimensionSize, fQuarterDimensionSize, -fQuarterDimensionSize);
            float3 topRightCenterBack = center + float3(fQuarterDimensionSize, fQuarterDimensionSize, -fQuarterDimensionSize);

            float3 bottomLeftCenterFront = center + float3(-fQuarterDimensionSize, -fQuarterDimensionSize, fQuarterDimensionSize);
            float3 bottomRightCenterFront = center + float3(fQuarterDimensionSize, -fQuarterDimensionSize, fQuarterDimensionSize);
            float3 bottomLeftCenterBack = center + float3(-fQuarterDimensionSize, -fQuarterDimensionSize, -fQuarterDimensionSize);
            float3 bottomRightCenterBack = center + float3(fQuarterDimensionSize, -fQuarterDimensionSize, -fQuarterDimensionSize);

            float3 aMinBounds[8] =
            {
                topLeftCenterFront - float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                topRightCenterFront - float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                topLeftCenterBack - float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                topRightCenterBack - float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),

                bottomLeftCenterFront - float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                bottomRightCenterFront - float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                bottomLeftCenterBack - float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                bottomRightCenterBack - float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
            };

            float3 aMaxBounds[8] =
            {
                topLeftCenterFront + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                topRightCenterFront + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                topLeftCenterBack + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                topRightCenterBack + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),

                bottomLeftCenterFront + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                bottomRightCenterFront + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                bottomLeftCenterBack + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
                bottomRightCenterBack + float3(fQuarterDimensionSize, fQuarterDimensionSize, fQuarterDimensionSize),
            };

            for(uint32_t i = 0; i < 8; i++)
            {
                buildOctTree(
                    octTree,
                    aOctNodes,
                    node.miID,
                    aMeshInstances,
                    aMeshes,
                    aMinBounds[i],
                    aMaxBounds[i],
                    iLevel + 1);
            }
        }
        

    }   // Common

}   // Render