#pragma once

#include "vec.h"
#include <vector>
#include <string>


#define MAX_CLUSTERS_IN_GROUP       128
#define MAX_MIP_LEVELS              2
#define MAX_ASSOCIATED_GROUPS       2
#define MAX_PARENT_CLUSTERS         128

namespace Render
{
    namespace Common
    {
        /*
        **
        */
        struct MeshClusterGroup
        {
            uint32_t                            maiClusters[MAX_MIP_LEVELS][MAX_CLUSTERS_IN_GROUP];
            uint32_t                            maiNumClusters[MAX_MIP_LEVELS];
            uint32_t                            miNumMIPS = 0;
            uint32_t                            miIndex;

            float                               mafMinErrors[MAX_MIP_LEVELS] = { 0.0f, 0.0f };
            float                               mafMaxErrors[MAX_MIP_LEVELS] = { 0.0f, 0.0f };

            uint32_t                            miLODLevel = 0;
            float3                              mMinBounds = float3(0.0f, 0.0f, 0.0f);
            float3                              mMaxBounds = float3(0.0f, 0.0f, 0.0f);
            float3                              mMidPosition = float3(0.0f, 0.0f, 0.0f);
            float                               mfRadius = 0.0f;
            float                               mfPctError = 0.0f;

            float3                              mNormal = float3(0.0f, 0.0f, 0.0f);

            float3                              maMaxErrorPositions[MAX_MIP_LEVELS][2];

            float                               mfMinError;
            float                               mfMaxError;

        public:
            MeshClusterGroup() = default;

            MeshClusterGroup(
                std::vector<uint32_t> aiClusters,
                uint32_t iLODLevel,
                uint32_t iMIPLevel,
                uint32_t iIndex,
                uint32_t iClusterIndexOffset)
            {
                maiNumClusters[iMIPLevel] = static_cast<uint32_t>(aiClusters.size());
                miLODLevel = iLODLevel;
                miIndex = iIndex;

                miNumMIPS = (miNumMIPS < iMIPLevel + 1) ? iMIPLevel + 1 : miNumMIPS;

                for(uint32_t i = 0; i < static_cast<uint32_t>(aiClusters.size()); i++)
                {
                    maiClusters[iMIPLevel][i] = aiClusters[i] + iClusterIndexOffset;
                }
            }
        };

        /*
        **
        */
        struct MeshCluster
        {
            uint32_t                                    miClusterGroup;
            uint32_t                                    miLODLevel;
            uint32_t                                    miIndex;

            uint32_t                                    maiClusterGroups[MAX_ASSOCIATED_GROUPS];
            uint32_t                                    miNumClusterGroups = 0;

            uint32_t                                    maiParentClusters[MAX_PARENT_CLUSTERS];
            uint32_t                                    miNumParentClusters = 0;

            float                                       mfError = 0.0f;
            float3                                      mNormal = float3(0.0f, 0.0f, 0.0f);

            float                                       mfAverageDistanceFromLOD0 = 0.0f;

            float3                                      mMinBounds;
            float3                                      mMaxBounds;
            float3                                      mCenter;


            float3                                      mMaxErrorPosition0;
            float3                                      mMaxErrorPosition1;

            uint64_t                                    miVertexPositionStartArrayAddress;
            uint32_t                                    miNumVertexPositions;
            uint64_t                                    miVertexNormalStartArrayAddress;
            uint32_t                                    miNumVertexNormals;
            uint64_t                                    miVertexUVStartArrayAddress;
            uint32_t                                    miNumVertexUVs;
            uint64_t                                    miTrianglePositionIndexArrayAddress;
            uint32_t                                    miNumTrianglePositionIndices;
            uint64_t                                    miTriangleNormalIndexArrayAddress;
            uint32_t                                    miNumTriangleNormalIndices;
            uint64_t                                    miTriangleUVIndexArrayAddress;
            uint32_t                                    miNumTriangleUVIndices;

            float4                                      mNormalCone;

        public:
            MeshCluster() = default;

