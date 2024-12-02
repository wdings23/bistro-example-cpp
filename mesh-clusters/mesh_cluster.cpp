#include "mesh_cluster.h"
#include <algorithm>
#include <assert.h>
#include "wtfassert.h"

namespace Render
{
    namespace Common
    {
        /*
        **
        */
        void saveMeshClusters(
            std::string const& outputFilePath,
            std::vector<MeshCluster*> const& apMeshClusters)
        {
            std::vector<uint8_t> acFileContent(apMeshClusters.size() * sizeof(MeshCluster));
            uint32_t iCurrFileContentSize = 0;
            for(auto const* pCluster : apMeshClusters)
            {
                uint8_t* pFileContent = acFileContent.data() + iCurrFileContentSize;
                memcpy(pFileContent, pCluster, sizeof(MeshCluster));
                iCurrFileContentSize += static_cast<uint32_t>(sizeof(MeshCluster));
            }

            FILE* fp = fopen(outputFilePath.c_str(), "wb");
            fwrite(acFileContent.data(), sizeof(char), iCurrFileContentSize, fp);
            fclose(fp);
        }

        /*
        **
        */
        void loadMeshClusters(
            std::vector<MeshCluster>& aMeshClusters,
            std::string const& filePath)
        {
            FILE* fp = fopen(filePath.c_str(), "rb");
            fseek(fp, 0, SEEK_END);
            uint64_t iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<uint8_t> acFileContent(iFileSize);
            fread(acFileContent.data(), iFileSize, sizeof(char), fp);
            fclose(fp);

            uint64_t iNumClusters = iFileSize / sizeof(MeshCluster);
            aMeshClusters.resize(iNumClusters);
            memcpy(aMeshClusters.data(), acFileContent.data(), sizeof(MeshCluster) * aMeshClusters.size());
        }

        /*
        **
        */
        void loadMeshClusterTriangleData(
            std::string const& filePath,
            std::vector<std::vector<ConvertedMeshVertexFormat>>& aaVertices,
            std::vector<std::vector<uint32_t>>& aaiTriangleVertexIndices)
        {
            FILE* fp = fopen(filePath.c_str(), "rb");
            fseek(fp, 0, SEEK_END);
            uint64_t iFileSize = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            std::vector<uint8_t> acFileContent(iFileSize);
            fread(acFileContent.data(), sizeof(char), iFileSize, fp);
            fclose(fp);

            uint32_t* piData = reinterpret_cast<uint32_t*>(acFileContent.data());
            uint32_t iNumClusterVertexList = *piData++;
            uint32_t iNumClusterVertexIndexList = *piData++;
            uint64_t iStartAddress = reinterpret_cast<uint64_t>(acFileContent.data());

            aaVertices.resize(iNumClusterVertexList);
            aaiTriangleVertexIndices.resize(iNumClusterVertexIndexList);

            std::vector<uint32_t> aiDataVertexSize(iNumClusterVertexList);
            for(uint32_t i = 0; i < iNumClusterVertexList; i++)
            {
                uint32_t iNumVertices = *piData;
                aaVertices[i].resize(iNumVertices);

                uint32_t iNumIndices = *(piData + 1);
                aaiTriangleVertexIndices[i].resize(iNumIndices);

                piData += 2;
            }

            ConvertedMeshVertexFormat* pVertexFormat = reinterpret_cast<ConvertedMeshVertexFormat*>(piData);
            uint64_t iCurrDataOffset = reinterpret_cast<uint64_t>(pVertexFormat) - iStartAddress;
            for(uint32_t i = 0; i < iNumClusterVertexList; i++)
            {
                memcpy(
                    aaVertices[i].data(),
                    pVertexFormat,
                    aaVertices[i].size() * sizeof(ConvertedMeshVertexFormat));
                pVertexFormat += aaVertices[i].size();
            }

            iCurrDataOffset = reinterpret_cast<uint64_t>(pVertexFormat) - iStartAddress;

            uint32_t* piVertexIndices = reinterpret_cast<uint32_t*>(pVertexFormat);
            for(uint32_t i = 0; i < iNumClusterVertexList; i++)
            {
                memcpy(
                    aaiTriangleVertexIndices[i].data(),
                    piVertexIndices,
                    aaiTriangleVertexIndices[i].size() * sizeof(uint32_t));
                piVertexIndices += aaiTriangleVertexIndices[i].size();
            }
        }

