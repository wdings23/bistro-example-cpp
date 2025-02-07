#define UINT32_MAX 0xffffffff
#define FLT_MAX  1.0e+10
#define PI 3.14159f
#define PROBE_IMAGE_SIZE 8u

#include "default-uniform-headers.slang"
//#include "irradiance-cache-headers.slang"
#include "murmur-hash-headers.slang"
#include "octahedron-map-headers.slang"
#include "random-headers.slang"
//#include "spherical-harmonics-headers.slang"
#include "bounced-radiance-headers.slang"

struct VertexFormat
{
    float4 mPosition;
    float4 mTexCoord;
    float4 mNormal;
};

struct RayPayload
{
    float4 mOutput;
    float4 mBarycentricCoord;
};

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

struct SpatialRestirResult
{
    float4 mReservoir;
    float4 mHitPosition;
    RandomResult mRandomResult;

    float3 mStartPosition;
    float3 mStartNormal;

    bool mbExchanged;
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

struct OnScreenRadianceResult
{
    float3 mRadiance;
    bool mbOnScreen;
};

[[vk::binding(0, 0)]] RWTexture2D<float4> radianceOutput;
[[vk::binding(1, 0)]] RWTexture2D<float4> reservoirOutput;
[[vk::binding(2, 0)]] RWTexture2D<float4> hitPositionOutput;
[[vk::binding(3, 0)]] RWTexture2D<float4> decodedSphericalHarmonicsTexture;
[[vk::binding(4, 0)]] RWTexture2D<float4> sphericalHarmonicCoefficientTexture0;
[[vk::binding(5, 0)]] RWTexture2D<float4> sphericalHarmonicCoefficientTexture1;
[[vk::binding(6, 0)]] RWTexture2D<float4> sphericalHarmonicCoefficientTexture2;
[[vk::binding(7, 0)]] RWTexture2D<float4> debugTexture0;
[[vk::binding(8, 0)]] RWTexture2D<float4> debugTexture1;
[[vk::binding(9, 0)]] RWTexture2D<float4> debugTexture2;
[[vk::binding(10, 0)]] RWTexture2D<float4> debugTexture3;

[[vk::binding(11, 0)]] RaytracingAccelerationStructure scene;

[[vk::binding(12, 0)]] Texture2D<float4> spatialSphericalHarmonicCoefficientTexture0;
[[vk::binding(13, 0)]] Texture2D<float4> spatialSphericalHarmonicCoefficientTexture1;
[[vk::binding(14, 0)]] Texture2D<float4> spatialSphericalHarmonicCoefficientTexture2;

[[vk::binding(15, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(16, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(17, 0)]] Texture2D<float4> skyTexture;

[[vk::binding(18, 0)]] Texture2D<float4> spatialReservoirTexture;
[[vk::binding(19, 0)]] Texture2D<float4> spatialRadianceTexture;
[[vk::binding(20, 0)]] Texture2D<float4> spatialHitPositionTexture;

[[vk::binding(21, 0)]] Texture2D<float4> directSunOutputTexture;
[[vk::binding(22, 0)]] Texture2D<float4> albedoTexture;

[[vk::binding(23, 0)]] Texture2D<float4> ambientOcclusionTexture;
[[vk::binding(24, 0)]] Texture2D<float4> sphericalHarmonicsSampleCountTexture;
[[vk::binding(25, 0)]] Texture2D<float4> initialTextureAtlas;
[[vk::binding(26, 0)]] Texture2D<float4> prevSpatialReservoirTexture;
[[vk::binding(27, 0)]] Texture2D<float4> prevSpatialHitPositionTexture;

[[vk::binding(28, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(29, 0)]] StructuredBuffer<IrradianceCacheEntry> irradianceCache;

[[vk::binding(0, 1)]] Texture2D<float4> blueNoiseTexture;
[[vk::binding(1, 1)]] StructuredBuffer<VertexFormat> vertexBuffer;
[[vk::binding(2, 1)]] StructuredBuffer<uint32_t> indexBuffer;
[[vk::binding(3, 1)]] StructuredBuffer<Material> materialData;
[[vk::binding(4, 1)]] StructuredBuffer<uint32_t> materialID;
[[vk::binding(5, 1)]] StructuredBuffer<MeshRange> meshRanges;
[[vk::binding(6, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

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
    uint2 rayIndex = DispatchRaysIndex().xy;

    RandomResult randomResult = initRand(
        uint32_t(rayIndex.x * 100.0f + rayIndex.y * 200.0f) + uint32_t(defaultUniformBuffer.mfRand0 * 100.0f),
        uint32_t(rayIndex.x * 10.0f + rayIndex.y * 20.0f) + uint32_t(defaultUniformBuffer.mfRand1 * 100.0f),
        10u);

    int2 outputTextureSize;
    radianceOutput.GetDimensions(outputTextureSize.x, outputTextureSize.y);

    float3 worldPosition = worldPositionTexture[rayIndex].xyz;
    float3 normal = normalTexture[rayIndex].xyz;

    SpatialRestirResult result;
    result.mRandomResult = randomResult;
    result.mbExchanged = false;
    result.mReservoir = spatialReservoirTexture[rayIndex];
    result.mHitPosition = spatialHitPositionTexture[rayIndex];

    float4 ambientOcclusion = ambientOcclusionTexture[rayIndex];

    float fNumSamples = ambientOcclusion.y;
    float fMaxFrames = 12.0f;
    float fMaxRadius = 6.0f;
    float fMinRadius = 1.0f;
    float fRadiusPct = min(fNumSamples, fMaxFrames) / fMaxFrames;
    float fRadius = fMinRadius + (1.0f - fRadiusPct) * (fMaxRadius - fMinRadius);  //max(fMaxRadius * (1.0f - fRadiusPct), 2.0f);
    result.mbExchanged = false;
    result.mStartPosition = worldPosition;
    result.mStartNormal = normal;
    result = spatialRestir(
        result,
        rayIndex,
        18,
        fRadius,
        1800.0f
    );

    reservoirOutput[rayIndex] = result.mReservoir;
    hitPositionOutput[rayIndex] = result.mHitPosition;

    float3 rayDirection = normalize(result.mHitPosition.xyz - worldPosition);
    float2 screenUV = float2(
        float(rayIndex.x) / float(outputTextureSize.x),
        float(rayIndex.y) / float(outputTextureSize.y)
    );
    
    radianceOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 1.0f);

    float fOneOverPDF = 1.0f; // 1.0f / PI;

    RayDesc ray;
    ray.Origin = worldPosition + rayDirection * 0.01f;
    ray.Direction = rayDirection;
    ray.TMin = 0.01f;
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

    debugTexture1[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    debugTexture2[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    debugTexture3[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 hitPosition = float3(0.0f, 0.0f, 0.0f);
    float3 barycentricCoordinate = float3(0.0f, 0.0f, 0.0f);
    float3 onScreenRadiance = float3(0.0f, 0.0f, 0.0f);
    uint32_t iHitType = 0;
    if(payload.mOutput.w > 0.0f)
    {
        // hit
        uint32_t iHitTriangle = uint32_t(payload.mOutput.y);
        uint32_t iMesh = uint32_t(payload.mOutput.z);
        uint32_t iMeshOffset = meshRanges[iMesh].miStart;

        barycentricCoordinate = payload.mBarycentricCoord.xyz;
        
        uint32_t iTri0 = indexBuffer[iMeshOffset + iHitTriangle * 3];
        uint32_t iTri1 = indexBuffer[iMeshOffset + iHitTriangle * 3 + 1];
        uint32_t iTri2 = indexBuffer[iMeshOffset + iHitTriangle * 3 + 2];

        VertexFormat v0 = vertexBuffer[iTri0];
        VertexFormat v1 = vertexBuffer[iTri1];
        VertexFormat v2 = vertexBuffer[iTri2];

        hitPosition =
            v0.mPosition.xyz * barycentricCoordinate.x +
            v1.mPosition.xyz * barycentricCoordinate.y +
            v2.mPosition.xyz * barycentricCoordinate.z;

        float3 hitNormal =
            v0.mNormal.xyz * barycentricCoordinate.x +
            v1.mNormal.xyz * barycentricCoordinate.y +
            v2.mNormal.xyz * barycentricCoordinate.z;

        float2 hitUV =
            v0.mTexCoord.xy * barycentricCoordinate.x +
            v1.mTexCoord.xy * barycentricCoordinate.y +
            v2.mTexCoord.xy * barycentricCoordinate.z;

        float fDP0 = max(dot(result.mStartNormal, normalize(result.mHitPosition.xyz - worldPosition)), 0.0f);
        float fDP1 = max(dot(result.mStartNormal, normalize(result.mHitPosition.xyz - result.mStartPosition)), 1.0e-4f);
        float fJacobian = fDP0 / fDP1;
        float fRadianceDP = 1.0f;
        float fDistanceAttenuation = 1.0f;
        uint32_t iMaterialID = materialID[iMesh];

        float fReflectivity = 0.5f;

        float2 motionVector = motionVectorTexture[rayIndex].xy * 2.0f - 1.0f;
        BouncedRadianceInfo bouncedRadianceInfo = getBouncedRadiance(
            worldPosition,
            normal,
            hitPosition,
            hitNormal,
            hitUV,
            float2(0.0f, 0.0f),
            0.8f,
            defaultUniformBuffer.mViewProjectionMatrix,
            outputTextureSize,
            iMaterialID,
            spatialRadianceTexture
        );
        onScreenRadiance.xyz =
            bouncedRadianceInfo.mRadiance *
            bouncedRadianceInfo.mfAnglePct *
            bouncedRadianceInfo.mfDistanceAttenuation *
            fJacobian *
            fOneOverPDF;

        iHitType = bouncedRadianceInfo.miIntersectionType;

        if(length(onScreenRadiance.xyz) > 0.0f)
        {
            encodeSphericalHarmonicsCoefficients(
                onScreenRadiance,
                ray.Direction.xyz,
                rayIndex
            );
        }
    }
    else
    {

        // miss
        float2 skyUV = octahedronMap2(ray.Direction.xyz);
        uint2 skyTextureSize = uint2(0, 0);
        skyTexture.GetDimensions(skyTextureSize.x, skyTextureSize.y);
        uint2 skyCoord = uint2(
            uint32_t(skyUV.x * float(skyTextureSize.x)),
            uint32_t(skyUV.y * float(skyTextureSize.y))
        );

        float fDP = max(dot(normal, ray.Direction.xyz), 0.0f);
        radianceOutput[rayIndex] += float4(skyTexture[skyCoord].xyz, 1.0f) * fDP * fOneOverPDF;

        encodeSphericalHarmonicsCoefficients(
            radianceOutput[rayIndex].xyz,
            ray.Direction.xyz,
            rayIndex
        );
    }

    radianceOutput[rayIndex].xyz += onScreenRadiance;

    float fAO = 1.0f - ambientOcclusionTexture[rayIndex].x / ambientOcclusionTexture[rayIndex].y;
    float fWeight = (fAO < result.mReservoir.w) ? result.mReservoir.w : fAO; // (reservoirOutput[rayIndex].z >= 500.0f) ? result.mReservoir.w : 1.0f;

    radianceOutput[rayIndex].xyz *= fWeight;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    float2 uv = float2( 
        float(rayIndex.x) / float(textureSize.x),
        float(rayIndex.y) / float(textureSize.y)
    );
    float3 totalSHOutput = decodeFromSphericalHarmonicCoefficients(
        normal,
        float3(1.0f, 1.0f, 1.0f),
        rayIndex,
        max(sphericalHarmonicsSampleCountTexture[rayIndex].x + 1.0f, 1.0f)
    );
    decodedSphericalHarmonicsTexture[rayIndex].xyz = totalSHOutput;
    decodedSphericalHarmonicsTexture[rayIndex].w = 1.0f;

    debugTexture0[rayIndex] = float4(hitPosition.xyz, payload.mOutput.w);
    if(payload.mOutput.w > 0.0f)
    {
        //float4x4 viewMatrix = defaultUniformBuffer.mViewMatrix;
        //float4x4 projectionMatrix = defaultUniformBuffer.mProjectionMatrix;
        //projectionMatrix._22 *= -1.0f;
        //float4x4 viewProjectionMatrix = mul(
        //    projectionMatrix,
        //    viewMatrix
        //);

        // get on-screen radiance if there's any
        float4 clipSpacePosition = mul(defaultUniformBuffer.mViewProjectionMatrix, float4(hitPosition.xyz, 1.0));
        clipSpacePosition.x /= clipSpacePosition.w;
        clipSpacePosition.y /= clipSpacePosition.w;
        clipSpacePosition.z /= clipSpacePosition.w;
        clipSpacePosition.x = clipSpacePosition.x * 0.5f + 0.5f;
        clipSpacePosition.y = (clipSpacePosition.y * 0.5f + 0.5f);
        clipSpacePosition.z = clipSpacePosition.z * 0.5f + 0.5f;

        uint2 clipSpaceCoord = uint2(
            uint32_t(floor(clipSpacePosition.x * float(textureSize.x) + 0.5f)),
            uint32_t(floor(clipSpacePosition.y * float(textureSize.y) + 0.5f))
        );

        // hit
        uint32_t iHitTriangle = uint32_t(payload.mOutput.y);
        barycentricCoordinate = payload.mBarycentricCoord.xyz;

        uint32_t iTri0 = indexBuffer[iHitTriangle * 3];
        uint32_t iTri1 = indexBuffer[iHitTriangle * 3 + 1];
        uint32_t iTri2 = indexBuffer[iHitTriangle * 3 + 2];

        VertexFormat v0 = vertexBuffer[iTri0];
        VertexFormat v1 = vertexBuffer[iTri1];
        VertexFormat v2 = vertexBuffer[iTri2];

        debugTexture1[rayIndex].xyz = clipSpacePosition.xyz;
        debugTexture2[rayIndex].xy = float2(float(clipSpaceCoord.x), float(clipSpaceCoord.y));
        debugTexture2[rayIndex].zw = float2(clipSpacePosition.x, clipSpacePosition.y);
    }

    debugTexture2[rayIndex] = float4(fAO, fAO, fAO, 1.0f);

    if (iHitType == 1 && onScreenRadiance.x < FLT_MAX)
    {
        debugTexture3[rayIndex].xyz = onScreenRadiance;
    }
}

/*
**
*/
SpatialRestirResult spatialRestir(
    in SpatialRestirResult prevResult,
    in int2 screenCoord,
    in int32_t iNumSamples,
    in float fRadius,
    in float fMaxReservoirSamples)
{
    SpatialRestirResult ret = prevResult;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);
    float fOneOverScreenWidth = 1.0f / float(textureSize.x);
    float fOneOverScreenHeight = 1.0f / float(textureSize.y);
    float2 screenUV = float2(
        float(screenCoord.x) * fOneOverScreenWidth,
        float(screenCoord.y) * fOneOverScreenHeight
    );
    float2 motion = motionVectorTexture[screenCoord].xy * 2.0f - 1.0f;
    float2 prevScreenUV = screenUV - motion;
    int2 prevScreenCoord = int2(
        int32_t(prevScreenUV.x * float(textureSize.x)),
        int32_t(prevScreenUV.y * float(textureSize.y))
    );

    ret.mReservoir = prevSpatialReservoirTexture[prevScreenCoord];
    ret.mHitPosition = prevSpatialHitPositionTexture[prevScreenCoord];

    float3 normal = normalTexture[screenCoord].xyz;

    uint2 blueNoiseTextureSize;
    blueNoiseTexture.GetDimensions(
        blueNoiseTextureSize.x,
        blueNoiseTextureSize.y
    );

    // blue noise tile coordinate
    uint32_t iTileSize = 32u;
    uint32_t iNumTilesPerRow = blueNoiseTextureSize.x / iTileSize;
    uint32_t iNumTotalTiles = iNumTilesPerRow * iNumTilesPerRow;

    uint32_t iCurrIndex = uint32_t(defaultUniformBuffer.miFrame + 20) * uint32_t(iNumSamples);

    float3 worldPosition = worldPositionTexture[screenCoord].xyz;

    float fJacobian = 1.0f;
    int2 blueNoiseScreenCoord;
    for(int32_t iSample = 0; iSample < iNumSamples; iSample++)
    {
        uint32_t iTileX = (iCurrIndex + uint32_t(iNumSamples)) % iNumTilesPerRow;
        uint32_t iTileY = ((iCurrIndex + uint32_t(iNumSamples)) / iNumTilesPerRow) % (iNumTilesPerRow * iNumTilesPerRow);
        uint32_t iTileOffsetX = (iCurrIndex + uint32_t(iNumSamples)) % iTileSize;
        uint32_t iTileOffsetY = ((iCurrIndex + uint32_t(iNumSamples)) / iTileSize) % (iTileSize * iTileSize);
        uint32_t iOffsetX = iTileOffsetX + iTileX * iTileSize;
        uint32_t iOffsetY = iTileOffsetY + iTileY * iTileSize;
        blueNoiseScreenCoord.x = (screenCoord.x + iOffsetX) % blueNoiseTextureSize.x;
        blueNoiseScreenCoord.y = (screenCoord.y + iOffsetY) % blueNoiseTextureSize.y;

        float4 blueNoise = blueNoiseTexture[blueNoiseScreenCoord];

        int2 sampleCoord = screenCoord + int2(
            int32_t((blueNoise.x - 0.5f) * fRadius), 
            int32_t((blueNoise.y - 0.5f) * fRadius)
        );
        sampleCoord = clamp(sampleCoord, int2(0, 0), int2(textureSize.x - 1, textureSize.y - 1));

        if(iSample == 0)
        {
            sampleCoord = screenCoord;
        }

        ret.mRandomResult = nextRand(ret.mRandomResult.miSeed);
        float fRand = ret.mRandomResult.mfNum;

        float4 neighborReservoir = spatialReservoirTexture[sampleCoord];
        float fWeight = neighborReservoir.y * neighborReservoir.z * neighborReservoir.w;
        ret.mReservoir.x += fWeight;
        ret.mReservoir.z += neighborReservoir.z;
        float fWeightPct = fWeight / max(ret.mReservoir.x, 0.001f);
        if(fRand < fWeightPct)
        {
            ret.mReservoir.y = neighborReservoir.y;
            ret.mHitPosition = spatialHitPositionTexture[sampleCoord];
            ret.mbExchanged = true;

            ret.mStartPosition = worldPositionTexture[sampleCoord].xyz;
            ret.mStartNormal = normalTexture[sampleCoord].xyz;
        }
        
    }

    // clamp reservoir
    if(ret.mReservoir.z > fMaxReservoirSamples)
    {
        float fPct = fMaxReservoirSamples / ret.mReservoir.z;
        ret.mReservoir.x *= fPct;
        ret.mReservoir.z = fMaxReservoirSamples;
    }

    ret.mReservoir.w = clamp(ret.mReservoir.x / (ret.mReservoir.z * ret.mReservoir.y + 0.0001f), 0.2f, 1.0f);

    return ret;
}

/*
**
*/
OnScreenRadianceResult getOnScreenRadiance(
    in float3 hitPosition,
    in float3 worldPosition,
    in int2 screenCoord
)
{
    OnScreenRadianceResult ret;
    ret.mbOnScreen = false;
    ret.mRadiance = float3(0.0f, 0.0f, 0.0f);

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    // distance for on-screen radiance and ambient occlusion
    float fDistance = length(hitPosition.xyz - worldPosition);
    float fDistanceAttenuation = 1.0f / max(fDistance * fDistance, 1.0f);

    //float4x4 viewMatrix = defaultUniformBuffer.mViewMatrix;
    //float4x4 projectionMatrix = defaultUniformBuffer.mProjectionMatrix;
    //projectionMatrix._22 *= -1.0f;
    //float4x4 viewProjectionMatrix = mul(
    //    projectionMatrix,
    //    viewMatrix
    //);

    // get on-screen radiance if there's any
    float4 clipSpacePosition = mul(defaultUniformBuffer.mViewProjectionMatrix, float4(hitPosition.xyz, 1.0));
    clipSpacePosition.x /= clipSpacePosition.w;
    clipSpacePosition.y /= clipSpacePosition.w;
    clipSpacePosition.z /= clipSpacePosition.w;
    clipSpacePosition.x = clipSpacePosition.x * 0.5f + 0.5f;
    clipSpacePosition.y = (clipSpacePosition.y * 0.5f + 0.5f);
    clipSpacePosition.z = clipSpacePosition.z * 0.5f + 0.5f;

    uint2 clipSpaceCoord = uint2(
        uint32_t(floor(clipSpacePosition.x * float(textureSize.x) + 0.5f)),
        uint32_t(floor(clipSpacePosition.y * float(textureSize.y) + 0.5f))
    );
    
    // see if it's actually on the screen by checking the onscreen position to the calculated position from above
    float4 worldSpaceHitPosition = worldPositionTexture[clipSpaceCoord];
    float4 hitPositionClipSpace = mul(defaultUniformBuffer.mViewProjectionMatrix, float4(worldSpaceHitPosition.xyz, 1.0f));
    hitPositionClipSpace.x /= hitPositionClipSpace.w;
    hitPositionClipSpace.y /= hitPositionClipSpace.w;
    hitPositionClipSpace.z /= hitPositionClipSpace.w;
    hitPositionClipSpace.x = hitPositionClipSpace.x * 0.5f + 0.5f;
    hitPositionClipSpace.y = (hitPositionClipSpace.y * 0.5f + 0.5f);
    hitPositionClipSpace.z = hitPositionClipSpace.z * 0.5f + 0.5f;

    float fDepthDiff = abs(hitPositionClipSpace.z - clipSpacePosition.z);
    if(clipSpacePosition.z >= 0.0f && clipSpacePosition.z <= 1.0f &&
       clipSpacePosition.x >= 0.0f && clipSpacePosition.x <= 1.0f &&
       clipSpacePosition.y >= 0.0f && clipSpacePosition.y <= 1.0f && 
       fDepthDiff <= 0.1f)
    {
        // on screen

        float fReflectivity = 0.3f;

        ret.mRadiance = spatialRadianceTexture[clipSpaceCoord].xyz;
        ret.mRadiance += directSunOutputTexture[clipSpaceCoord].xyz * albedoTexture[clipSpaceCoord].xyz;
        
        // distance attenuation
        float3 diff = hitPosition.xyz - worldPosition.xyz;
        float fDistanceAttenuation = 1.0f / max(dot(diff, diff), 1.0f) * fReflectivity;
        ret.mRadiance *= fDistanceAttenuation;

        ret.mbOnScreen = true;
    }

    return ret;
}

/*
**
*/
float computeLuminance(
    in float3 radiance)
{
    return dot(radiance, float3(0.2126f, 0.7152f, 0.0722f));
}

#if 0
/*
**
*/
float3 getIrradianceCacheRadiance(
    in float3 position,
    in float3 worldPosition)
{
    float fReflectivity = 1.0f;

    float3 ret = float3(0.0f, 0.0f, 0.0f);

    uint32_t iHitIrradianceCacheIndex = fetchIrradianceCacheIndex(position);
    float4 irradianceCachePosition = irradianceCache[iHitIrradianceCacheIndex].mPosition;
    if (irradianceCachePosition.w > 0.0f)
    {
        float3 diff = irradianceCachePosition.xyz - worldPosition;
        float3 positionToCacheDirection = normalize(irradianceCachePosition.xyz - worldPosition) * -1.0f;
        float3 irradianceCacheProbeRadiance = getRadianceFromIrradianceCacheProbe(
            positionToCacheDirection,
            iHitIrradianceCacheIndex);
        float fLengthSquared = dot(diff, diff);
        ret.xyz = (irradianceCacheProbeRadiance.xyz * fReflectivity) / max(fLengthSquared, 1.0f);
    }

    return ret;
}
#endif // #if 0

/*
**
*/
void encodeSphericalHarmonicsCoefficients(
    in float3 radiance,
    in float3 rayDirection,
    in int2 screenCoord
)
{
    float4 SHCoefficent0 = spatialSphericalHarmonicCoefficientTexture0[screenCoord];
    float4 SHCoefficent1 = spatialSphericalHarmonicCoefficientTexture1[screenCoord];
    float4 SHCoefficent2 = spatialSphericalHarmonicCoefficientTexture2[screenCoord];

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
        afC.y * rayDirection.y * A.y,
        afC.z * rayDirection.z * A.z,
        afC.w * rayDirection.x * A.w
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

    sphericalHarmonicCoefficientTexture0[screenCoord] = SHCoefficent0;
    sphericalHarmonicCoefficientTexture1[screenCoord] = SHCoefficent1;
    sphericalHarmonicCoefficientTexture2[screenCoord] = SHCoefficent2;
    
}

/*
**
*/
float3 decodeFromSphericalHarmonicCoefficients(
    in float3 direction,
    in float3 maxRadiance,
    in int2 screenCoord,
    in float fRayCount
)
{
    float4 SHCoefficent0 = sphericalHarmonicCoefficientTexture0[screenCoord];
    float4 SHCoefficent1 = sphericalHarmonicCoefficientTexture1[screenCoord];
    float4 SHCoefficent2 = sphericalHarmonicCoefficientTexture2[screenCoord];

    float3 aTotalCoefficients[4];
    aTotalCoefficients[0] = float3(SHCoefficent0.x, SHCoefficent0.y, SHCoefficent0.z);
    aTotalCoefficients[1] = float3(SHCoefficent0.w, SHCoefficent1.x, SHCoefficent1.y);
    aTotalCoefficients[2] = float3(SHCoefficent1.z, SHCoefficent1.w, SHCoefficent2.x);
    aTotalCoefficients[3] = float3(SHCoefficent2.y, SHCoefficent2.z, SHCoefficent2.w);

    float fC1 = 0.42904276540489171563379376569857f;
    float fC2 = 0.51166335397324424423977581244463f;
    float fC3 = 0.24770795610037568833406429782001f;
    float fC4 = 0.88622692545275801364908374167057f;

    float3 decoded =
        aTotalCoefficients[0] * fC4 +
        (aTotalCoefficients[3] * direction.x + aTotalCoefficients[1] * direction.y + aTotalCoefficients[2] * direction.z) *
            fC2 * 2.0f;
    decoded = clamp(decoded, float3(0.0f, 0.0f, 0.0f), maxRadiance);

    return decoded;
}