            MeshCluster(
                uint64_t iVertexPositionAddress,
                uint64_t iVertexNormalAddress,
                uint64_t iVertexUVAddress,
                uint64_t iTrianglePositionIndexArrayAddress,
                uint64_t iTriangleNormalIndexArrayAddress,
                uint64_t iTriangleUVIndexArrayAddress,
                uint32_t iNumVertexPositions,
                uint32_t iNumVertexNormals,
                uint32_t iNumVertexUVs,
                uint32_t iNumTriangleIndices,
                uint32_t iClusterGroup,
                uint32_t iLODLevel,
                uint32_t iIndex,
                uint32_t iMeshClusterGroupIndexOffset)
            {
                miVertexPositionStartArrayAddress = iVertexPositionAddress;
                miTrianglePositionIndexArrayAddress = iTrianglePositionIndexArrayAddress;

                miVertexNormalStartArrayAddress = iVertexNormalAddress;
                miTriangleNormalIndexArrayAddress = iTriangleNormalIndexArrayAddress;

                miVertexUVStartArrayAddress = iVertexUVAddress;
                miTriangleUVIndexArrayAddress = iTriangleUVIndexArrayAddress;

                miNumVertexPositions = iNumVertexPositions;
                miNumTrianglePositionIndices = iNumTriangleIndices;

                miNumVertexNormals = iNumVertexNormals;
                miNumTriangleNormalIndices = iNumTriangleIndices;

                miNumVertexUVs = iNumVertexUVs;
                miNumTriangleUVIndices = iNumTriangleIndices;

                miClusterGroup = iClusterGroup + iMeshClusterGroupIndexOffset;
                miLODLevel = iLODLevel;

                miIndex = iIndex;

                //maiClusterGroups[miNumClusterGroups] = iClusterGroup;
                //++miNumClusterGroups;
            }
        };

        void loadMeshClusters(
            std::vector<MeshCluster>& aMeshClusters,
            std::string const& filePath);

        void saveMeshClusterData(
            std::vector<uint8_t> const& aVertexPositionBuffer,
            std::vector<uint8_t> const& aVertexNormalBuffer,
            std::vector<uint8_t> const& aVertexUVBuffer,
            std::vector<uint8_t> const& aiTrianglePositionIndexBuffer,
            std::vector<uint8_t> const& aiTriangleNormalIndexBuffer,
            std::vector<uint8_t> const& aiTriangleUVIndexBuffer,
            std::vector<MeshCluster*> const& apMeshClusters,
            std::string const& outputFilePath);

        struct MeshClusterVertexFormat
        {
            float3          mPosition;
            float3          mNormal;
            float2          mUV;

            MeshClusterVertexFormat() = default;

            MeshClusterVertexFormat(float3 const& pos, float3 const& norm, float2 const& uv)
            {
                mPosition = pos; mNormal = norm; mUV = uv;
            }
        };

        struct ConvertedMeshVertexFormat
        {
            float4          mPosition;
            float4          mNormal;
            float4          mUV;

            ConvertedMeshVertexFormat() = default;

            ConvertedMeshVertexFormat(float3 const& pos, float3 const& norm, float2 const& uv)
            {
                mPosition = float4(pos, 1.0f);
                mNormal = float4(norm, 1.0f);
                mUV = float4(uv.x, uv.y, 0.0f, 0.0f);
            }

            ConvertedMeshVertexFormat(MeshClusterVertexFormat const& v)
            {
                mPosition = float4(v.mPosition, 1.0f);
                mNormal = float4(v.mNormal, 1.0f);
                mUV = float4(v.mUV.x, v.mUV.y, 0.0f, 0.0f);
            }
        };

        void loadMeshClusterTriangleData(
            std::string const& filePath,
            std::vector<std::vector<ConvertedMeshVertexFormat>>& aaVertices,
            std::vector<std::vector<uint32_t>>& aaiTriangleVertexIndices);

        void loadMeshClusterTriangleDataTableOfContent(
            std::vector<uint32_t>& aiNumClusterVertices,
            std::vector<uint32_t>& aiNumClusterIndices,
            std::vector<uint64_t>& aiVertexBufferArrayOffsets,
            std::vector<uint64_t>& aiIndexBufferArrayOffset,
            std::string const& vertexDataFilePath,
            std::string const& indexDataFilePath);

        void loadMeshClusterTriangleDataChunk(
            std::vector<ConvertedMeshVertexFormat>& aClusterTriangleVertices,
            std::vector<uint32_t>& aiClusterTriangleVertexIndices,
            uint32_t& iVertexDataBufferSize,
            uint32_t& iIndexDataBufferSize,
            std::string const& vertexDataFilePath,
            std::string const& indexDataFilePath,
            std::vector<uint32_t> const& aiNumClusterVertices,
            std::vector<uint32_t> const& aiNumClusterIndices,
            std::vector<uint64_t> const& aiVertexBufferArrayOffsets,
            std::vector<uint64_t> const& aiIndexBufferArrayOffsets,
            uint32_t iClusterIndex);

    }   // Common

}   // Render