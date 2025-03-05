#define UINT32_MAX 0xffffffff
#define FLT_MAX  1.0e+10
#define PI 3.14159f

#include "default-uniform-headers.slang"

struct RandomResult 
{
    float mfNum;
    uint32_t miSeed;
};
struct RayPayload
{
    float4 mOutput;
    float4 mBarycentricCoord;
};
struct Ray
{
    float4 mOrigin;
    float4 mDirection;
    float4 mfT;
};
struct IntersectBVHResult
{
    float3 mHitPosition;
    float3 mHitNormal;
    float2 mHitUV;
    uint32_t miHitTriangle;
    float3 mBarycentricCoordinate;

    uint32_t miMaterialID;
    uint32_t miMesh;
};
struct TemporalRestirResult
{
    float4 mRadiance;
    float4 mReservoir;
    float4 mRayDirection;
    float4 mAmbientOcclusion;
    float4 mHitPosition;
    float4 mHitNormal;
    float4 mDirectSunLight;
    IntersectBVHResult mIntersectionResult;
    RandomResult mRandomResult;
    float mfNumValidSamples;

    float3 mCandidateRadiance;
    float4 mCandidateHitPosition;
    bool mbExchanged;

    uint32_t miIntersectionType;

    float4 mSphericalHarmonicCoefficient0;
    float4 mSphericalHarmonicCoefficient1;
    float4 mSphericalHarmonicCoefficient2;

