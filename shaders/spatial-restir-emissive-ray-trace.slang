#define UINT32_MAX 0xffffffff
#define FLT_MAX  1.0e+10
#define PI 3.14159f
#define PROBE_IMAGE_SIZE 8u

#include "default-uniform-headers.slang"
#include "murmur-hash-headers.slang"
#include "octahedron-map-headers.slang"
#include "random-headers.slang"

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

[[vk::binding(12, 0)]] Texture2D<float4> temporalSphericalHarmonicCoefficientTexture0;
[[vk::binding(13, 0)]] Texture2D<float4> temporalSphericalHarmonicCoefficientTexture1;
[[vk::binding(14, 0)]] Texture2D<float4> temporalSphericalHarmonicCoefficientTexture2;

[[vk::binding(15, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(16, 0)]] Texture2D<float4> normalTexture;

[[vk::binding(17, 0)]] Texture2D<float4> temporalReservoirTexture;
[[vk::binding(18, 0)]] Texture2D<float4> temporalRadianceTexture;
[[vk::binding(19, 0)]] Texture2D<float4> temporalHitPositionTexture;

[[vk::binding(20, 0)]] Texture2D<float4> prevSpatialRadianceTexture;
[[vk::binding(21, 0)]] Texture2D<float4> prevSpatialReservoirTexture;
[[vk::binding(22, 0)]] Texture2D<float4> prevSpatialHitPositionTexture;

[[vk::binding(23, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(24, 0)]] Texture2D<float4> ambientOcclusionTexture;

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
        uint32_t(rayIndex.x * 100.0f + rayIndex.y) + uint32_t(defaultUniformBuffer.mfRand0 * 100.0f),
        uint32_t(rayIndex.x * 10.0f + rayIndex.y) + uint32_t(defaultUniformBuffer.mfRand1 * 100.0f),
        10u);

    int2 outputTextureSize;
    radianceOutput.GetDimensions(outputTextureSize.x, outputTextureSize.y);

    float3 worldPosition = worldPositionTexture[rayIndex].xyz;
    float3 normal = normalTexture[rayIndex].xyz;

    SpatialRestirResult result;
    result.mRandomResult = randomResult;
    result.mbExchanged = false;
    result.mReservoir = temporalReservoirTexture[rayIndex];
    result.mHitPosition = temporalHitPositionTexture[rayIndex];
    result.mStartPosition = worldPosition;
    result.mStartNormal = normal;

    float4 ambientOcclusion = ambientOcclusionTexture[rayIndex];

    float fNumSamples = ambientOcclusion.y;
    float fMaxFrames = 64.0f;
    float fMaxRadius = 64.0f;
    float fMinRadius = 64.0f;
    float fRadiusPct = min(fNumSamples, fMaxFrames) / fMaxFrames;
    float fRadius = fMinRadius + (1.0f - fRadiusPct) * (fMaxRadius - fMinRadius);  //max(fMaxRadius * (1.0f - fRadiusPct), 2.0f);
    result.mbExchanged = false;
    result = spatialRestir(
        result,
        rayIndex,
        24,
        fRadius,
        1200.0f
    );

    reservoirOutput[rayIndex] = result.mReservoir;
    hitPositionOutput[rayIndex] = result.mHitPosition;
    radianceOutput[rayIndex] = prevSpatialRadianceTexture[rayIndex];

    float3 rayDirection = result.mHitPosition.xyz - worldPosition;
    float fOneOverPDF = 1.0f; // 1.0f / PI;

    RayDesc ray;
    ray.Origin = worldPosition + rayDirection * 0.1f;
    ray.Direction = rayDirection;
    ray.TMin = 0.1f;
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

    int32_t iHitMesh = int32_t(result.mHitPosition.w);

    float4 temporalHitPosition = temporalHitPositionTexture[rayIndex];
    debugTexture0[rayIndex] = float4(float(iHitMesh), payload.mOutput.z, temporalHitPosition.w, 0.0f);

    debugTexture1[rayIndex] = float4(0.0f, 0.0f, 1.0f, 0.0f);
    debugTexture2[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    debugTexture3[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float3 radiance = float3(0.0f, 0.0f, 0.0f);
    if(payload.mOutput.w > 0.0f)
    {
        uint32_t iHitTriangle = uint32_t(payload.mOutput.y);
        uint32_t iMesh = uint32_t(payload.mOutput.z);

        hitPositionOutput[rayIndex].w = float(iMesh);
        if(iMesh == iHitMesh)
        {
            float fRadianceDP = max(dot(normalize(rayDirection), normal), 0.0f);
            float fDistanceAttenuation = 1.0f / max(dot(rayDirection, rayDirection), 1.0f);
            int32_t iMaterialID = materialID[iMesh] - 1;

            Material material = materialData[iMaterialID];
            radiance =
                material.mEmissive.xyz * 10.0f *
                fRadianceDP *
                fDistanceAttenuation;

            debugTexture1[rayIndex] = float4(1.0f, 1.0f, 1.0f, 1.0f);
            
            if(length(material.mEmissive.xyz) <= 0.0f)
            {
                debugTexture1[rayIndex] = float4(0.0f, 1.0f, 0.0f, 1.0f);
                reservoirOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);

                radianceOutput[rayIndex].w = 0.0f;
            }
            else
            {
                reservoirOutput[rayIndex].y = computeLuminance(radiance);
                radianceOutput[rayIndex] = float4(radiance, 1.0f);
            }
        }
        else
        {
            reservoirOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
            debugTexture1[rayIndex] = float4(0.0f, 0.0f, 0.0f, 1.0f);
            radianceOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        }

        encodeSphericalHarmonicsCoefficients(
            radiance,
            rayDirection.xyz,
            rayIndex
        );

    }
    else
    {
        reservoirOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        radianceOutput[rayIndex].w = 0.0f;
    }
    
    

    float3 totalSHOutput = decodeFromSphericalHarmonicCoefficients(
        normal,
        float3(1.0f, 1.0f, 1.0f),
        rayIndex
    );
    decodedSphericalHarmonicsTexture[rayIndex].xyz = totalSHOutput;
    decodedSphericalHarmonicsTexture[rayIndex].w = 1.0f;

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

    uint32_t iCurrIndex = uint32_t(defaultUniformBuffer.miFrame) * uint32_t(iNumSamples);

    float3 worldPosition = worldPositionTexture[screenCoord].xyz;

    float fJacobian = 1.0f;
    uint32_t iFrame = defaultUniformBuffer.miFrame;
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

        float4 neighborHitPosition = temporalHitPositionTexture[sampleCoord];
        if(neighborHitPosition.w <= 0.0f)
        {
            continue;
        }

        uint32_t iMesh = uint32_t(neighborHitPosition.w);
        uint32_t iMaterialIndex = materialID[iMesh] - 1;
        Material neighborMaterial = materialData[iMaterialIndex];
        float fNeighborLuminance = computeLuminance(neighborMaterial.mEmissive.xyz * 10.0f);

        ret.mRandomResult = nextRand(ret.mRandomResult.miSeed);
        float fRand = ret.mRandomResult.mfNum;

        float4 neighborReservoir = temporalReservoirTexture[sampleCoord];
        float fWeight = fNeighborLuminance * neighborReservoir.z * neighborReservoir.w;
        ret.mReservoir.x += fWeight;
        ret.mReservoir.z += neighborReservoir.z;
        float fWeightPct = fWeight / max(ret.mReservoir.x, 0.001f);
        if(fRand < fWeightPct)
        {
            ret.mReservoir.y = fNeighborLuminance;
            ret.mHitPosition = temporalHitPositionTexture[sampleCoord];
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
float computeLuminance(
    in float3 radiance)
{
    return dot(radiance, float3(0.2126f, 0.7152f, 0.0722f));
}

/*
**
*/
void encodeSphericalHarmonicsCoefficients(
    in float3 radiance,
    in float3 rayDirection,
    in int2 screenCoord
)
{
    float fDstPct = 1.5f;
    float fSrcPct = 0.7f;

    float4 SHCoefficent0 = temporalSphericalHarmonicCoefficientTexture0[screenCoord] * fSrcPct;
    float4 SHCoefficent1 = temporalSphericalHarmonicCoefficientTexture1[screenCoord] * fSrcPct;
    float4 SHCoefficent2 = temporalSphericalHarmonicCoefficientTexture2[screenCoord] * fSrcPct;

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
    aResults[0] = radiance.xyz * coefficient.x * fDstPct;
    aResults[1] = radiance.xyz * coefficient.y * fDstPct;
    aResults[2] = radiance.xyz * coefficient.z * fDstPct;
    aResults[3] = radiance.xyz * coefficient.w * fDstPct;

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
    in int2 screenCoord
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