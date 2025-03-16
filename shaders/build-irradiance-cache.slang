#define UINT32_MAX 0xffffffff
#define FLT_MAX 1.0e+10
#define PI 3.14159f
#define PROBE_IMAGE_SIZE 8u

#include "default-uniform-headers.slang"
#include "octahedron-map-headers.slang"
#include "murmur-hash-headers.slang"
#include "random-headers.slang"
#include "ray-sampling-headers.slang"
#include "barycentric-headers.slang"
#include "irradiance-cache-headers.slang"

struct RayPayload
{
    float4 mOutput;
    float4 mBarycentricCoord;
};

struct IrradianceCacheQueueEntry
{
    float4 mPosition;
    float4 mNormal;
};
struct IntersectBVHResult
{
    float3 mHitPosition;
    float3 mHitNormal;
    uint32_t miHitTriangle;
    uint32_t miHitMesh;
    float3 mBarycentricCoordinate;
};

struct UpdateIrradianceCacheResult
{
    RandomResult mRandomResult;
};

struct VertexFormat
{
    float4 mPosition;
    float4 mTexCoord;
    float4 mNormal;
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

struct MeshRange
{
    uint32_t miStart;
    uint32_t miEnd;
};

[[vk::binding(0, 0)]]
RWStructuredBuffer<IrradianceCacheEntry> irradianceCache;

[[vk::binding(1, 0)]]
RWStructuredBuffer<IrradianceCacheQueueEntry> irradianceCacheQueue;

[[vk::binding(2, 0)]]
RWStructuredBuffer<int32_t> counterBuffer;

[[vk::binding(3, 0)]] 
RaytracingAccelerationStructure scene;

[[vk::binding(4, 0)]]
Texture2D<float4> skyTexture;

[[vk::binding(5, 0)]]
Texture2D<float4> sunLightTexture;

[[vk::binding(6, 0)]]
Texture2D<float4> worldPositionTexture;

[[vk::binding(7, 0)]]
Texture2D<float4> normalTexture;

[[vk::binding(8, 0)]]
Texture2D<float4> hitPositionTexture;

[[vk::binding(9, 0)]]
Texture2D<float4> hitNormalTexture;

[[vk::binding(10, 0)]]
RWTexture2D<float4> debugTexture0;



[[vk::binding(0, 1)]] StructuredBuffer<VertexFormat> vertexBuffer;
[[vk::binding(1, 1)]] StructuredBuffer<uint32_t> indexBuffer;
[[vk::binding(2, 1)]] StructuredBuffer<Material> materialData;
[[vk::binding(3, 1)]] StructuredBuffer<uint32_t> materialID;
[[vk::binding(4, 1)]] StructuredBuffer<MeshRange> meshTriangleRangeData;
[[vk::binding(5, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformData;

#define NUM_THREADS         16

struct AssembledVertex
{
    float4 position : POSITION;
    float4 texCoord : TEXCOORD;
};

struct VertexStageOutput
{
    float4 sv_position : SV_Position;
    float4 texCoord;
};

[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    output.sv_position = assembledVertex.position;
    output.texCoord = assembledVertex.texCoord;

    return output;
}

[shader("closesthit")]
void hitTriangle(
    inout RayPayload payload,
    in BuiltInTriangleIntersectionAttributes attr
)
{
    payload.mBarycentricCoord = float4(1.0f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y, 0.0f);

    float fT = RayTCurrent();
    float fPrimitiveIndex = float(PrimitiveIndex());
    float fGeometry = float(GeometryIndex());
    payload.mOutput = float4(fT, fPrimitiveIndex, fGeometry, 1.0f);
}

[shader("miss")]
void missShader(
    inout RayPayload payload
)
{
    payload.mBarycentricCoord = float4(0.0f, 0.0f, 0.0f, 0.0f);
    payload.mOutput = float4(0.0f, 0.0f, 0.0f, 0.0f);
}

[shader("raygeneration")]
void rayGen()
{
    int32_t iMaxNumCachePerFrame = 200;

    uint2 rayIndex = DispatchRaysIndex().xy;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    RandomResult randomResult = initRand(
        uint32_t(float(rayIndex.x + rayIndex.y) * 100.0f + float(rayIndex.y + rayIndex.x) * 200.0f) + uint32_t(defaultUniformData.mfRand0 * 100.0f),
        uint32_t(float(rayIndex.x + rayIndex.y) * 10.0f + float(rayIndex.x + rayIndex.y) * 20.0f) + uint32_t(defaultUniformData.mfRand1 * 100.0f),
        10u);

    int2 texCoord = int2(rayIndex.x, rayIndex.y);

    float4 worldPosition = worldPositionTexture[texCoord];
    float4 hitPosition = hitPositionTexture[texCoord];
    float3 normal = normalTexture[texCoord].xyz;
    float fRayLength = length(hitPosition.xyz - worldPosition.xyz);

    debugTexture0[texCoord] = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // skip on pixels hitting the sky
    if(fRayLength < 100.0f && length(hitPosition.xyz) > 0.0f)
    {
        // check if on screen
        float4 clipSpacePosition = mul(defaultUniformData.mViewProjectionMatrix, float4(hitPosition.xyz, 1.0f));
        clipSpacePosition.x /= clipSpacePosition.w;
        clipSpacePosition.y /= clipSpacePosition.w;
        clipSpacePosition.z /= clipSpacePosition.w;
        clipSpacePosition.x = clipSpacePosition.x * 0.5f + 0.5f;
        clipSpacePosition.y = 1.0f - (clipSpacePosition.y * 0.5f + 0.5f);
        clipSpacePosition.z = clipSpacePosition.z * 0.5f + 0.5f;

        int2 clipSpaceScreenCoord = int2(
            int32_t(clipSpacePosition.x * float(textureSize.x)),
            int32_t(clipSpacePosition.y * float(textureSize.y))
        );

        float4 hitWorldPosition = worldPositionTexture[clipSpaceScreenCoord];
        var hitPositionClipSpace = mul(defaultUniformData.mViewProjectionMatrix, float4(hitWorldPosition.xyz, 1.0f));
        hitPositionClipSpace.x /= hitPositionClipSpace.w;
        hitPositionClipSpace.y /= hitPositionClipSpace.w;
        hitPositionClipSpace.z /= hitPositionClipSpace.w;
        hitPositionClipSpace.x = hitPositionClipSpace.x * 0.5f + 0.5f;
        hitPositionClipSpace.y = 1.0f - (hitPositionClipSpace.y * 0.5f + 0.5f);
        hitPositionClipSpace.z = hitPositionClipSpace.z * 0.5f + 0.5f;

        int2 hitPositionClipSpaceScreenCoord = int2(
            int32_t(hitPositionClipSpace.x * float(textureSize.x)),
            int32_t(hitPositionClipSpace.y * float(textureSize.y))
        );

        // depth difference between the hit position and current on screen world position
        // larger hit position's depth means it's in the back of the current on screen world position
        // so the difference greater than threshold means the hit position is obscured by the current world position
        float fDepthDiff = clipSpacePosition.z - hitPositionClipSpace.z;

        bool bOffScreen = !(clipSpacePosition.x >= 0.0f && clipSpacePosition.x <= 1.0f && clipSpacePosition.y >= 0.0f && clipSpacePosition.y <= 1.0f);
        if (counterBuffer[0] < 100/* &&
            bOffScreen && 
            fDepthDiff >= 0.01f*/)
        {
            // not on screen, register irradiance cache for hit position
            
            //if(abs(hitPosition.x) <= 100.0f && abs(hitPosition.y) <= 100.0f && abs(hitPosition.z) <= 100.0f)
            {
                debugTexture0[texCoord] = float4(hitPosition.xyz, 1.0f);

                float4 hitNormal  = hitNormalTexture[texCoord];
                updateIrradianceCache(
                    worldPosition.xyz,
                    normal.xyz,
                    hitPosition.xyz,
                    hitNormal.xyz,
                    randomResult,
                    texCoord.x,
                    texCoord.y);

                InterlockedAdd(counterBuffer[0], 1);
            }
        }
    }

}

/*
**
*/
uint32_t createIrradianceCacheEntry(
    in float3 position
)
{
    uint32_t iIrradianceCacheIndex = fetchIrradianceCacheIndex(position);

    float3 scaledPosition = position * 10.0f;
    float fSignX = sign(position.x);
    float fSignY = sign(position.y);
    float fSignZ = sign(position.z);
    scaledPosition.x = float(floor(abs(scaledPosition.x) + 0.5f)) * 0.1f * fSignX;
    scaledPosition.y = float(floor(abs(scaledPosition.y) + 0.5f)) * 0.1f * fSignY;
    scaledPosition.z = float(floor(abs(scaledPosition.z) + 0.5f)) * 0.1f * fSignZ; 
    
    irradianceCache[iIrradianceCacheIndex].mPosition = float4(scaledPosition, 1.0f);

    //for(uint32_t i = 0; i < 64u; i++)
    //{
    //    irradianceCache[iIrradianceCacheIndex].mImageProbe[i] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    //}

    irradianceCache[iIrradianceCacheIndex].mSphericalHarmonics0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    irradianceCache[iIrradianceCacheIndex].mSphericalHarmonics1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    irradianceCache[iIrradianceCacheIndex].mSphericalHarmonics2 = float4(0.0f, 0.0f, 0.0f, 0.0f);

    return iIrradianceCacheIndex;
}

/*
**
*/
UpdateIrradianceCacheResult updateIrradianceCache(
    in float3 worldPosition,
    in float3 normal,
    in float3 hitPosition,
    in float3 hitNormal,
    RandomResult randomResult: RandomResult,
    uint32_t iThreadX,
    uint32_t iThreadY
)
{
    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    UpdateIrradianceCacheResult ret;
    RandomResult randomResultCopy = randomResult;

    // create new irradiance cache entry if non-existent
    uint32_t iCurrIrradianceCacheEntry = fetchIrradianceCacheIndex(hitPosition);
    if(irradianceCache[iCurrIrradianceCacheEntry].mPosition.w <= 0.0f)
    {
        iCurrIrradianceCacheEntry = createIrradianceCacheEntry(hitPosition);
    }

    debugTexture0[uint2(iThreadX, iThreadY)].w = float(iCurrIrradianceCacheEntry);

    // sample ray for the center sample
    randomResultCopy = nextRand(randomResultCopy.miSeed);
    float fRand0 = randomResultCopy.mfNum;
    randomResultCopy = nextRand(randomResultCopy.miSeed);
    float fRand1 = randomResultCopy.mfNum;
    Ray rayDirection = uniformSampling(
        hitPosition,
        hitNormal,
        fRand0,
        fRand1);

    // hit position and hit normal
    IntersectBVHResult intersectionInfo;
    intersectionInfo.miHitTriangle = UINT32_MAX;

    RayDesc ray;
    ray.Origin = hitPosition + rayDirection.mDirection.xyz * 0.01f;
    ray.Direction = rayDirection.mDirection.xyz;
    ray.TMin = 0.001f;
    ray.TMax = 1000.0f;
    RayPayload payload = {
        float4(0.0f, 0.0f, 0.0f, 0.0f)
    };
    TraceRay(
        scene,
        RAY_FLAG_NONE,
        0xff,
        0,
        0,
        0,
        ray,
        payload
    );
    if(payload.mOutput.w > 0.0f)
    {
        // hit
        intersectionInfo.miHitTriangle = uint32_t(payload.mOutput.y);
        intersectionInfo.mBarycentricCoordinate = payload.mBarycentricCoord.xyz;

        intersectionInfo.miHitMesh = uint32_t(payload.mOutput.z);
        uint32_t iMeshOffset = meshTriangleRangeData[intersectionInfo.miHitMesh].miStart;

        uint32_t iTri0 = indexBuffer[iMeshOffset + intersectionInfo.miHitTriangle * 3];
        uint32_t iTri1 = indexBuffer[iMeshOffset + intersectionInfo.miHitTriangle * 3 + 1];
        uint32_t iTri2 = indexBuffer[iMeshOffset + intersectionInfo.miHitTriangle * 3 + 2];

        VertexFormat v0 = vertexBuffer[iTri0];
        VertexFormat v1 = vertexBuffer[iTri1];
        VertexFormat v2 = vertexBuffer[iTri2];

        intersectionInfo.mHitPosition =
            v0.mPosition.xyz * intersectionInfo.mBarycentricCoordinate +
            v1.mPosition.xyz * intersectionInfo.mBarycentricCoordinate +
            v2.mPosition.xyz * intersectionInfo.mBarycentricCoordinate;

        intersectionInfo.mHitNormal =
            v0.mNormal.xyz * intersectionInfo.mBarycentricCoordinate +
            v1.mNormal.xyz * intersectionInfo.mBarycentricCoordinate +
            v2.mNormal.xyz * intersectionInfo.mBarycentricCoordinate;
    }
    else
    {
        intersectionInfo.miHitTriangle = UINT32_MAX;
        intersectionInfo.mHitPosition.xyz = rayDirection.mDirection.xyz * 1000.0f;
        intersectionInfo.mHitNormal = float3(0.0f, 0.0f, 0.0f);
        intersectionInfo.mBarycentricCoordinate = float3(0.0f, 0.0f, 0.0f);
    }

    float4 candidateHitPosition = float4(intersectionInfo.mHitPosition.xyz, 1.0f);
    float4 candidateHitNormal = float4(intersectionInfo.mHitNormal.xyz, 1.0f);

    let fRayLength = length(intersectionInfo.mHitPosition.xyz - hitPosition);

    // process intersection
    var cacheRadiance = float3(0.0f, 0.0f, 0.0f);
    if(intersectionInfo.miHitTriangle == UINT32_MAX || fRayLength >= 100.0f)
    {
        // didn't hit anything, use skylight
        float fRadianceDP = max(dot(normal.xyz, rayDirection.mDirection.xyz), 0.0f);
        float2 skyUV = octahedronMap2(rayDirection.mDirection.xyz);

        int2 skyTextureSize;
        skyTexture.GetDimensions(skyTextureSize.x, skyTextureSize.y);

        int2 skyCoord = int2(
            int32_t(skyUV.x * float(skyTextureSize.x)),
            int32_t(skyUV.y * float(skyTextureSize.y))
        );
        float3 skyRadiance = skyTexture[skyCoord].xyz;
        float3 sunLight = sunLightTexture[skyCoord].xyz;
        cacheRadiance = sunLight + skyRadiance * fRadianceDP;

        // update irradiance cache probe with the radiance and ray direction
        updateIrradianceCacheProbe(
            cacheRadiance,
            rayDirection.mDirection.xyz * -1.0f,
            iCurrIrradianceCacheEntry);
    }
    else 
    {
        int32_t iHitTriangleIndex = int32_t(intersectionInfo.miHitTriangle);
        if (intersectionInfo.miHitMesh < defaultUniformData.miNumMeshes)
        {
            uint32_t iMaterialID = materialID[intersectionInfo.miHitMesh] - 1;
            float3 emissiveRadiance = materialData[iMaterialID].mEmissive.xyz;
            cacheRadiance += emissiveRadiance;
        }

        // hit another mesh triangle, get the irradiance cache in the vicinity
        int32_t iHitIrradianceCacheIndex = fetchIrradianceCacheIndex(candidateHitPosition.xyz);
        if(iHitIrradianceCacheIndex != iCurrIrradianceCacheEntry)
        {
            if(irradianceCache[iHitIrradianceCacheIndex].mPosition.w <= 0.0f)
            {
                // no valid irradiance cache here, create a new one
                createIrradianceCacheEntry(candidateHitPosition.xyz);
            }

            // get the radiance from probe image
            float fRadianceDP = max(dot(hitNormal.xyz, rayDirection.mDirection.xyz), 0.0f);
            cacheRadiance += getRadianceFromIrradianceCacheProbe(rayDirection.mDirection.xyz * -1.0f, iHitIrradianceCacheIndex) * fRadianceDP;
        
            // update irradiance cache probe with the radiance and ray direction
            if(length(cacheRadiance.xyz) > 0.0f)
            {
                updateIrradianceCacheProbe(
                    cacheRadiance,
                    rayDirection.mDirection.xyz * -1.0f,
                    iCurrIrradianceCacheEntry);
            }
        }
    }
    
    ret.mRandomResult = randomResultCopy;
    return ret;
}

struct SphericalHarmonicsEncodeResult
{
    float3 maCoefficients[4]; 
};

struct SphericalHarmonicsDecodeResult
{
    float4 mProbeImage[64];
};

/*
**
*/
SphericalHarmonicsEncodeResult encodeProbeImage(
    in float4 aFilteredProbeImage[64]
)
{
    SphericalHarmonicsEncodeResult ret;

    float4 probeImage[64];
    probeImage = aFilteredProbeImage;

    float3 aTotalCoefficients[4];
    aTotalCoefficients[0] = float3(0.0f, 0.0f, 0.0f);
    aTotalCoefficients[1] = float3(0.0f, 0.0f, 0.0f);
    aTotalCoefficients[2] = float3(0.0f, 0.0f, 0.0f);
    aTotalCoefficients[3] = float3(0.0f, 0.0f, 0.0f);
    
    // encode to spherical harmonics coefficients
    float fNumSamples = 0.0f;
    for(uint32_t iLocalTileImageY = 0; iLocalTileImageY < PROBE_IMAGE_SIZE; iLocalTileImageY++)
    {
        for(uint32_t iLocalTileImageX = 0; iLocalTileImageX < PROBE_IMAGE_SIZE; iLocalTileImageX++)
        {
            float2 tileUV = float2(
                float(iLocalTileImageX) / float(PROBE_IMAGE_SIZE),
                float(iLocalTileImageY) / float(PROBE_IMAGE_SIZE));

            // radiance and direction
            uint32_t iLocalImageIndex = iLocalTileImageY * PROBE_IMAGE_SIZE + iLocalTileImageX;
            float4 radiance = float4(probeImage[iLocalImageIndex].xyz, 1.0f);
            float3 direction = decodeOctahedronMap(tileUV);
            
            // encode coefficients with direction
            float afC[4];
            afC[0] = 0.282095f;
            afC[1] = 0.488603f;
            afC[2] = 0.488603f;
            afC[3] = 0.488603f;

            float afCoefficients[4];
            afCoefficients[0] = afC[0];
            afCoefficients[1] = afC[1] * direction.y;
            afCoefficients[2] = afC[2] * direction.z;
            afCoefficients[3] = afC[3] * direction.x;
            
            // encode radiance with direction coefficients
            float3 aResults[4];
            aResults[0] = radiance.xyz * afCoefficients[0];
            aResults[1] = radiance.xyz * afCoefficients[1];
            aResults[2] = radiance.xyz * afCoefficients[2];
            aResults[3] = radiance.xyz * afCoefficients[3];
            
            // apply to total coefficients
            aTotalCoefficients[0] += aResults[0];
            aTotalCoefficients[1] += aResults[1];
            aTotalCoefficients[2] += aResults[2];
            aTotalCoefficients[3] += aResults[3];

            fNumSamples += 1.0f;
            
        }   // for x = 0 to tile image size

    }   // for y = 0 to tile image size

    ret.maCoefficients = aTotalCoefficients;

    return ret;
}

/*
**
*/
SphericalHarmonicsDecodeResult decodeProbeImage(
    SphericalHarmonicsEncodeResult encoding,
    float3 maxRadiance,
    float fSampleCount)
{
    SphericalHarmonicsDecodeResult ret;

    float3 aTotalCoefficients[4];
    float fTotalSamples = float(PROBE_IMAGE_SIZE * PROBE_IMAGE_SIZE);
    float fFactor = (4.0f * 3.14159f) / fSampleCount;
    aTotalCoefficients[0] = encoding.maCoefficients[0] * fFactor;
    aTotalCoefficients[1] = encoding.maCoefficients[1] * fFactor;
    aTotalCoefficients[2] = encoding.maCoefficients[2] * fFactor;
    aTotalCoefficients[3] = encoding.maCoefficients[3] * fFactor;

    float fC1 = 0.42904276540489171563379376569857f;
    float fC2 = 0.51166335397324424423977581244463f;
    float fC3 = 0.24770795610037568833406429782001f;
    float fC4 = 0.88622692545275801364908374167057f;

    // apply coefficients for decoding
    for(int32_t iLocalTileImageY = 0; iLocalTileImageY < PROBE_IMAGE_SIZE; iLocalTileImageY++)
    {
        for(int32_t iLocalTileImageX = 0; iLocalTileImageX < PROBE_IMAGE_SIZE; iLocalTileImageX++)
        {
            float2 tileUV = float2(
                float(iLocalTileImageX) / float(PROBE_IMAGE_SIZE),
                float(iLocalTileImageY) / float(PROBE_IMAGE_SIZE));

            float3 direction  = decodeOctahedronMap(tileUV);
            float3 decoded =
                aTotalCoefficients[0] * fC4 +
                (aTotalCoefficients[3] * direction.x + aTotalCoefficients[1] * direction.y + aTotalCoefficients[2] * direction.z) * 
                fC2 * 2.0f;
            decoded = clamp(decoded, float3(0.0f, 0.0f, 0.0f), maxRadiance);
            uint32_t iImageIndex = iLocalTileImageY * PROBE_IMAGE_SIZE + iLocalTileImageX;
            ret.mProbeImage[iImageIndex] = float4(decoded, 1.0f);
        }
    }

    return ret;

}

/*
**
*/
void updateIrradianceCacheProbe(
    float3 radiance,
    float3 direction,
    uint32_t iCacheEntryIndex)
{
    // encode to spherical harmonics coefficients
    encodeSphericalHarmonicCoefficients(
        radiance,
        direction,
        iCacheEntryIndex
    );

    irradianceCache[iCacheEntryIndex].mSampleCount.z = float(defaultUniformData.miFrame);
}

/*
**
*/
void encodeSphericalHarmonicCoefficients(
    in float3 radiance,
    in float3 direction,
    in uint32_t iCacheEntryIndex
)
{
    float4 SHCoefficent0 = irradianceCache[iCacheEntryIndex].mSphericalHarmonics0; 
    float4 SHCoefficent1 = irradianceCache[iCacheEntryIndex].mSphericalHarmonics1; 
    float4 SHCoefficent2 = irradianceCache[iCacheEntryIndex].mSphericalHarmonics2;

    float4 afC = float4(
        0.282095f,
        0.488603f,
        0.488603f,
        0.488603f
    );

    float4 A = float4(
        0.886227f,
        1.023326f,
        1.023326f,
        1.023326f
    );

    // encode coefficients with direction
    float4 coefficient = float4(
        afC.x * A.x,
        afC.y * direction.y * A.y,
        afC.z * direction.z * A.z,
        afC.w * direction.x * A.w
    );

    // encode with radiance
    float3 aResults[4];
    aResults[0] = radiance.xyz * coefficient.x;
    aResults[1] = radiance.xyz * coefficient.y;
    aResults[2] = radiance.xyz * coefficient.z;
    aResults[3] = radiance.xyz * coefficient.w;

    SHCoefficent0.x += aResults[0].x;
    SHCoefficent0.y += aResults[0].y;
    SHCoefficent0.z += aResults[0].z;
    SHCoefficent0.w += aResults[1].x;

    SHCoefficent1.x += aResults[1].y;
    SHCoefficent1.y += aResults[1].z;
    SHCoefficent1.z += aResults[2].x;
    SHCoefficent1.w += aResults[2].y;

    SHCoefficent2.x += aResults[2].z;
    SHCoefficent2.y += aResults[3].x;
    SHCoefficent2.z += aResults[3].y;
    SHCoefficent2.w += aResults[3].z;

    irradianceCache[iCacheEntryIndex].mSphericalHarmonics0 = SHCoefficent0;
    irradianceCache[iCacheEntryIndex].mSphericalHarmonics1 = SHCoefficent1;
    irradianceCache[iCacheEntryIndex].mSphericalHarmonics2 = SHCoefficent2;

    irradianceCache[iCacheEntryIndex].mSampleCount.x += 1.0f;
}

/*
**
*/
float3 decodeFromSphericalHarmonicCoefficients(
    in float3 direction,
    in uint32_t iCacheEntryIndex
)
{
    float4 SHCoefficent0 = irradianceCache[iCacheEntryIndex].mSphericalHarmonics0;
    float4 SHCoefficent1 = irradianceCache[iCacheEntryIndex].mSphericalHarmonics1;
    float4 SHCoefficent2 = irradianceCache[iCacheEntryIndex].mSphericalHarmonics2;

    float3 aTotalCoefficients[4];
    float fFactor = 1.0f / max(irradianceCache[iCacheEntryIndex].mSampleCount.x, 1.0f);

    aTotalCoefficients[0] = float3(SHCoefficent0.x, SHCoefficent0.y, SHCoefficent0.z) * fFactor;
    aTotalCoefficients[1] = float3(SHCoefficent0.w, SHCoefficent1.x, SHCoefficent1.y) * fFactor;
    aTotalCoefficients[2] = float3(SHCoefficent1.z, SHCoefficent1.w, SHCoefficent2.x) * fFactor;
    aTotalCoefficients[3] = float3(SHCoefficent2.y, SHCoefficent2.z, SHCoefficent2.w) * fFactor;

    float fC1 = 0.42904276540489171563379376569857f;
    float fC2 = 0.51166335397324424423977581244463f;
    float fC3 = 0.24770795610037568833406429782001f;
    float fC4 = 0.88622692545275801364908374167057f;

    float3 decoded =
        aTotalCoefficients[0] * fC4 +
        (aTotalCoefficients[3] * direction.x + aTotalCoefficients[1] * direction.y + aTotalCoefficients[2] * direction.z) *
            fC2 * 2.0f;

    return decoded;
}