    float2 mOnScreenUV;
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
struct SphericalHarmonicsResult
{
    float4 mCoefficients0;
    float4 mCoefficients1;
    float4 mCoefficients2;
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


[[vk::binding(0, 0)]] RaytracingAccelerationStructure scene;

[[vk::binding(1, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(2, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(3, 0)]] Texture2D<float4> texCoordTexture;

[[vk::binding(4, 0)]] Texture2D<float4> prevTemporalRadianceTexture;
[[vk::binding(5, 0)]] Texture2D<float4> prevTemporalReservoirTexture;
[[vk::binding(6, 0)]] Texture2D<float4> prevTemporalHitPositionTexture;
[[vk::binding(7, 0)]] Texture2D<float4> prevTemporalHitNormalTexture;

[[vk::binding(8, 0)]] Texture2D<float4> prevWorldPositionTexture;
[[vk::binding(9, 0)]] Texture2D<float4> prevNormalTexture;
[[vk::binding(10, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(11, 0)]] Texture2D<float4> prevMotionVectorTexture;

[[vk::binding(12, 0)]] Texture2D<float4> prevSphericalHarmonicCoefficientTexture0;
[[vk::binding(13, 0)]] Texture2D<float4> prevSphericalHarmonicCoefficientTexture1;
[[vk::binding(14, 0)]] Texture2D<float4> prevSphericalHarmonicCoefficientTexture2;

[[vk::binding(15, 0)]] Texture2D<float4> albedoTexture;

[[vk::binding(16, 0)]] Texture2D<float4> sampleHitPositionTexture;
[[vk::binding(17, 0)]] Texture2D<float4> sampleHitNormalTexture;
[[vk::binding(18, 0)]] Texture2D<float4> sampleHitUVMeshTriangleTexture;

[[vk::binding(19, 0)]] Texture2D<float4> hitPositionTexture;

[[vk::binding(20, 0)]] SamplerState textureSampler;

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

[shader("fragment")]
PSOutput PSMain(VertexStageOutput in)
{
    PSOutput out;

    float fReservoirSize = 10.0f;

    uint2 textureSize;
    worldPositionTexture.GetDimensions(
        textureSize.x,
        textureSize.y);
    float2 uv = float2(
        in.texCoord.x,
        in.texCoord.y
    );

    RandomResult randomResult = initRand(
        uint32_t(uv.x * 100.0f + uv.y * 200.0f) + uint32_t(defaultUniformBuffer.mfRand0 * 100.0f),
        uint32_t(uv.y * 10.0f + uv.y * 20.0f) + uint32_t(defaultUniformBuffer.mfRand1 * 100.0f),
        10u
    );

    float4 worldPosition = worldPositionTexture.Sample(textureSampler, uv);
    float3 normal = normalTexture.Sample(textureSampler, uv).xyz;

    int32_t iFrame = defaultUniformBuffer.miFrame;

    uint2 blueNoiseTextureSize;
    blueNoiseTexture.GetDimensions(
        blueNoiseTextureSize.x,
        blueNoiseTextureSize.y
    );

    if (worldPosition.w <= 0.0f)
    {
        out.mRadiance = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out.mReservoir = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out.mHitPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out.mHitNormal = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out.mSphericalHarmonic0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out.mSphericalHarmonic1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out.mSphericalHarmonic2 = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out.mDecodedSphericalHarmonic = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return out;
    }

    float fCenterDepth = fract(worldPosition.w);
    int32_t iCenterMeshID = int32_t(worldPosition.w - fCenterDepth);

    float2 motionVector = motionVectorTexture.Sample(textureSampler, uv).xy;
    motionVector = motionVector * 2.0f - 1.0f;

    // check for disocclusion for previous history pixel
    float2 prevScreenUV = uv - motionVector;
    bool bOutOfBounds = (
        prevScreenUV.x < 0.0f || prevScreenUV.x > 1.0f ||
                         prevScreenUV.y < 0.0f || prevScreenUV.y > 1.0f
    );
    bool bDisoccluded = isDisoccluded(prevScreenUV, uv);
    float fDisocclusion = (bOutOfBounds || bDisoccluded) ? 1.0f : 0.0f;
    float fValidHistory = 1.0f - fDisocclusion;

    float fClearReservoir = 1.0f - defaultUniformBuffer.mfClearReservoirs;

    uint2 prevScreenCoord = uint2(
        uint32_t(floor(prevScreenUV.x * float(textureSize.x) + 0.5f)),
        uint32_t(floor(prevScreenUV.y * float(textureSize.y) + 0.5f)),
    );

    TemporalRestirResult result;
    result.mReservoir = prevTemporalReservoirTexture.Sample(textureSampler, prevScreenUV) * fValidHistory * fClearReservoir;
    result.mRadiance = prevTemporalRadianceTexture.Sample(textureSampler, prevScreenUV) * fValidHistory * fClearReservoir;
    result.mHitPosition = prevTemporalHitPositionTexture.Sample(textureSampler, prevScreenUV) * fValidHistory * fClearReservoir;
    result.mHitNormal = prevTemporalHitNormalTexture.Sample(textureSampler, prevScreenUV) * fValidHistory * fClearReservoir;
    result.mIntersectionResult.miHitTriangle = UINT32_MAX;
    result.mfNumValidSamples = 0.0f;
    result.mbExchanged = false;
    result.mOnScreenUV = float2(0.0f, 0.0f);
    result.miIntersectionType = 0;

    // more samples for disoccluded pixel
    int32_t iNumCenterSamples = 1;
    if (fDisocclusion >= 1.0f || result.mReservoir.z <= 4.0f)
    {
        iNumCenterSamples = 4;
    }

    float fNumValidSamples = 0.0f;

    uint2 blueNoiseScreenCoord = uint2(
        uint32_t(uv.x * float(blueNoiseTextureSize.x)),
        uint32_t(uv.y * float(blueNoiseTextureSize.y))
    );

    // blue noise tile coordinate
    uint32_t iTileSize = 32u;
    uint32_t iNumTilesPerRow = blueNoiseTextureSize.x / iTileSize;
    uint32_t iNumTotalTiles = iNumTilesPerRow * iNumTilesPerRow;
    uint32_t iCurrIndex = uint32_t(defaultUniformBuffer.miFrame) * uint32_t(iNumCenterSamples);

    SphericalHarmonicsResult sphericalHarmonicsResult;
    sphericalHarmonicsResult.mCoefficients0 = prevSphericalHarmonicCoefficientTexture0.Sample(textureSampler, prevScreenUV) * fClearReservoir;
    sphericalHarmonicsResult.mCoefficients1 = prevSphericalHarmonicCoefficientTexture1.Sample(textureSampler, prevScreenUV) * fClearReservoir;
    sphericalHarmonicsResult.mCoefficients2 = prevSphericalHarmonicCoefficientTexture2.Sample(textureSampler, prevScreenUV) * fClearReservoir;

    // center pixel sample
    float3 sampleRayDirection = float3(0.0f, 0.0f, 0.0f);
    bool bRayTrace = true;
    float2 prevScreenUVCopy = prevScreenUV;
    for (int32_t iSample = 0; iSample < iNumCenterSamples; iSample++)
    {
        uint32_t iTileX = (iCurrIndex + uint32_t(iNumCenterSamples)) % iNumTilesPerRow;
        uint32_t iTileY = ((iCurrIndex + uint32_t(iNumCenterSamples)) / iNumTilesPerRow) % (iNumTilesPerRow * iNumTilesPerRow);
        uint32_t iTileOffsetX = (iCurrIndex + uint32_t(iNumCenterSamples)) % iTileSize;
        uint32_t iTileOffsetY = ((iCurrIndex + uint32_t(iNumCenterSamples)) / iTileSize) % (iTileSize * iTileSize);
        uint32_t iOffsetX = iTileOffsetX + iTileX * iTileSize;
        uint32_t iOffsetY = iTileOffsetY + iTileY * iTileSize;
        blueNoiseScreenCoord.x = (blueNoiseScreenCoord.x + iOffsetX) % blueNoiseTextureSize.x;
        blueNoiseScreenCoord.y = (blueNoiseScreenCoord.y + iOffsetY) % blueNoiseTextureSize.y;

        float3 blueNoise = blueNoiseTexture.Sample(textureSampler, uv).xyz;
        Ray ray = uniformSampling(
            worldPosition.xyz,
            normal.xyz,
            blueNoise.x,
            blueNoise.y);
        sampleRayDirection = ray.mDirection.xyz;
        result.mIntersectionResult.miHitTriangle = UINT32_MAX;

        result = temporalRestir(
            result,

            worldPosition.xyz,
            normal,
            uv,
            (iSample == 0) ? prevScreenUVCopy : uv,
            sampleRayDirection,

            fReservoirSize,
            1.0f,
            randomResult,
            0u,
            0.1f,
            textureSize,
            (fDisocclusion > 0.0f),
            sphericalHarmonicsResult
        );

        sphericalHarmonicsResult.mCoefficients0 = result.mSphericalHarmonicCoefficient0;
        sphericalHarmonicsResult.mCoefficients1 = result.mSphericalHarmonicCoefficient1;
        sphericalHarmonicsResult.mCoefficients2 = result.mSphericalHarmonicCoefficient2;
    }

    result = permutationRestir(
        result,
        uv,
        prevScreenUV,
        worldPosition.xyz,
        normal.xyz,
        iCenterMeshID,
        fReservoirSize,
        textureSize,
        randomResult,
        sphericalHarmonicsResult
    );

    sphericalHarmonicsResult.mCoefficients0 = result.mSphericalHarmonicCoefficient0;
    sphericalHarmonicsResult.mCoefficients1 = result.mSphericalHarmonicCoefficient1;
    sphericalHarmonicsResult.mCoefficients2 = result.mSphericalHarmonicCoefficient2;

    result.mReservoir.w = 1.0f; // clamp(result.mReservoir.x / max(result.mReservoir.z * result.mReservoir.y, 0.001f), 0.0f, 1.0f);

    out.mRadiance = result.mRadiance * result.mReservoir.w;
    out.mReservoir = result.mReservoir;
    out.mHitPosition = result.mHitPosition;
    out.mHitNormal = result.mHitNormal;

    out.mSphericalHarmonic0 = result.mSphericalHarmonicCoefficient0;
    out.mSphericalHarmonic1 = result.mSphericalHarmonicCoefficient1;
    out.mSphericalHarmonic2 = result.mSphericalHarmonicCoefficient2;

    float3 shOutput = decodeFromSphericalHarmonicCoefficients(
        normal.xyz,
        float3(10.0f, 10.0f, 10.0f),
        uv,
        sphericalHarmonicsResult
    );
    out.mDecodedSphericalHarmonic = float4(shOutput.xyz, 1.0f);

    return out;
}

/*
**
*/
TemporalRestirResult temporalRestir(
    in TemporalRestirResult prevResult,

    in float3 worldPosition,
    in float3 normal,
    in float2 inputTexCoord,
    in float2 prevInputTexCoord,
    in float3 rayDirection,

    in float fMaxTemporalReservoirSamples,
    in float fJacobian,

    in RandomResult randomResult,

    in uint32_t iSampleIndex,
    in float fM,
    in uint2 textureSize,
    in bool bDisoccluded,
    in SphericalHarmonicsResult prevSphericalHarmonicsResult)
{
    float fOneOverPDF = 1.0f; //1.0f / PI;

    TemporalRestirResult ret = prevResult;
    ret.mOnScreenUV = float2(0.0f, 0.0f);

    uint2 screenCoord = uint2(
        uint32_t(floor(inputTexCoord.x * float(textureSize.x)) + 0.5f),
        uint32_t(floor(inputTexCoord.y * float(textureSize.y)) + 0.5f)
    );

    ret.mRandomResult = randomResult;

    ret.mRandomResult = nextRand(ret.mRandomResult.miSeed);
    float fRand0 = ret.mRandomResult.mfNum;
    ret.mRandomResult = nextRand(ret.mRandomResult.miSeed);
    float fRand1 = ret.mRandomResult.mfNum;
    ret.mRandomResult = nextRand(ret.mRandomResult.miSeed);
    float fRand2 = ret.mRandomResult.mfNum;

    IntersectBVHResult intersectionInfo;

    float4 candidateHitPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 candidateHitNormal = float4(0.0f, 0.0f, 0.0f, 1.0f);
    intersectionInfo.miHitTriangle = UINT32_MAX;

    float4 hitPosition = sampleHitPositionTexture.Sample(
        textureSampler,
        inputTexCoord
    );
    float4 hitNormal = sampleHitNormalTexture.Sample(
        textureSampler,
        inputTexCoord
    );
    float4 hitUVMeshTriangle = sampleHitUVMeshTriangleTexture.Sample(
        textureSampler,
        inputTexCoord
    );

    candidateHitPosition.xyz = intersectionInfo.mHitPosition.xyz;
    candidateHitPosition.w = hitPosition.w;
    candidateHitNormal.xyz = intersectionInfo.mHitNormal.xyz;

    float3 sampleRayDirection = hitPosition.xyz - worldPosition.xyz;
    float3 candidateRayDirection = normalize(sampleRayDirection);

    SphericalHarmonicsResult sphericalHarmonicsResult = prevSphericalHarmonicsResult;
    ret.mSphericalHarmonicCoefficient0 = sphericalHarmonicsResult.mCoefficients0;
    ret.mSphericalHarmonicCoefficient1 = sphericalHarmonicsResult.mCoefficients1;
    ret.mSphericalHarmonicCoefficient2 = sphericalHarmonicsResult.mCoefficients2;

    float4 candidateRadiance = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if(hitPosition.w > 0.0f)
    {
        intersectionInfo.miMesh = uint32_t(hitUVMeshTriangle.z);
        uint32_t iMeshOffset = meshRanges[intersectionInfo.miMesh].miStart;
        intersectionInfo.miMaterialID = materialID[intersectionInfo.miMesh];
        Material material = materialData[intersectionInfo.miMaterialID-1];

        candidateRadiance.xyz = material.mEmissive.xyz * defaultUniformBuffer.mfEmissiveRadiancePct;
        candidateHitPosition.w = hitUVMeshTriangle.z;

        float fRadianceDP = max(dot(normalize(candidateRayDirection), normal), 0.0f);
        float fDistanceAttenuation = 1.0f / max(dot(sampleRayDirection, sampleRayDirection), 1.0f);
        candidateRadiance.xyz =
            candidateRadiance.xyz *
            fJacobian *
            fRadianceDP *
            fDistanceAttenuation *
            fOneOverPDF;

        sphericalHarmonicsResult = encodeSphericalHarmonics(
            sphericalHarmonicsResult,
            candidateRadiance.xyz,
            candidateRayDirection.xyz
        );

        ret.mSphericalHarmonicCoefficient0 = sphericalHarmonicsResult.mCoefficients0;
        ret.mSphericalHarmonicCoefficient1 = sphericalHarmonicsResult.mCoefficients1;
        ret.mSphericalHarmonicCoefficient2 = sphericalHarmonicsResult.mCoefficients2;
    }
    else
    {
        candidateHitPosition.w = 0.0f;
    }

    

#if 0
    // encode spherical harmonics
    {
        float fDstPct = 1.5f;
        float fSrcPct = 0.7f;

        float4 SHCoefficent0 = prevSphericalHarmonicCoefficientTexture0.Sample(
            textureSampler,
            inputTexCoord
        ) * fSrcPct;
        float4 SHCoefficent1 = prevSphericalHarmonicCoefficientTexture1.Sample(
            textureSampler,
            inputTexCoord
        ) * fSrcPct;
        float4 SHCoefficent2 = prevSphericalHarmonicCoefficientTexture2.Sample(
            textureSampler,
            inputTexCoord
        ) * fSrcPct;

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
            afC.y * candidateRayDirection.y * A.y,
            afC.z * candidateRayDirection.z * A.z,
            afC.w * candidateRayDirection.x * A.w
        );

        // encode with radiance
        float3 aResults[4];
        aResults[0] = candidateRadiance.xyz * coefficient.x * fDstPct;
        aResults[1] = candidateRadiance.xyz * coefficient.y * fDstPct;
        aResults[2] = candidateRadiance.xyz * coefficient.z * fDstPct;
        aResults[3] = candidateRadiance.xyz * coefficient.w * fDstPct;
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

        ret.mSphericalHarmonicCoefficient0 = SHCoefficent0;
        ret.mSphericalHarmonicCoefficient1 = SHCoefficent1;
        ret.mSphericalHarmonicCoefficient2 = SHCoefficent2;
    }
#endif // #if 0

    // reservoir
    float fLuminance = computeLuminance(
        candidateRadiance.xyz
    );

    float fPHat = clamp(fLuminance, 0.0f, 1.0f);
    ret.mbExchanged = false;

    ret.mReservoir.x += fPHat;
    ret.mReservoir.z += fM;
    float fWeightPct = fPHat / ret.mReservoir.x;
    if (fRand2 < fWeightPct || ret.mReservoir.z <= 0.0f)
    {
        ret.mReservoir.y = fPHat;
        ret.mRadiance = candidateRadiance;
        ret.mHitPosition = candidateHitPosition;
        ret.mHitNormal = candidateHitNormal;
        ret.mRayDirection = float4(candidateRayDirection, 1.0f);
        ret.mbExchanged = true;
    }
    else
    {
        ret.miIntersectionType = 0;
    }

    // clamp reservoir
    if (ret.mReservoir.z > fMaxTemporalReservoirSamples)
    {
        float fPct = fMaxTemporalReservoirSamples / ret.mReservoir.z;
        ret.mReservoir.x *= fPct;
        ret.mReservoir.z = fMaxTemporalReservoirSamples;
    }

    ret.mCandidateRadiance = candidateRadiance.xyz;
    ret.mCandidateHitPosition = candidateHitPosition;
    ret.mIntersectionResult = intersectionInfo;
    ret.mfNumValidSamples += fM * float(fLuminance > 0.0f);

    return ret;
}

/*
**
*/
RandomResult initRand(
    uint32_t val0,
    uint32_t val1,
    uint32_t backoff)
{
    RandomResult retResult;

    uint32_t v0 = val0;
    uint32_t v1 = val1;
    uint32_t s0 = 0u;

    for (uint32_t n = 0; n < backoff; n++)
    {
        s0 += uint32_t(0x9e3779b9);
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    retResult.miSeed = v0;
    retResult.mfNum = 0.0f;

    return retResult;
}

/*
**
*/
RandomResult nextRand(
    in uint32_t s)
{
    RandomResult retResult;

    uint32_t sCopy = s;
    sCopy = (1664525u * sCopy + 1013904223u);
    retResult.mfNum = float(sCopy & 0x00FFFFFF) / float(0x01000000);
    retResult.miSeed = sCopy;

    return retResult;
}

/*
**
*/
Ray uniformSampling(
    in float3 worldPosition,
    in float3 normal,
    in float fRand0,
    in float fRand1)
{
    float fPhi = 2.0f * PI * fRand0;
    float fCosTheta = 1.0f - fRand1;
    float fSinTheta = sqrt(1.0f - fCosTheta * fCosTheta);
    float3 h = float3(
        cos(fPhi) * fSinTheta,
        sin(fPhi) * fSinTheta,
        fCosTheta);

    float3 up = float3(0.0f, 1.0f, 0.0f);
    if (abs(normal.y) > 0.999f)
    {
        up = float3(1.0f, 0.0f, 0.0f);
    }
    float3 tangent = normalize(cross(up, normal));
    float3 binormal = normalize(cross(normal, tangent));
    float3 rayDirection = normalize(tangent * h.x + binormal * h.y + normal * h.z);

    Ray ray;
    ray.mOrigin = float4(worldPosition, 1.0f);
    ray.mDirection = float4(rayDirection, 1.0f);
    ray.mfT = float4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

    return ray;
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
bool isPrevUVOutOfBoundsRayTrace(
    in uint2 inputScreenCoord,
    in uint2 screenSize)
{
    float2 inputTexCoord = inputScreenCoord / float2(screenSize.x, screenSize.y);
    float4 motionVector = motionVectorTexture[inputScreenCoord];
    float2 backProjectedScreenUV = inputTexCoord - motionVector.xy;

    return (backProjectedScreenUV.x < 0.0f || backProjectedScreenUV.x > 1.0 || backProjectedScreenUV.y < 0.0f || backProjectedScreenUV.y > 1.0f);
}

/*
**
*/
bool isDisoccluded(
    in float2 prevScreenUV,
    in float2 uv
)
{
    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);
    float3 worldPosition = worldPositionTexture.Sample(textureSampler, uv).xyz;

    float3 prevWorldPosition = prevWorldPositionTexture.Sample(textureSampler, prevScreenUV).xyz;
    float3 worldPositionDiff = worldPosition.xyz - prevWorldPosition;

    float3 normal = normalTexture.Sample(textureSampler, uv).xyz;
    float3 prevNormal = prevNormalTexture.Sample(textureSampler, prevScreenUV).xyz;
    float4 motionVector = motionVectorTexture.Sample(textureSampler, uv);
    float4 prevMotionVectorAndMeshIDAndDepth = prevMotionVectorTexture.Sample(textureSampler, prevScreenUV);

    float fDepth = fract(motionVector.z);
    float fPrevDepth = fract(prevMotionVectorAndMeshIDAndDepth.z);

    uint32_t iMesh = uint32_t(motionVector.z - fDepth);
    uint32_t iPrevMesh = uint32_t(prevMotionVectorAndMeshIDAndDepth.z - fPrevDepth);

    float fDP = max(dot(normal, prevNormal), 0.0f);
    float fLength = dot(worldPositionDiff, worldPositionDiff);
    bool bDisoccluded = (fLength > 0.01f || /*fDP < 0.8f || */ iMesh != iPrevMesh);

    return bDisoccluded;
}

/*
**
*/
float3 decodeFromSphericalHarmonicCoefficients(
    in float3 direction,
    in float3 maxRadiance,
    in float2 texCoord,
    in SphericalHarmonicsResult sphericalHarmonicsResult
)
{
    float4 SHCoefficent0 = sphericalHarmonicsResult.mCoefficients0;
    float4 SHCoefficent1 = sphericalHarmonicsResult.mCoefficients1;
    float4 SHCoefficent2 = sphericalHarmonicsResult.mCoefficients2;

    float3 aTotalCoefficients[4];
    float fFactor = 1.0f;

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
    decoded = clamp(decoded, float3(0.0f, 0.0f, 0.0f), maxRadiance);

    return decoded;
}

/*
**
*/
TemporalRestirResult permutationRestir(
    in TemporalRestirResult result,
    in float2 uv,
    in float2 prevUV,
    in float3 worldPosition,
    in float3 normal,
    in uint32_t iCenterMeshID,
    in float fReservoirSize,
    in uint2 textureSize,
    in RandomResult randomResult,
    in SphericalHarmonicsResult sphericalHarmonicsResult
)
{
    TemporalRestirResult prevResult = result;
    result.mRandomResult = randomResult;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    int2 origScreenCoord = int2(
        int32_t(uv.x * float(textureSize.x)),
        int32_t(uv.y * float(textureSize.y))
    );

    int2 prevScreenCoord = int2(
        int32_t(prevUV.x * float(textureSize.x)),
        int32_t(prevUV.y * float(textureSize.y))
    );

    // permutation samples
    int32_t iNumPermutations = 5;
    for (int32_t iSample = 1; iSample < iNumPermutations; iSample++)
    {
        uint2 aXOR[4];
        aXOR[0] = int2(3, 3);
        aXOR[1] = int2(2, 1);
        aXOR[2] = int2(1, 2);
        aXOR[3] = int2(3, 3);

        int2 aOffsets[4];
        aOffsets[0] = int2(-1, -1);
        aOffsets[1] = int2(1, 1);
        aOffsets[2] = int2(-1, 1);
        aOffsets[3] = int2(1, -1);

        // apply permutation offset to screen coordinate, converting to uv after
        int32_t iFrame = int32_t(defaultUniformBuffer.miFrame);
        int32_t iIndex0 = iFrame & 3;
        int32_t iIndex1 = (iSample + (iFrame ^ 1)) & 3;
        int2 offset = aOffsets[iIndex0] + aOffsets[iIndex1];
        int2 screenCoord = (origScreenCoord + offset) ^ aXOR[iFrame & 3];

        float3 sampleRayDirection = float3(0.0f, 0.0f, 0.0f);
        Ray ray;

        // permutation uv
        float2 sampleUV = float2(
            ceil(float(screenCoord.x) + 0.5f) / float(defaultUniformBuffer.miScreenWidth),
            ceil(float(screenCoord.y) + 0.5f) / float(defaultUniformBuffer.miScreenHeight));

        // get sample world position, normal, and ray direction
        float fJacobian = 1.0f;
        {
            // sample world position
            float4 sampleWorldPosition = prevWorldPositionTexture.Sample(textureSampler, prevUV);
            float3 sampleNormal = prevNormalTexture.Sample(textureSampler, prevUV).xyz;

            // neighbor normal difference check
            float fDP = dot(sampleNormal, normal);
            if (fDP <= 0.6f)
            {
                continue;
            }

            // neightbor depth difference check
            float fSampleDepth = fract(sampleWorldPosition.w);

            // mesh id difference check
            int32_t iSampleMeshID = int32_t(floor((sampleWorldPosition.w - fSampleDepth) + 0.5f));
            if (iSampleMeshID != iCenterMeshID)
            {
                continue;
            }

            // hit point and hit normal for jacobian
            float3 sampleHitPoint = prevTemporalHitPositionTexture.Sample(textureSampler, prevUV).xyz;
            float3 neighborHitNormal = prevTemporalHitNormalTexture.Sample(textureSampler, prevUV).xyz;
            float3 centerToNeighborHitPointUnNormalized = sampleHitPoint - worldPosition.xyz;
            float3 neighborToNeighborHitPointUnNormalized = sampleHitPoint - sampleWorldPosition.xyz;
            float3 centerToNeighborHitPointNormalized = normalize(centerToNeighborHitPointUnNormalized);
            float3 neighborToNeighborHitPointNormalized = normalize(neighborToNeighborHitPointUnNormalized);

            // compare normals for jacobian
            float fDP0 = max(dot(neighborHitNormal, centerToNeighborHitPointNormalized * -1.0f), 0.0f);
            float fDP1 = max(dot(neighborHitNormal, neighborToNeighborHitPointNormalized * -1.0f), 1.0e-4f);
            fJacobian = fDP0 / fDP1;

            // compare length for jacobian
            float fCenterToHitPointLength = length(centerToNeighborHitPointUnNormalized);
            float fNeighborToHitPointLength = length(neighborToNeighborHitPointUnNormalized);
            fJacobian *= ((fCenterToHitPointLength * fCenterToHitPointLength) / (fNeighborToHitPointLength * fNeighborToHitPointLength));
            fJacobian = clamp(fJacobian, 0.0f, 1.0f);

            sampleRayDirection = centerToNeighborHitPointNormalized;
        }

        result.mIntersectionResult.miHitTriangle = UINT32_MAX;
        TemporalRestirResult ret = temporalRestir(
            prevResult,

            worldPosition.xyz,
            normal,
            uv,
            prevUV,
            sampleRayDirection,

            fReservoirSize,
            fJacobian,
            randomResult,
            uint32_t(iSample),
            0.1f,
            textureSize,
            false,
            sphericalHarmonicsResult
        );

        prevResult = ret;

    } // for sample = 0 to num permutation samples

    return prevResult;
}


/*
**
*/
SphericalHarmonicsResult encodeSphericalHarmonics(
    in SphericalHarmonicsResult result,
    in float3 radiance,
    in float3 rayDirection
)
{
    SphericalHarmonicsResult ret = result;

    float fDstPct = 1.5f;
    float fSrcPct = 0.9f;
    //float fDstPct = 1.0f;
    //float fSrcPct = 1.0f;

    float4 SHCoefficent0 = result.mCoefficients0 * fSrcPct;
    float4 SHCoefficent1 = result.mCoefficients1 * fSrcPct;
    float4 SHCoefficent2 = result.mCoefficients2 * fSrcPct;

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

    ret.mCoefficients0 = SHCoefficent0;
    ret.mCoefficients1 = SHCoefficent1;
    ret.mCoefficients2 = SHCoefficent2;

    return ret;
}