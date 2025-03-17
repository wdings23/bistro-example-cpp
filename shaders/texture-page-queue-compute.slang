#define UINT32_MAX          0xffffffffu
#define FLT_MAX             1.0e+10
#define PI: f32             3.14159f
#define PROBE_IMAGE_SIZE    8
#define TEXTURE_PAGE_SIZE   64

#include "default-uniform-headers.slang"

struct TexturePage
{
    int32_t miPageUV;
    int32_t miTextureID;
    int32_t miHashIndex;

    int32_t miMIP;
};

struct HashEntry
{
    uint32_t miPageCoordinate;
    uint32_t miPageIndex;
    uint32_t miTextureIDAndMIP;
    uint32_t miUpdateFrame;
};

struct Material
{
    float4 mDiffuse;
    float4 mSpecular;
    float4 mEmissive;

    uint32_t miID;
    uint32_t miAlbedoTextureID;
    uint32_t miNormalTextureID;
    uint32_t miEmissiveTextureID;
};

struct ConstantBufferData
{
    uint32_t        miNumDispatchX;
    uint32_t        miNumDispatchY;
    uint32_t        miNumDispatchZ;
};

[[vk::binding(0, 0)]] RWStructuredBuffer<TexturePage> aTexturePageQueueMIP;
[[vk::binding(1, 0)]] RWStructuredBuffer<int32_t> aiCounters;
[[vk::binding(2, 0)]] RWStructuredBuffer<uint32_t> aiUsedHashPages;
[[vk::binding(3, 0)]] RWStructuredBuffer<HashEntry> aPageHashTableMIP;
[[vk::binding(4, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(5, 0)]] Texture2D<float4> texCoordTexture;
[[vk::binding(6, 0)]] Texture2D<float4> motionDepthMeshTexture;
[[vk::binding(7, 0)]] RWTexture2D<float4> textureAtlas0;
[[vk::binding(8, 0)]] RWTexture2D<float4> textureAtlas1;
[[vk::binding(9, 0)]] RWTexture2D<float4> textureAtlas2;
[[vk::binding(10, 0)]] RWTexture2D<float4> textureAtlas3;
[[vk::binding(11, 0)]] RWTexture2D<float4> initialTextureAtlas;
[[vk::binding(12, 0)]] RWTexture2D<float4> texturePageInfoTexture;
[[vk::binding(13, 0)]] RWTexture2D<float4> normalTexturePageInfoTexture;
[[vk::binding(14, 0)]] SamplerState sampler;

[[vk::binding(0, 1)]] StructuredBuffer<int2> aTextureSizes;
[[vk::binding(1, 1)]] StructuredBuffer<uint32_t> aMeshMaterialID;
[[vk::binding(2, 1)]] StructuredBuffer<Material> aMeshMaterials;
[[vk::binding(3, 1)]] StructuredBuffer<int2> aNormalTextureSizes;
[[vk::binding(4, 1)]] ConstantBuffer<ConstantBufferData> constantBufferData;
[[vk::binding(5, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformData;

/*
**
*/
[shader("compute")]
[numthreads(16, 16, 1)]
void CSMain(
    uint3 localIndex: SV_GroupThreadID,
    uint3 workGroupIndex: SV_GroupID)
{
    uint32_t iNumThreadX = 16;
    uint32_t iNumThreadY = 16;
    uint32_t iNumLocalThreads = 256;

    // uint2 workGroupSize = uint2(constantBufferData.miNumDispatchX, constantBufferData.miNumDispatchY);
    uint2 workGroupSize = uint2(8, 1);

    float fFar = 100.0f;
    float fNear = 1.0f;
    int32_t iMaxNumCachePerFrame = 200;

    // total threads with workgroup and local workgroup
    uint32_t iNumTotalThreadX = iNumThreadX * workGroupSize.x;
    uint32_t iNumTotalThreadY = iNumThreadY * workGroupSize.y; 

    uint32_t iTileWidth = uint32_t(defaultUniformData.miScreenWidth) / iNumTotalThreadX;
    uint32_t iTileHeight = uint32_t(defaultUniformData.miScreenHeight) / iNumTotalThreadY;

    
    uint32_t iTotalThreadIndex = (localIndex.y * iNumThreadX + localIndex.x) + (workGroupIndex.y * workGroupSize.x + workGroupIndex.x) * 256;

    uint32_t iTotalTileX = localIndex.x * iTileWidth;
    uint32_t iTotalTileY = localIndex.y * iTileHeight;

    uint2 textureSize = uint2(0, 0);
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    processPixel(
        iTotalThreadIndex,
        textureSize,
        iNumThreadX,
        iNumThreadY,
        workGroupSize,
        fNear,
        fFar
    );
}

/////
uint32_t registerTexturePage(
    in int2 startPageUV,
    in uint32_t iHashIndex,
    in uint32_t iTextureID,
    in uint32_t iMIP)
{
    uint32_t iTextureAndMIP = (uint32_t(iMIP) << 24) | uint32_t(iTextureID);

    uint32_t iBefore = 0;
    InterlockedExchange(aiUsedHashPages[iHashIndex], iHashIndex, iBefore);
    if(iBefore <= 0)
    {
        // successful exchange, new entry

        int32_t iQueueIndex = aiCounters[0]; 
        InterlockedAdd(aiCounters[0], 1, iQueueIndex); 
        InterlockedAdd(aiCounters[iMIP + 1], 1);

        int32_t iSignedEncodedCoordinateMIP = int32_t(int32_t(startPageUV.x) | int32_t(int32_t(startPageUV.y) << 16));
        aTexturePageQueueMIP[iQueueIndex].miPageUV = iSignedEncodedCoordinateMIP;
        aTexturePageQueueMIP[iQueueIndex].miTextureID = int32_t(iTextureID);
        aTexturePageQueueMIP[iQueueIndex].miHashIndex = int32_t(iHashIndex);
        aTexturePageQueueMIP[iQueueIndex].miMIP = int32_t(iMIP);

        aPageHashTableMIP[iHashIndex].miPageCoordinate = uint32_t(uint32_t(startPageUV.x) | uint32_t(uint32_t(startPageUV.y) << 16u));
        aPageHashTableMIP[iHashIndex].miPageIndex = UINT32_MAX;
        aPageHashTableMIP[iHashIndex].miTextureIDAndMIP = iTextureAndMIP;
    }
    else
    {
        // already registered

        uint32_t iMIPEncodedCoordinate = uint32_t(uint32_t(startPageUV.x) | uint32_t(uint32_t(startPageUV.y) << 16u));
        if (aPageHashTableMIP[iHashIndex].miPageCoordinate != iMIPEncodedCoordinate ||
            aPageHashTableMIP[iHashIndex].miTextureIDAndMIP != iTextureAndMIP)
        {
            // entry in has doesn't match, look for a new one

            uint32_t iOrigHashIndex = uint32_t(iHashIndex);
            for(uint32_t i = 0u; i < 10000u; i++)
            {
                iHashIndex = (iHashIndex + 1u) % 80000u;

                // found an existing entry
                if (aPageHashTableMIP[iHashIndex].miPageCoordinate == iMIPEncodedCoordinate &&
                    aPageHashTableMIP[iHashIndex].miTextureIDAndMIP == iTextureAndMIP)
                {
                    break;
                }

                // register new entry if not found
                if (aPageHashTableMIP[iHashIndex].miPageCoordinate <= 0 &&
                    aPageHashTableMIP[iHashIndex].miPageIndex == 0 &&
                    aPageHashTableMIP[iHashIndex].miUpdateFrame == 0 &&
                    aPageHashTableMIP[iHashIndex].miTextureIDAndMIP == 0)
                {
                    uint32_t iBefore = 0;
                    InterlockedExchange(aiUsedHashPages[iHashIndex], iHashIndex, iBefore);
                    if(iBefore <= 0u)
                    {
                        // successful exchange, new entry

                        int32_t iQueueIndex = 0;
                        InterlockedAdd(aiCounters[0], 1, iQueueIndex); 
                        InterlockedAdd(aiCounters[iMIP + 1], 1);

                        int32_t iSignedEncodedCoordinateMIP = int32_t(int32_t(startPageUV.x) | int32_t(int32_t(startPageUV.y) << 16));
                        aTexturePageQueueMIP[iQueueIndex].miPageUV = iSignedEncodedCoordinateMIP;
                        aTexturePageQueueMIP[iQueueIndex].miTextureID = int32_t(iTextureID);
                        aTexturePageQueueMIP[iQueueIndex].miHashIndex = int32_t(iHashIndex);
                        aTexturePageQueueMIP[iQueueIndex].miMIP = int32_t(iMIP);

                        aPageHashTableMIP[iHashIndex].miPageCoordinate = uint32_t(uint32_t(startPageUV.x) | uint32_t(uint32_t(startPageUV.y) << 16u));
                        aPageHashTableMIP[iHashIndex].miPageIndex = UINT32_MAX;
                        aPageHashTableMIP[iHashIndex].miTextureIDAndMIP = iTextureAndMIP;

                        break;
                    }
                }
            }
        }
    }

    return iHashIndex;
}


/////
uint32_t murmurHash13(
    in uint3 src)
{
    uint3 srcCopy = src;
    uint32_t M = 0x5bd1e995u;
    uint32_t h = 1190494759u;
    srcCopy *= M; srcCopy.x ^= srcCopy.x >> 24u; srcCopy.y ^= srcCopy.y >> 24u; srcCopy.z ^= srcCopy.z >> 24u; srcCopy *= M;
    h *= M; h ^= srcCopy.x; h *= M; h ^= srcCopy.y; h *= M; h ^= srcCopy.z;
    h ^= h >> 13u; h *= M; h ^= h >> 15u;
    return h;
}

/////
uint32_t hash13(
    in float3 src,
    in uint32_t iNumSlots)
{
    uint3 srcU32 = uint3(
        asuint(src.x),
        asuint(src.y),
        asuint(src.z)
    );

    uint32_t h = uint32_t(murmurHash13(srcU32));
    float fValue = asfloat((h & 0x007ffffffu) | 0x3f800000u) - 1.0f;
    uint32_t iRet = clamp(uint32_t(fValue * uint32_t(iNumSlots - 1)), 0u, iNumSlots - 1);
    return iRet;
}

/*
**
*/
void processPixel(
    in uint32_t iTotalThreadIndex, 
    in uint2 textureSize, 
    in uint32_t iNumThreadX,
    in uint32_t iNumThreadY,
    in uint2 workGroupSize,
    in float fNear,
    in float fFar)
{
    uint32_t iNumTotalThreads = iNumThreadX * iNumThreadY * workGroupSize.x * workGroupSize.y;
    uint32_t iNumPixelsPerThread = uint32_t(ceil((textureSize.x * textureSize.y) / iNumTotalThreads));
    for(uint32_t iPixel = 0; iPixel < iNumPixelsPerThread; iPixel++)
    {
        uint32_t iPixelCoord = iTotalThreadIndex * iNumPixelsPerThread + iPixel;
        if(iPixelCoord >= textureSize.x * textureSize.y)
        {
            break;
        }

        uint2 sampleCoord = uint2(
            iPixelCoord % textureSize.x,
            iPixelCoord / textureSize.y
            );
        float2 uv = float2(
            float(sampleCoord.x) / float(textureSize.x),
            float(sampleCoord.y) / float(textureSize.y)
            );
        float4 worldPosition = worldPositionTexture[sampleCoord];
        float4 texCoord = texCoordTexture[sampleCoord];
        float4 motionDepthMesh = motionDepthMeshTexture[sampleCoord];

        if (texCoord.x < 0.0f)
        {
            texCoord.x = fract(abs(1.0f - texCoord.x));
        }
        else if (texCoord.x > 1.0f)
        {
            texCoord.x = fract(texCoord.x);
        }

        if (texCoord.y < 0.0f)
        {
            texCoord.y = fract(abs(1.0f - texCoord.y));
        }
        else if (texCoord.y > 1.0f)
        {
            texCoord.y = fract(texCoord.y);
        }

        // TODO: use depth for MIP level
        float fDepth = fract(motionDepthMesh.z);

        // linearize depth
        float fLinearDepth = (fNear * fFar) / (fFar - fDepth * (fFar - fNear));
        uint32_t iMIP = 0u;
        if (fLinearDepth >= 3.0f && fLinearDepth < 10.0f)
        {
            iMIP = 1u;
        }
        else if (fLinearDepth >= 10.0f && fLinearDepth < 40.0f)
        {
            iMIP = 2u;
        }
        else if (fLinearDepth >= 40.0f)
        {
            iMIP = 3u;
        }

// no mip for now
iMIP = 0u;

        // image coordinate to fetch
        uint32_t iMesh = uint32_t(motionDepthMesh.z - fDepth);
        uint32_t iMaterialID = aMeshMaterialID[iMesh] - 1u;
        uint32_t iAlbedoTextureID = aMeshMaterials[iMaterialID].miAlbedoTextureID;
        uint32_t iNormalTextureID = aMeshMaterials[iMaterialID].miNormalTextureID;

        if(length(aMeshMaterials[iMaterialID].mEmissive.xyz) >= 0.6f)
        {
            iAlbedoTextureID = 0;
        }

        float2 textureSize = float2(
            float(aTextureSizes[iMaterialID].x),
            float(aTextureSizes[iMaterialID].y)
        );
        float2 normalTextureSize = float2(
            float(aNormalTextureSizes[iMaterialID].x),
            float(aNormalTextureSizes[iMaterialID].y)
        );

        // mip dimension, division by power of 2
        float2 mipTextureSize = textureSize / pow(2.0f, float(iMIP));
        float2 mipNormalTextureSize = normalTextureSize / pow(2.0f, float(iMIP));

        // get the start uv in image space for the page
        float2 mipImageCoord = texCoord.xy * mipTextureSize;
        int2 mipStartPageUV = int2(
            int32_t(floor(mipImageCoord.x / float(TEXTURE_PAGE_SIZE))),
            int32_t(floor(mipImageCoord.y / float(TEXTURE_PAGE_SIZE)))
        );
        float2 mipNormalImageCoord = texCoord.xy * mipNormalTextureSize;
        int2 mipNormalStartPageUV = int2(
            int32_t(floor(mipNormalImageCoord.x / float(TEXTURE_PAGE_SIZE))),
            int32_t(floor(mipNormalImageCoord.y / float(TEXTURE_PAGE_SIZE)))
        );

        // hash index to check for existence
        uint32_t iAlbedoHashIndex = hash13(
            float3(
                float(mipStartPageUV.x),
                float(mipStartPageUV.y) * 32.0f,
                float(iAlbedoTextureID) + float(iMIP + 1) * 1024.0f 
            ),
            80000u
        );

        uint32_t iNormalHashIndex = hash13(
            float3(
                float(mipNormalStartPageUV.x),
                float(mipNormalStartPageUV.y) * 32.0f,
                float(iNormalTextureID + 65536) + float(iMIP + 1) * 1024.0f 
            ),
            80000u
        );

        iAlbedoHashIndex = registerTexturePage(
            mipStartPageUV,
            iAlbedoHashIndex,
            iAlbedoTextureID,
            iMIP
        );

        // output page info
        uint32_t iMIPEncodedCoordinate = uint32_t(uint32_t(mipStartPageUV.x) | uint32_t(uint32_t(mipStartPageUV.y) << 16u));
        texturePageInfoTexture[int2(int32_t(uv.x * float(defaultUniformData.miScreenWidth)), int32_t(uv.y * float(defaultUniformData.miScreenHeight)))] =
            float4(
                float(iMIPEncodedCoordinate),
                float(mipStartPageUV.y),
                float(iAlbedoTextureID)/* + float(iMIP) * 0.1f*/,
                float(iAlbedoHashIndex)
            );

        iNormalHashIndex = registerTexturePage(
            mipNormalStartPageUV,
            iNormalHashIndex,
            iNormalTextureID + 65536,
            iMIP
        );

        uint32_t iNormalMIPEncodedCoordinate = uint32_t(uint32_t(mipNormalStartPageUV.x) | uint32_t(uint32_t(mipNormalStartPageUV.y) << 16u));
        normalTexturePageInfoTexture[int2(int32_t(uv.x * float(defaultUniformData.miScreenWidth)), int32_t(uv.y * float(defaultUniformData.miScreenHeight)))] =
            float4(
                float(iNormalMIPEncodedCoordinate),
                float(mipNormalStartPageUV.y),
                float(iNormalTextureID) /* + float(iMIP) * 0.1f*/,
                float(iNormalHashIndex)
            );
    }
}