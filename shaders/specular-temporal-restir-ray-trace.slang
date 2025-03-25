#define UINT32_MAX 0xffffffff
#define FLT_MAX  1.0e+10
#define PI 3.14159f
#define PROBE_IMAGE_SIZE 8u
#define VALIDATION_STEP 16u

#include "default-uniform-headers.slang"
#include "murmur-hash-headers.slang"
#include "octahedron-map-headers.slang"
#include "irradiance-cache-headers.slang"
//#include "motion-vector-headers.slang"

#include "bounced-radiance-headers.slang"

struct RayPayload
{
    float4 mOutput;
    float4 mBarycentricCoord;
};

struct RandomResult {
    float mfNum;
    uint32_t miSeed;
};

struct Ray
{
    float4 mOrigin;
    float4 mDirection;
    float4 mfT;
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
    float4 mCandidateHitNormal;
    float2 mCandidateHitUV;
    bool mbExchanged;

    uint32_t miIntersectionType;

    float2 mOnScreenUV;

    uint32_t miCandidateHitMesh;
    uint32_t miCandidateHitTriangle;
};

struct ReservoirResult
{
    float4 mReservoir;
    bool mbExchanged;
};

struct DisocclusionResult
{
    int2 mBackProjectScreenCoord;
    bool mbDisoccluded;
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

struct OnScreenRadianceResult
{
    float3              mRadiance;
    bool                mbOnScreen;
};

struct MeshRange
{
    uint32_t miStart;
    uint32_t miEnd;
};

[[vk::binding(0, 0)]] RWTexture2D<float4> radianceOutput;
[[vk::binding(1, 0)]] RWTexture2D<float4> reservoirOutput;
[[vk::binding(2, 0)]] RWTexture2D<float4> hitPositionOutput;
[[vk::binding(3, 0)]] RWTexture2D<float4> hitNormalOutput;
[[vk::binding(4, 0)]] RWTexture2D<float4> sampleRayHitPositionOutput;
[[vk::binding(5, 0)]] RWTexture2D<float4> sampleRayHitNormalOutput;
[[vk::binding(6, 0)]] RWTexture2D<float4> sampleRayHitUVMeshTriangle;

[[vk::binding(7, 0)]] RWTexture2D<float4> sphericalHarmonicCoefficientTexture0;
[[vk::binding(8, 0)]] RWTexture2D<float4> sphericalHarmonicCoefficientTexture1;
[[vk::binding(9, 0)]] RWTexture2D<float4> sphericalHarmonicCoefficientTexture2;

[[vk::binding(10, 0)]] RWTexture2D<float4> decodedSphericalHarmonicsOutput;

[[vk::binding(11, 0)]] RaytracingAccelerationStructure scene;
[[vk::binding(12, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(13, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(14, 0)]] Texture2D<float4> texCoordTexture;
[[vk::binding(15, 0)]] Texture2D<float4> skyTexture;
[[vk::binding(16, 0)]] Texture2D<float4> prevTemporalReservoirTexture;
[[vk::binding(17, 0)]] Texture2D<float4> prevTemporalRadianceTexture;
[[vk::binding(18, 0)]] Texture2D<float4> prevTemporalHitPositionTexture;
[[vk::binding(19, 0)]] Texture2D<float4> prevTemporalHitNormalTexture;
[[vk::binding(20, 0)]] Texture2D<float4> prevWorldPositionTexture;
[[vk::binding(21, 0)]] Texture2D<float4> prevNormalTexture;
[[vk::binding(22, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(23, 0)]] Texture2D<float4> prevMotionVectorTexture;
[[vk::binding(24, 0)]] Texture2D<float4> directSunOutputTexture;
[[vk::binding(25, 0)]] Texture2D<float4> previousAmbientOcclusionTexture;

[[vk::binding(26, 0)]] Texture2D<float4> prevSphericalHarmonicCoefficientTexture0;
[[vk::binding(27, 0)]] Texture2D<float4> prevSphericalHarmonicCoefficientTexture1;
[[vk::binding(28, 0)]] Texture2D<float4> prevSphericalHarmonicCoefficientTexture2;

[[vk::binding(29, 0)]] StructuredBuffer<IrradianceCacheEntry> irradianceCache;

[[vk::binding(30, 0)]] Texture2D<float4> albedoTexture;

[[vk::binding(31, 0)]] Texture2D<float4> screenMotionVectorTexture;
[[vk::binding(32, 0)]] Texture2D<float4> initialTextureAtlas;

[[vk::binding(33, 0)]] Texture2D<float4> sunLightTexture;

[[vk::binding(34, 0)]] RWTexture2D<float4> debugTexture0;
[[vk::binding(35, 0)]] RWTexture2D<float4> debugTexture1;
[[vk::binding(36, 0)]] RWTexture2D<float4> debugTexture2;
[[vk::binding(37, 0)]] RWTexture2D<float4> debugTexture3;

[[vk::binding(38, 0)]] SamplerState textureSampler;

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
    float fReservoirSize = 10.0f;

    uint2 rayIndex = DispatchRaysIndex().xy;

    debugTexture0[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    debugTexture1[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    debugTexture2[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    debugTexture3[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);

    //sphericalHarmonicCoefficientTexture0[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    //sphericalHarmonicCoefficientTexture1[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    //sphericalHarmonicCoefficientTexture2[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if(defaultUniformBuffer.miFrame < 1)
    {
        radianceOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        reservoirOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        hitPositionOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        hitNormalOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        sampleRayHitPositionOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        sampleRayHitNormalOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        sampleRayHitUVMeshTriangle[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    uint2 textureSize;
    worldPositionTexture.GetDimensions(
        textureSize.x,
        textureSize.y);
    float2 uv = float2(
        float(rayIndex.x) / float(textureSize.x),
        float(rayIndex.y) / float(textureSize.y) 
    );

    RandomResult randomResult = initRand(
        uint32_t(uv.x * 100.0f + rayIndex.y * 200.0f) + uint32_t(defaultUniformBuffer.mfRand0 * 100.0f),
        uint32_t(uv.y * 10.0f + rayIndex.y * 20.0f) + uint32_t(defaultUniformBuffer.mfRand1 * 100.0f),
        10u);

    float4 worldPosition = worldPositionTexture[rayIndex];
    float3 normal = normalTexture[rayIndex].xyz;

    int32_t iFrame = defaultUniformBuffer.miFrame;

    uint2 blueNoiseTextureSize;
    blueNoiseTexture.GetDimensions(
        blueNoiseTextureSize.x,
        blueNoiseTextureSize.y
    );
    uint32_t iOffsetX = uint32_t(iFrame) % blueNoiseTextureSize.x;
    uint32_t iOffsetY = (uint32_t(iFrame) / blueNoiseTextureSize.y) % blueNoiseTextureSize.y;
    uint2 blueNoiseSampleScreenCoord = uint2(
        (rayIndex.x + iOffsetX) % textureSize.x,
        (rayIndex.y + iOffsetY) % textureSize.y
    );

    if(worldPosition.w <= 0.0f)
    {
        radianceOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        reservoirOutput[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    float fCenterDepth = fract(worldPosition.w);
    int32_t iCenterMeshID = int32_t(worldPosition.w - fCenterDepth);

    float2 motionVector = motionVectorTexture[rayIndex].xy;
    motionVector = motionVector * 2.0f - 1.0f;

    int2 outputTextureSize;
    radianceOutput.GetDimensions(outputTextureSize.x, outputTextureSize.y);
    float fOneOverWidth = 1.0f / float(outputTextureSize.x);
    float fOneOverHeight = 1.0f / float(outputTextureSize.y);

    // check for disocclusion for previous history pixel
    float2 prevScreenUV = uv - motionVector;
    bool bOutOfBounds = (
        prevScreenUV.x < 0.0f || prevScreenUV.x > 1.0f ||
        prevScreenUV.y < 0.0f || prevScreenUV.y > 1.0f
    );
    bool bDisoccluded = isDisoccluded(prevScreenUV, rayIndex);
    float fDisocclusion = (bOutOfBounds || bDisoccluded) ? 1.0f : 0.0f;
    float fValidHistory = 1.0f - fDisocclusion;

    uint2 prevScreenCoord = uint2(
        uint32_t(floor(prevScreenUV.x * float(textureSize.x) + 0.5f)),
        uint32_t(floor(prevScreenUV.y * float(textureSize.y) + 0.5f)),
    );

    TemporalRestirResult result;
    result.mReservoir = prevTemporalReservoirTexture[prevScreenCoord] * fValidHistory;
    result.mRadiance = prevTemporalRadianceTexture[prevScreenCoord] * fValidHistory;
    result.mHitPosition = prevTemporalHitPositionTexture[prevScreenCoord] * fValidHistory;
    result.mHitNormal = prevTemporalHitNormalTexture[prevScreenCoord] * fValidHistory;
    result.mAmbientOcclusion = previousAmbientOcclusionTexture[prevScreenCoord] * fValidHistory;
    result.mIntersectionResult.miHitTriangle = UINT32_MAX;
    result.mfNumValidSamples = 0.0f;
    result.mbExchanged = false;
    result.mOnScreenUV = float2(0.0f, 0.0f);
    result.miIntersectionType = 0;

    // update every 3 frames
    if(result.mReservoir.z >= 6.0f && (defaultUniformBuffer.miFrame % 3) == 1 && fDisocclusion <= 0.0f)
    {
        //return;
    }

    // more samples for disoccluded pixel
    int32_t iNumCenterSamples = 1;
    if(fDisocclusion >= 1.0f || result.mReservoir.z <= 4.0f)
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

    if(fDisocclusion > 0.0f)
    {
        result.mAmbientOcclusion = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    int32_t iMaterialID = max(materialID[iCenterMeshID] - 1, 0);
    float3 viewDirection = normalize(defaultUniformBuffer.mCameraPosition.xyz - worldPosition.xyz);

    float fRoughness = materialData[iMaterialID].mSpecular.x;
    float fMetalness = max(materialData[iMaterialID].mSpecular.y, 0.1f);

    // center pixel sample
    float3 sampleRayDirection = float3(0.0f, 0.0f, 0.0f);
    bool bRayTrace = true;
    float2 prevScreenUVCopy = prevScreenUV;
    for(int32_t iSample = 0; iSample < iNumCenterSamples; iSample++)
    { 
        uint32_t iTileX = (iCurrIndex + uint32_t(iNumCenterSamples)) % iNumTilesPerRow;
        uint32_t iTileY = ((iCurrIndex + uint32_t(iNumCenterSamples)) / iNumTilesPerRow) % (iNumTilesPerRow * iNumTilesPerRow);
        uint32_t iTileOffsetX = (iCurrIndex + uint32_t(iNumCenterSamples)) % iTileSize;
        uint32_t iTileOffsetY = ((iCurrIndex + uint32_t(iNumCenterSamples)) / iTileSize) % (iTileSize * iTileSize);
        uint32_t iOffsetX = iTileOffsetX + iTileX * iTileSize;
        uint32_t iOffsetY = iTileOffsetY + iTileY * iTileSize;
        blueNoiseScreenCoord.x = (blueNoiseScreenCoord.x + iOffsetX) % blueNoiseTextureSize.x;
        blueNoiseScreenCoord.y = (blueNoiseScreenCoord.y + iOffsetY) % blueNoiseTextureSize.y;

        float3 blueNoise = blueNoiseTexture[blueNoiseScreenCoord].xyz;
        result.mIntersectionResult.miHitTriangle = UINT32_MAX;

        float3 sampleRayDirection = sampleGGXVNDF(
            viewDirection,
            fRoughness,
            blueNoise.x,
            blueNoise.y
        );

        //Ray ray = uniformSampling(
        //    worldPosition.xyz,
        //    normal.xyz,
        //    blueNoise.x,
        //    blueNoise.y);
        //sampleRayDirection = ray.mDirection.xyz;

        // restir
        result.mAmbientOcclusion.y += 1.0f;
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
            bRayTrace,
            textureSize,
            (fDisocclusion > 0.0f),
            fRoughness,
            fMetalness
        );
    }

    result.mAmbientOcclusion.y += 1;
    if (result.mIntersectionResult.miHitTriangle != UINT32_MAX)
    {
        result.mAmbientOcclusion.x += 1;
    }
    result.mReservoir.w = clamp(result.mReservoir.x / max(result.mReservoir.z * result.mReservoir.y, 0.001f), 0.0f, 1.0f);

    float fReservoirWeight = (result.mReservoir.z <= 3.0f) ? 1.0f : result.mReservoir.w;
    float fAO = 1.0f - (result.mAmbientOcclusion.x / result.mAmbientOcclusion.y);
    float fWeight = (fAO > fReservoirWeight) ? fAO : fReservoirWeight;

    radianceOutput[rayIndex] = result.mRadiance * fWeight;

    float4 prevSH0 = prevSphericalHarmonicCoefficientTexture0[prevScreenCoord];
    float4 prevSH1 = prevSphericalHarmonicCoefficientTexture1[prevScreenCoord];
    float4 prevSH2 = prevSphericalHarmonicCoefficientTexture2[prevScreenCoord];
    if (bDisoccluded)
    {
        prevSH0 = float4(0.0f);
        prevSH1 = float4(0.0f);
        prevSH2 = float4(0.0f);
    }

    float fDstPct = 0.2f;
    float fSrcPct = 1.0f - fDstPct;
    encodeToSphericalHarmonicCoefficients2(
        radianceOutput[rayIndex].xyz * 10.0f,
        normalize(result.mHitPosition.xyz - worldPosition.xyz),
        rayIndex,
        prevSH0,
        prevSH1,
        prevSH2,
        fSrcPct,
        fDstPct
    );

    reservoirOutput[rayIndex] = result.mReservoir;

    sampleRayHitPositionOutput[rayIndex] = result.mCandidateHitPosition;
    sampleRayHitNormalOutput[rayIndex] = result.mCandidateHitNormal;
    sampleRayHitUVMeshTriangle[rayIndex].xy = result.mCandidateHitUV;
    sampleRayHitUVMeshTriangle[rayIndex].z = float(result.miCandidateHitMesh);
    sampleRayHitUVMeshTriangle[rayIndex].w = float(result.miCandidateHitTriangle);

    hitPositionOutput[rayIndex] = result.mHitPosition;
    hitNormalOutput[rayIndex] = result.mHitNormal;

    float3 shOutput = decodeFromSphericalHarmonicCoefficients(
        normal.xyz,
        rayIndex
    );

    decodedSphericalHarmonicsOutput[rayIndex] = float4(shOutput.xyz, 1.0f);

    debugTexture0[rayIndex] = float4(fRoughness, fMetalness, 0.0f, 1.0f);
}

/////
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
    in bool bTraceRay,
    in uint2 textureSize,
    in bool bDisoccluded,
    in float fRoughness,
    in float fMetalness)
{
    float fOneOverPDF = 1.0f; // 1.0f / PI;

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

    uint2 inputCoord = uint2(
        uint32_t(inputTexCoord.x * float(textureSize.x)),
        uint32_t(inputTexCoord.y * float(textureSize.y))
    );
    uint2 prevInputCoord = uint2(
        uint32_t(prevInputTexCoord.x * float(textureSize.x)),
        uint32_t(prevInputTexCoord.y * float(textureSize.y))
    );
    
    IntersectBVHResult intersectionInfo;

    float4 candidateHitPosition = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 candidateHitNormal = float4(0.0f, 0.0f, 0.0f, 1.0f);
    intersectionInfo.miHitTriangle = UINT32_MAX;

    float3 emissiveRadiance = float3(0.0f, 0.0f, 0.0f);

    RayDesc ray;
    ray.Origin = worldPosition + rayDirection * 0.01f;
    ray.Direction = rayDirection;
    ray.TMin = 0.01f;
    ray.TMax = 1000.0f;
    RayPayload payload = {
        float4(0.0f, 0.0f, 0.0f, 0.0f)
    };
    if(bTraceRay)
    {
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
            
            uint32_t iMesh = uint32_t(payload.mOutput.z);
            uint32_t iMeshOffset = meshRanges[iMesh].miStart;

            intersectionInfo.miMesh = iMesh;

            uint32_t iTri0 = indexBuffer[iMeshOffset + intersectionInfo.miHitTriangle * 3];
            uint32_t iTri1 = indexBuffer[iMeshOffset + intersectionInfo.miHitTriangle * 3 + 1];
            uint32_t iTri2 = indexBuffer[iMeshOffset + intersectionInfo.miHitTriangle * 3 + 2];

            VertexFormat v0 = vertexBuffer[iTri0];
            VertexFormat v1 = vertexBuffer[iTri1];
            VertexFormat v2 = vertexBuffer[iTri2];

            intersectionInfo.mHitPosition =
                v0.mPosition.xyz * intersectionInfo.mBarycentricCoordinate.x +
                v1.mPosition.xyz * intersectionInfo.mBarycentricCoordinate.y +
                v2.mPosition.xyz * intersectionInfo.mBarycentricCoordinate.z;

            intersectionInfo.mHitNormal =
                v0.mNormal.xyz * intersectionInfo.mBarycentricCoordinate.x +
                v1.mNormal.xyz * intersectionInfo.mBarycentricCoordinate.y +
                v2.mNormal.xyz * intersectionInfo.mBarycentricCoordinate.z;

            intersectionInfo.mHitUV =
                v0.mTexCoord.xy * intersectionInfo.mBarycentricCoordinate.x +
                v1.mTexCoord.xy * intersectionInfo.mBarycentricCoordinate.y +
                v2.mTexCoord.xy * intersectionInfo.mBarycentricCoordinate.z;

            intersectionInfo.miMesh = uint32_t(payload.mOutput.z);
            intersectionInfo.miMaterialID = materialID[intersectionInfo.miMesh];
            candidateHitPosition.xyz = intersectionInfo.mHitPosition.xyz;
            candidateHitPosition.w = payload.mOutput.y;
            candidateHitNormal.xyz = intersectionInfo.mHitNormal.xyz;

            uint32_t iMaterialID = materialID[iMesh] - 1;
            Material material = materialData[iMaterialID];
            emissiveRadiance = material.mEmissive.xyz; 
        }
        else
        {
            // miss

            intersectionInfo.miHitTriangle = UINT32_MAX;
            intersectionInfo.mBarycentricCoordinate = float3(0.0f, 0.0f, 0.0f);
            intersectionInfo.mHitPosition.xyz = rayDirection.xyz * 1000.0f;
        }
    }
    else
    {
        candidateHitPosition = prevTemporalHitPositionTexture[inputCoord];
        candidateHitNormal = prevTemporalHitNormalTexture[inputCoord];

        intersectionInfo.miHitTriangle = uint32_t(floor(candidateHitPosition.w));
        if(length(candidateHitPosition.xyz) >= 1000.0f)
        {
            intersectionInfo.miHitTriangle = UINT32_MAX;
        }
    }

    float fReflectivity = 1.0f;

    float4 candidateRadiance = float4(0.0f, 0.0f, 0.0f, 1.0f);
    float4 candidateRayDirection = float4(rayDirection, 1.0f);
    float fRadianceDP = max(dot(normal, ray.Direction.xyz), 0.0f);
    float fDistanceAttenuation = 1.0f;
    float3 onScreenRadiance = float3(0.0f, 0.0f, 0.0f);
    if(intersectionInfo.miHitTriangle == UINT32_MAX)
    {
        // didn't hit anything, use skylight
        float2 skyUV = octahedronMap2(ray.Direction.xyz);
        uint2 skyTextureSize = uint2(0, 0);
        skyTexture.GetDimensions(skyTextureSize.x, skyTextureSize.y);
        uint2 skyCoord = uint2(
            uint32_t(skyUV.x * float(skyTextureSize.x)),
            uint32_t(skyUV.y * float(skyTextureSize.y))
        );

        candidateRadiance = skyTexture[skyCoord] + sunLightTexture[inputCoord];

        candidateHitNormal = float4(-ray.Direction.x, -ray.Direction.y, -ray.Direction.z, 1.0f);
        candidateHitPosition.x = worldPosition.x + ray.Direction.x * 50000.0f;
        candidateHitPosition.y = worldPosition.y + ray.Direction.y * 50000.0f;
        candidateHitPosition.z = worldPosition.z + ray.Direction.z * 50000.0f;
        
        ret.miIntersectionType = 1;
    }
    else
    {
        fReflectivity = 0.5f;

        float2 motionVector = motionVectorTexture[screenCoord].xy;
        motionVector = motionVector * 2.0f - 1.0f;
        BouncedRadianceInfo bounceRadianceInfo = getBouncedRadiance(
            worldPosition,
            normal,
            candidateHitPosition.xyz,
            candidateHitNormal.xyz,
            intersectionInfo.mHitUV,
            motionVector,
            0.8f,
            defaultUniformBuffer.mViewProjectionMatrix,
            textureSize,
            intersectionInfo.miMaterialID,
            prevTemporalRadianceTexture
        );
        candidateRadiance.xyz = bounceRadianceInfo.mRadiance;
        fJacobian = 1.0f;
        fRadianceDP = bounceRadianceInfo.mfAnglePct;
        fDistanceAttenuation = bounceRadianceInfo.mfDistanceAttenuation;
    }

    candidateRadiance.xyz += emissiveRadiance;

    // reservoir
    float fLuminance = computeLuminance(
        candidateRadiance.xyz
    );

    float fPHat = clamp(fLuminance, 0.0f, 1.0f);
    ret.mbExchanged = false;

    ret.mReservoir.x += fPHat;
    ret.mReservoir.z += fM;
    float fWeightPct = fPHat / ret.mReservoir.x;
    if(fRand2 < fWeightPct || ret.mReservoir.z <= 0.0f)
    {
        ret.mReservoir.y = fPHat;
        //ret.mRadiance = candidateRadiance;
        ret.mHitPosition = candidateHitPosition;
        ret.mHitNormal = candidateHitNormal;
        ret.mRayDirection = candidateRayDirection;
        ret.mbExchanged = true;

        ret.mRadiance.xyz = computeSpecularRadiance(
            candidateRadiance.xyz,
            worldPosition,
            normal,
            candidateHitPosition.xyz,
            fRoughness,
            fMetalness
        );
    }
    else
    {
        ret.miIntersectionType = 0;
    }

    // clamp reservoir
    if(ret.mReservoir.z > fMaxTemporalReservoirSamples)
    {
        float fPct = fMaxTemporalReservoirSamples / ret.mReservoir.z;
        ret.mReservoir.x *= fPct;
        ret.mReservoir.z = fMaxTemporalReservoirSamples;
    }

    
    ret.mIntersectionResult = intersectionInfo;
    ret.mfNumValidSamples += fM * float(fLuminance > 0.0f);

    ret.mCandidateRadiance = candidateRadiance.xyz;
    ret.mCandidateHitPosition = candidateHitPosition;
    ret.mCandidateHitNormal = candidateHitNormal;
    ret.mCandidateHitUV = intersectionInfo.mHitUV;
    ret.miCandidateHitMesh = intersectionInfo.miMesh;
    ret.miCandidateHitTriangle = intersectionInfo.miHitTriangle;

    return ret;
}

/////
ReservoirResult updateReservoir(
    in float4 reservoir,
    in float fPHat,
    in float fM,
    in float fRand)
{
    ReservoirResult ret;
    ret.mReservoir = reservoir;
    ret.mbExchanged = false;

    ret.mReservoir.x += fPHat;

    ret.mReservoir.z += fM; // * fMult;
    
    float fWeightPct = fPHat / ret.mReservoir.x;

    if(fRand < fWeightPct || reservoir.z <= 0.0f)
    {
        ret.mReservoir.y = fPHat;
        ret.mbExchanged = true;
    }

    return ret;
}

/////
RandomResult initRand(
    uint32_t val0, 
    uint32_t val1, 
    uint32_t backoff)
{
    RandomResult retResult;

    uint32_t v0 = val0;
    uint32_t v1 = val1;
    uint32_t s0 = 0u;

    for(uint32_t n = 0; n < backoff; n++)
    {
        s0 += uint32_t(0x9e3779b9);
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }

    retResult.miSeed = v0;
    retResult.mfNum = 0.0f;

    return retResult;
}

/////
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

/////
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
    if(abs(normal.y) > 0.999f)
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

/////
float computeLuminance(
    in float3 radiance)
{
    return dot(radiance, float3(0.2126f, 0.7152f, 0.0722f));
}

/////
float3 barycentric(
    float3 p, 
    float3 a, 
    float3 b, 
    float3 c)
{
    float3 v0 = b - a;
    float3 v1 = c - a;
    float3 v2 = p - a;
    float fD00 = dot(v0, v0);
    float fD01 = dot(v0, v1);
    float fD11 = dot(v1, v1);
    float fD20 = dot(v2, v0);
    float fD21 = dot(v2, v1);
    float fOneOverDenom = 1.0f / (fD00 * fD11 - fD01 * fD01);
    float fV = (fD11 * fD20 - fD01 * fD21) * fOneOverDenom;
    float fW = (fD00 * fD21 - fD01 * fD20) * fOneOverDenom;
    float fU = 1.0f - fV - fW;

    return float3(fU, fV, fW);
}

/*
**
*/
float2 getPreviousScreenUV(
    in float2 screenUV,
    in uint2 textureSize)
{
    uint2 screenCoord = uint2(
        uint32_t(screenUV.x * float(textureSize.x)),
        uint32_t(screenUV.y * float(textureSize.y))
    );

    float2 screenUVCopy = screenUV;
    float3 motionVector = motionVectorTexture[screenCoord].xyz;
    motionVector.xy = motionVector.xy * 2.0f - 1.0f;
    float2 prevScreenUV = screenUVCopy - motionVector.xy;

    return prevScreenUV;
}

/////
float3 ToneMapFilmic_Hejl2015(
    in float3 hdr, 
    in float whitePt)
{
    float4 vh = float4(hdr, whitePt);
    float4 va = (vh * 1.425f) + 0.05f;
    float4 vf = ((vh * va + 0.004f) / ((vh * (va + 0.55f) + 0.0491f))) - float4(0.0821f);
    float3 ret = vf.rgb / vf.w;

    return clamp(ret, float3(0.0f, 0.0f, 0.0f), float3(1.0f, 1.0f, 1.0f));
}

/////
float3 convertToSRGB(
    in float3 radiance)
{
    float maxComp = max(max(radiance.x, radiance.y), radiance.z);
    float3 maxRadiance = max(radiance, 
        float3(0.01f * maxComp));
    float3 linearRadiance = ACESFilm(maxRadiance);

    return linearToSRGB(linearRadiance);
}

/////
float3 ACESFilm(
    in float3 radiance)
{
    float fA = 2.51f;
    float fB = 0.03f;
    float fC = 2.43f;
    float fD = 0.59f;
    float fE = 0.14f;

    return saturate((radiance * (fA * radiance + fB)) / (radiance * (fC * radiance + fD) + fE));
}

/////
float3 linearToSRGB(
    in float3 x)
{
    bool bCond = (x.x < 0.0031308f || x.y < 0.0031308f || x.z < 0.0031308f);
    float3 ret = x * 12.92f;
    if(!bCond) 
    {
        ret = float3(
            pow(x.x, 1.0f / 2.4f) * 1.055f - 0.055f,
            pow(x.y, 1.0f / 2.4f) * 1.055f - 0.055f,
            pow(x.z, 1.0f / 2.4f) * 1.055f - 0.055f
        );
    }

    return ret;
}

// https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/gdc2018-precomputedgiobalilluminationinfrostbite.pdf
// http://orlandoaguilar.github.io/sh/spherical/harmonics/irradiance/map/2017/02/12/SphericalHarmonics.html
#if 0
/*
**
*/
void encodeToSphericalHarmonicCoefficients2(
    in float3 radiance,
    in float3 direction,
    in uint2 screenCoord
)
{
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

    sphericalHarmonicCoefficientTexture0[screenCoord].x = aResults[0].x;
    sphericalHarmonicCoefficientTexture0[screenCoord].y = aResults[0].y;
    sphericalHarmonicCoefficientTexture0[screenCoord].z = aResults[0].z;
    sphericalHarmonicCoefficientTexture0[screenCoord].w = aResults[1].x;

    sphericalHarmonicCoefficientTexture1[screenCoord].x = aResults[1].y;
    sphericalHarmonicCoefficientTexture1[screenCoord].y = aResults[1].z;
    sphericalHarmonicCoefficientTexture1[screenCoord].z = aResults[2].x;
    sphericalHarmonicCoefficientTexture1[screenCoord].w = aResults[2].y;

    sphericalHarmonicCoefficientTexture2[screenCoord].x = aResults[2].z;
    sphericalHarmonicCoefficientTexture2[screenCoord].y = aResults[3].x;
    sphericalHarmonicCoefficientTexture2[screenCoord].z = aResults[3].y;
    sphericalHarmonicCoefficientTexture2[screenCoord].w = aResults[3].z;
}
#endif // #if 0

/*
**
*/
void encodeToSphericalHarmonicCoefficients2(
    in float3 radiance,
    in float3 direction,
    in uint2 screenCoord,
    in float4 prevSphericalHarmonicsCoefficients0,
    in float4 prevSphericalHarmonicsCoefficients1,
    in float4 prevSphericalHarmonicsCoefficients2,
    in float fSrcPct,
    in float fDstPct
)
{
    float4 SHCoefficent0 = prevSphericalHarmonicsCoefficients0 * fSrcPct;
    float4 SHCoefficent1 = prevSphericalHarmonicsCoefficients1 * fSrcPct;
    float4 SHCoefficent2 = prevSphericalHarmonicsCoefficients2 * fSrcPct;

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
    in int2 screenCoord
)
{
    float4 SHCoefficent0 = sphericalHarmonicCoefficientTexture0[screenCoord];
    float4 SHCoefficent1 = sphericalHarmonicCoefficientTexture1[screenCoord];
    float4 SHCoefficent2 = sphericalHarmonicCoefficientTexture2[screenCoord];

    float fFactor = 4.0f * PI;

    float3 aTotalCoefficients[4];
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

/*
**
*/
float4 getIrradianceCachePosition(
    uint32_t iIrradianceCacheIndex
)
{
    return irradianceCache[iIrradianceCacheIndex].mPosition;
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
float4 getPreviousScreenCoord2(
    in float2 screenUV)
{
    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);
    int2 screenCoord = uint2(
        int32_t(screenUV.x * float(textureSize.x)),
        int32_t(screenUV.y * float(textureSize.y)) 
    );
    float2 motionVector = screenMotionVectorTexture[screenCoord].xy; 
    float2 prevScreenUV = screenUV - motionVector;

    return float4(
        float(floor(prevScreenUV.x * float(textureSize.x) + 0.5f)),
        float(floor(prevScreenUV.y * float(textureSize.y) + 0.5f)), 
        0.0f, 
        1.0f);
}

/*
**
*/
bool isDisoccluded(
    in float2 prevScreenUV,
    in int2 screenCoord
)
{
    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);
    float3 worldPosition = worldPositionTexture[screenCoord].xyz;

    int2 prevScreenCoord = int2(
        int32_t(floor(prevScreenUV.x * textureSize.x + 0.5f)),
        int32_t(floor(prevScreenUV.y * textureSize.y + 0.5f))
    );
    float3 prevWorldPosition = prevWorldPositionTexture[prevScreenCoord].xyz;
    float3 worldPositionDiff = worldPosition.xyz - prevWorldPosition;

    float3 normal = normalTexture[screenCoord].xyz;
    float3 prevNormal = prevNormalTexture[prevScreenCoord].xyz;
    float4 motionVector = motionVectorTexture[screenCoord];
    float4 prevMotionVectorAndMeshIDAndDepth = prevMotionVectorTexture[prevScreenCoord];

    float fDepth = fract(motionVector.z);
    float fPrevDepth = fract(prevMotionVectorAndMeshIDAndDepth.z);

    uint32_t iMesh = uint32_t(motionVector.z - fDepth);
    uint32_t iPrevMesh = uint32_t(prevMotionVectorAndMeshIDAndDepth.z - fPrevDepth);

    float fDP = max(dot(normal, prevNormal), 0.0f);
    float fLength = dot(worldPositionDiff, worldPositionDiff);
    bool bDisoccluded = (fLength > 0.005f || /*fDP < 0.8f || */iMesh != iPrevMesh);

    return bDisoccluded;
}

/*
**
*/
OnScreenRadianceResult getOnScreenRadiance(
    in float3 hitPosition,
    in float3 worldPosition,
    in int2 screenCoord,
    in float2 motionVector
)
{
    OnScreenRadianceResult ret;
    ret.mbOnScreen = false;
    ret.mRadiance = float3(0.0f, 0.0f, 0.0f);

    float3 diff = hitPosition - worldPosition;
    if(length(diff) < 1.0f)
    {
        return ret;
    }

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    // distance for on-screen radiance and ambient occlusion
    float fDistance = length(hitPosition.xyz - worldPosition);
    float fDistanceAttenuation = 1.0f / max(fDistance * fDistance, 1.0f);

    // get on-screen radiance if there's any
    float4 clipSpacePosition = mul(defaultUniformBuffer.mViewProjectionMatrix, float4(hitPosition.xyz, 1.0f));
    clipSpacePosition.x /= clipSpacePosition.w;
    clipSpacePosition.y /= clipSpacePosition.w;
    clipSpacePosition.z /= clipSpacePosition.w;
    clipSpacePosition.x = clipSpacePosition.x * 0.5f + 0.5f;
    clipSpacePosition.y = (clipSpacePosition.y * 0.5f + 0.5f);
    clipSpacePosition.z = clipSpacePosition.z * 0.5f + 0.5f;

    float3 prevClipSpacePosition = float3(
        clipSpacePosition.x - motionVector.x,
        clipSpacePosition.y - motionVector.y,
        clipSpacePosition.z);

    uint2 clipSpaceCoord = uint2(
        uint32_t(floor(clipSpacePosition.x * float(textureSize.x) + 0.5f)),
        uint32_t(floor(clipSpacePosition.y * float(textureSize.y) + 0.5f))
    );

    uint2 prevClipSpaceCoord = uint2(
        uint32_t(floor(prevClipSpacePosition.x * float(textureSize.x) + 0.5f)),
        uint32_t(floor(prevClipSpacePosition.y * float(textureSize.y) + 0.5f))
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

    float fReflectivity = 0.3f;

    float fDepthDiff = abs(hitPositionClipSpace.z - clipSpacePosition.z);
    if (prevClipSpacePosition.x >= 0.0f && prevClipSpacePosition.x <= 1.0f &&
        prevClipSpacePosition.y >= 0.0f && prevClipSpacePosition.y <= 1.0f &&
        prevClipSpacePosition.z >= 0.0f && prevClipSpacePosition.z <= 1.0f &&
        fDepthDiff <= 0.1f)
    {
        ret.mRadiance = prevTemporalRadianceTexture[prevClipSpaceCoord].xyz * albedoTexture[clipSpaceCoord].xyz;
        ret.mbOnScreen = true;
    }

    if(clipSpacePosition.z >= 0.0f && clipSpacePosition.z <= 1.0f &&
       clipSpacePosition.x >= 0.0f && clipSpacePosition.x <= 1.0f &&
       clipSpacePosition.y >= 0.0f && clipSpacePosition.y <= 1.0f &&
       fDepthDiff <= 0.1f)
    {
        // on screen
        ret.mRadiance += directSunOutputTexture[clipSpaceCoord].xyz * albedoTexture[clipSpaceCoord].xyz;
        ret.mbOnScreen = true;
    }

    // distance attenuation
    ret.mRadiance *= fDistanceAttenuation * fReflectivity;

    return ret;
}

/*
**
*/
float3 sampleGGXVNDF(
    in float3 viewDirection,
    in float fRoughness,
    in float fRand0,
    in float fRand1)
{
    // Section 3.2: transforming the view direction to the hemisphere configuration
    float3 Vh = normalize(float3(fRoughness * viewDirection.x, fRoughness * viewDirection.y, viewDirection.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    float3 T1 = float3(1.0f, 0.0f, 0.0f);
    if(lensq > 0)
    {
        T1 = float3(-Vh.y, Vh.x, 0) * (1.0f / sqrt(lensq));
    }

    float3 T2 = cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r = sqrt(fRand0);
    float phi = 2.0f * PI * fRand1;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5f * (1.0f + Vh.z);
    t2 = (1.0f - s)*sqrt(1.0f - t1*t1) + s*t2;
    // Section 4.3: reprojection onto hemisphere
    float3 Nh = t1*T1 + t2*T2 + sqrt(max(0.0f, 1.0f - t1*t1 - t2*t2))*Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    float3 Ne = normalize(float3(fRoughness * Nh.x, fRoughness * Nh.y, max(0.0f, Nh.z)));
    return Ne;
}

/*
**
*/
float3 computeSpecularRadiance(
    in float3 radiance,
    in float3 worldPosition,
    in float3 normal,
    in float3 hitPosition,
    in float fRoughness,
    in float fMetalness
)
{
    float3 specularLight = float3(0.0f, 0.0f, 0.0f);
    float kS = 0.0f;
    {
        float3 viewDir = normalize(defaultUniformBuffer.mCameraPosition.xyz - worldPosition.xyz);

        float3 F0 = float3(0.04f, 0.04f, 0.04f);
        F0 = lerp(F0, float3(1.0f, 1.0f, 1.0f), fMetalness);

        // calculate per-light radiance
        float3 lightDir = normalize(hitPosition.xyz - worldPosition.xyz);
        float3 halfV = normalize(viewDir + lightDir);

        // cook-torrance brdf
        float NDF = distributionGGX(normal.xyz, halfV, fRoughness);
        float geometry = geometrySmith(normal.xyz, viewDir, lightDir, fRoughness);
        float3 fresnel = fresnelShlickRoughness(max(dot(halfV, viewDir), 0.0f), F0, fRoughness);

        float3 numerator = fresnel * NDF * geometry;
        float denominator = 4.0f * max(dot(normal.xyz, viewDir), 0.0f) * max(dot(normal.xyz, lightDir), 0.0f) + 0.0001f;
        float3 specular = clamp(
                           numerator / denominator,
                           float3(0.0f, 0.0f, 0.0f),
                           float3(1.0f, 1.0f, 1.0f));

        specularLight = specular * radiance * max(dot(normal.xyz, lightDir.xyz), 0.0f) * 1.0f;
        kS = 1.0f - fresnel.x;
    }

    return specularLight;
}

/*
**
*/
float distributionGGX(
    in float3 N: vec3<f32>,
    in float3 H: vec3<f32>,
    in float roughness)
{
    float a  = roughness * roughness;
    float a2  = a * a;
    float NdotH  = max(dot(N, H), 0.0f);
    float NdotH2  = NdotH * NdotH;

    float num  = a2;
    float denom  = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return num / denom;
}

/*
**
*/
float geometrySchlickGGX(
    float NdotV,
    float roughness)
{
    float r  = (roughness + 1.0f);
    float k  = (r * r) / 8.0f;

    float num  = NdotV;
    float denom  = NdotV * (1.0f - k) + k;

    return num / denom;
}

/*
**
*/
float geometrySmith(
    float3 N: vec3<f32>,
    float3 V: vec3<f32>,
    float3 L: vec3<f32>,
    float roughness)
{
    float NdotV  = max(dot(N, V), 0.0f);
    float NdotL  = max(dot(N, L), 0.0f);
    float ggx2  = geometrySchlickGGX(NdotV, roughness);
    float ggx1  = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

/*
**
*/
float3 fresnelShlickRoughness(
    float fCosTheta,
    float3 f0: vec3<f32>,
    float fRoughness)
{
    float fOneMinusRoughness  = 1.0f - fRoughness;
    float fClamp  = clamp(1.0f - fCosTheta, 0.0f, 1.0f);
    return f0 + (max(float3(fOneMinusRoughness, fOneMinusRoughness, fOneMinusRoughness), f0) - f0) * pow(fClamp, 5.0f);
}