        /*
        **
        */
        void loadMeshClusterTriangleDataTableOfContent(
            std::vector<uint32_t>& aiNumClusterVertices,
            std::vector<uint32_t>& aiNumClusterIndices,
            std::vector<uint64_t>& aiVertexBufferArrayOffsets,
            std::vector<uint64_t>& aiIndexBufferArrayOffset,
            std::string const& vertexDataFilePath,
            std::string const& indexDataFilePath)
        {
            {
                std::vector<uint8_t> acContentBuffer(1 << 20);
                FILE* fp = fopen(vertexDataFilePath.c_str(), "rb");
                fread(acContentBuffer.data(), sizeof(char), sizeof(uint32_t), fp);
                uint32_t* piIntData = reinterpret_cast<uint32_t*>(acContentBuffer.data());
                uint32_t iNumClusters = *piIntData++;
                acContentBuffer.resize(sizeof(uint32_t) * iNumClusters * sizeof(uint32_t));
                piIntData = reinterpret_cast<uint32_t*>(acContentBuffer.data() + sizeof(uint32_t));
                fread(piIntData, sizeof(char), sizeof(uint32_t) * iNumClusters, fp);
                fclose(fp);

                aiNumClusterVertices.resize(iNumClusters);
                aiVertexBufferArrayOffsets.resize(iNumClusters);
                uint32_t iVertexArrayOffset = 0;
                for(uint32_t i = 0; i < iNumClusters; i++)
                {
                    aiNumClusterVertices[i] = *piIntData++;
                    aiVertexBufferArrayOffsets[i] = iVertexArrayOffset;
                    iVertexArrayOffset += aiNumClusterVertices[i];
                }
            }

            {
                std::vector<uint8_t> acContentBuffer(1 << 20);
                FILE* fp = fopen(indexDataFilePath.c_str(), "rb");
                fread(acContentBuffer.data(), sizeof(char), sizeof(uint32_t), fp);
                uint32_t* piIntData = reinterpret_cast<uint32_t*>(acContentBuffer.data());
                uint32_t iNumClusters = *piIntData++;
                acContentBuffer.resize(sizeof(uint32_t) * iNumClusters * sizeof(uint32_t));
                piIntData = reinterpret_cast<uint32_t*>(acContentBuffer.data() + sizeof(uint32_t));
                fread(piIntData, sizeof(char), sizeof(uint32_t) * iNumClusters, fp);
                fclose(fp);

                aiNumClusterIndices.resize(iNumClusters);
                aiIndexBufferArrayOffset.resize(iNumClusters);
                uint32_t iIndexArrayOffset = 0;
                for(uint32_t i = 0; i < iNumClusters; i++)
                {
                    aiNumClusterIndices[i] = *piIntData++;
                    aiIndexBufferArrayOffset[i] = iIndexArrayOffset;
                    iIndexArrayOffset += aiNumClusterIndices[i];
                }

            }
        }

        /*
        **
        */
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
            uint32_t iClusterIndex)
        {
            assert(iClusterIndex < aiVertexBufferArrayOffsets.size());

            FILE* fp = fopen(vertexDataFilePath.c_str(), "rb");
            iVertexDataBufferSize = static_cast<uint32_t>(aiNumClusterVertices[iClusterIndex] * sizeof(ConvertedMeshVertexFormat));
            assert(iVertexDataBufferSize <= aClusterTriangleVertices.size() * sizeof(ConvertedMeshVertexFormat));
            uint64_t iStartVertexDataOffset = sizeof(uint32_t) + sizeof(uint32_t) * aiNumClusterVertices.size() + aiVertexBufferArrayOffsets[iClusterIndex] * sizeof(ConvertedMeshVertexFormat);
            fseek(fp, static_cast<long>(iStartVertexDataOffset), SEEK_SET);
            fread(aClusterTriangleVertices.data(), sizeof(char), iVertexDataBufferSize, fp);
            fclose(fp);

            fp = fopen(indexDataFilePath.c_str(), "rb");
            assert(aiNumClusterIndices[iClusterIndex] <= 384);
            iIndexDataBufferSize = static_cast<uint32_t>(aiNumClusterIndices[iClusterIndex] * sizeof(uint32_t));
            assert(iIndexDataBufferSize <= static_cast<uint32_t>(aiNumClusterIndices[iClusterIndex] * sizeof(uint32_t)));
            assert(iIndexDataBufferSize <= aiClusterTriangleVertexIndices.size() * sizeof(uint32_t));
            uint64_t iStartIndexDataOffset = sizeof(uint32_t) + sizeof(uint32_t) * aiNumClusterIndices.size() + aiIndexBufferArrayOffsets[iClusterIndex] * sizeof(uint32_t);
            fseek(fp, static_cast<long>(iStartIndexDataOffset), SEEK_SET);
            fread(aiClusterTriangleVertexIndices.data(), sizeof(char), iIndexDataBufferSize, fp);
            fclose(fp);
        }

    }   // Common

}   // Render