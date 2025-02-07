#define FLT_MAX  1.0e+10

#include "default-uniform-headers.slang"
#include "octahedron-map-headers.slang"
#include "motion-vector-headers.slang"
#include "murmur-hash-headers.slang"
#include "irradiance-cache-headers.slang"

struct HashEntry
{
    uint32_t miPageCoordinate;
    uint32_t miPageIndex;
    uint32_t miTextureIDAndMIP;
    uint32_t miUpdateFrame;
};

struct TextureAtlasOutput
{
    float4 mColor;
    bool mbLoaded;
};

[[vk::binding(0, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(1, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(2, 0)]] Texture2D<float4> prevWorldPositionTexture;
[[vk::binding(3, 0)]] Texture2D<float4> prevNormalTexture;
[[vk::binding(4, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(5, 0)]] Texture2D<float4> prevMotionVectorTexture;
[[vk::binding(6, 0)]] Texture2D<float4> screenMotionVectorTexture;

[[vk::binding(7, 0)]] Texture2D<float4> indirectDiffuseRadianceTexture;
[[vk::binding(8, 0)]] Texture2D<float2> indirectDiffuseMomentTexture;

[[vk::binding(9, 0)]] StructuredBuffer<IrradianceCacheEntry> irradianceCache;

[[vk::binding(10, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

struct AssembledVertex
{
    float4 position : POSITION;
    float4 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 mOutput0 : SV_TARGET0;
    float4 mOutput1 : SV_TARGET1;
    float4 mOutput2 : SV_TARGET2;
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

[shader("fragment")]
PSOutput PSMain(VertexStageOutput in)
{
    PSOutput out;

    float2 uv = float2(
        in.texCoord.x,
        in.texCoord.y 
    );

    float3 normal = normalTexture.Sample(
        textureSampler,
        uv
    ).xyz;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    float fOneOverWidth = 1.0f / float(textureSize.x);
    float fOneOverHeight = 1.0f / float(textureSize.y);

    float fDisocclusion = 0.0f;
    float4 prevScreenCoord = getPreviousScreenCoord2(uv);
    float2 prevScreenUV = float2(
        float(prevScreenCoord.x) * fOneOverWidth,
        float(prevScreenCoord.y) * fOneOverHeight
    );
    if(isPrevUVOutOfBounds(uv))
    {
        fDisocclusion = 1.0f;
    }
    else
    {
        fDisocclusion = (isDisoccluded3(uv, prevScreenUV) == true) ? 1.0f : 0.0f;
    }

    float3 worldPosition = worldPositionTexture.Sample(
                                                   textureSampler,
                                                   uv
    ) .xyz;
    uint32_t iCacheIndex = fetchIrradianceCacheIndex(worldPosition);
    out.mOutput0.xyz = getRadianceFromIrradianceCacheProbe(normal, iCacheIndex);
    out.mOutput0.w = float(iCacheIndex);

    out.mOutput1.xyz = worldPosition;
    out.mOutput1.w = float(iCacheIndex);

    float3 scaledPosition = worldPosition;
    float fSignX = sign(worldPosition.x);
    float fSignY = sign(worldPosition.y);
    float fSignZ = sign(worldPosition.z);
    scaledPosition.x = float(floor(abs(scaledPosition.x) + 0.5f)) * 0.1f * fSignX;
    scaledPosition.y = float(floor(abs(scaledPosition.y) + 0.5f)) * 0.1f * fSignY;
    scaledPosition.z = float(floor(abs(scaledPosition.z) + 0.5f)) * 0.1f * fSignZ;

    out.mOutput2.xyz = scaledPosition;
    out.mOutput2.w = float(iCacheIndex);

#if 0
    float3 radiance = indirectDiffuseRadianceTexture.Sample(
        textureSampler,
        float2(in.texCoord.x, 1.0f - in.texCoord.y)
    ).xyz;

    // out.mOutput = float4(normal.xyz + float3(fDisocclusion, 0.0f, 0.0f), 1.0f);
    // out.mOutput0 = float4(fDisocclusion, fDisocclusion, fDisocclusion, 1.0f);
    out.mOutput0 = float4(radiance.xyz + float3(fDisocclusion, 0.0f, 0.0f), 1.0f);
    out.mOutput1 = prevScreenCoord;

    uint32_t iX = uint32_t(uv.x * float(textureSize.x));
    uint32_t iY = uint32_t(uv.y * float(textureSize.x));

    SVGFFilterResult svgfRet = svgFilter(
        iX,
        iY,
        1,
        30.0f
    );

    out.mOutput2 = float4(
        svgfRet.mRadiance,
        1.0f
    );
#endif // #if 0

    return out;
}

/*
**
*/
float4 getPreviousScreenCoord2(
    in float2 screenUV)
{
    float2 screenUVCopy = screenUV;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    int2 screenCoord = uint2(
        int32_t(screenUV.x * float(textureSize.x)),
        int32_t(screenUV.y * float(textureSize.y)) 
    );

    float3 worldPosition = worldPositionTexture[screenCoord].xyz;
    float3 normal = normalTexture[screenCoord].xyz;

    float fOneOverScreenWidth = 1.0f / float(textureSize.x);
    float fOneOverScreenHeight = 1.0f / float(textureSize.y);

    float2 motionVector = screenMotionVectorTexture[screenCoord].xy;
    int2 prevScreenCoord = screenCoord - int2(int32_t(motionVector.x), int32_t(motionVector.y));

    return float4(float(screenCoord.x), float(screenCoord.y), 0.0f, 1.0f);
}

/*
**
*/
bool isDisoccluded3(
    in float2 screenUV,
    in float2 prevScreenUV
)
{
    uint2 textureSize;

    float3 worldPosition = worldPositionTexture.Sample(textureSampler, screenUV).xyz;
    float3 prevWorldPosition = prevWorldPositionTexture.Sample(textureSampler, prevScreenUV).xyz;
    float3 normal = normalTexture.Sample(textureSampler, screenUV).xyz;
    float3 prevNormal = prevNormalTexture.Sample(textureSampler, prevScreenUV).xyz;
    float4 motionVector = motionVectorTexture.Sample(textureSampler, screenUV);
    float4 prevMotionVectorAndMeshIDAndDepth = prevMotionVectorTexture.Sample(textureSampler, prevScreenUV);

    float fDepth = fract(motionVector.z);
    float fPrevDepth = fract(prevMotionVectorAndMeshIDAndDepth.z);

    uint32_t iMesh = uint32_t(motionVector.z - fDepth);
    uint32_t iPrevMesh = uint32_t(prevMotionVectorAndMeshIDAndDepth.z - fPrevDepth);

    float fCheckDepth = abs(fDepth - fPrevDepth);
    float3 worldPositionDiff = prevWorldPosition.xyz - worldPosition.xyz;
    float fCheckDP = abs(dot(normalize(normal.xyz), normalize(prevNormal.xyz)));
    float fCheckWorldPositionDistance = dot(worldPositionDiff, worldPositionDiff);

    return (fCheckWorldPositionDistance > 0.01f || iMesh != iPrevMesh);
}

/*
**
*/
float computeSVGFNormalStoppingWeight(
    float3 sampleNormal,
    float3 normal,
    float fPower)
{
    float fDP = clamp(dot(normal, sampleNormal), 0.0f, 1.0f);
    return pow(fDP, fPower);
};

/*
**
*/
float computeSVGFDepthStoppingWeight(
    float fSampleDepth,
    float fDepth,
    float fPhi)
{
    float kfEpsilon = 0.001f;
    return exp(-abs(fDepth - fSampleDepth) / (fPhi + kfEpsilon));
}

/*
**
*/
float computeSVGFLuminanceStoppingWeight(
    float fSampleLuminance,
    float fLuminance,
    int32_t iX,
    int32_t iY,
    float fPower)
{
    float kfEpsilon = 0.001f;

    float fOneOverSixteen = 1.0f / 16.0f;
    float fOneOverEight = 1.0f / 8.0f;
    float fOneOverFour = 1.0f / 4.0f;
    float afKernel[9] = {
        fOneOverSixteen, fOneOverEight, fOneOverSixteen,
        fOneOverEight, fOneOverFour, fOneOverEight,
        fOneOverSixteen, fOneOverEight, fOneOverSixteen,
    };

    // gaussian blur for variance
    float fLuminanceDiff = abs(fLuminance - fSampleLuminance);
    float fTotalVariance = 0.0f;
    float fTotalKernel = 0.0f;
    for(int32_t iOffsetY = -1; iOffsetY <= 1; iOffsetY++)
    {
        int32_t iSampleY  = iY + iOffsetY;
        if(iSampleY < 0 && iSampleY >= int32_t(defaultUniformBuffer.miScreenHeight))
        {
            continue;
        }
        for(int32_t iOffsetX = -1; iOffsetX <= 1; iOffsetX++)
        {
            int32_t iSampleX  = iX + iOffsetX;
            if (iSampleX < 0 && iSampleY >= int32_t(defaultUniformBuffer.miScreenWidth))
            {
                continue;
            }

            float2 sampleUV = float2(
                float(iSampleX) / float(defaultUniformBuffer.miScreenWidth),
                float(iSampleY) / float(defaultUniformBuffer.miScreenHeight));
            float2 moment = indirectDiffuseMomentTexture.Sample(
                             textureSampler,
                             sampleUV
            );

            int32_t iIndex  = (iOffsetY + 1) * 3 + (iOffsetX + 1);
            fTotalVariance += afKernel[iIndex] * abs(moment.y - moment.x * moment.x);
            fTotalKernel += afKernel[iIndex];
        }
    }

    float fRet = exp(-fLuminanceDiff / (sqrt(fTotalVariance / fTotalKernel) * fPower + kfEpsilon));
    return fRet;
}

struct SVGFFilterResult
{
    float3 mRadiance;
    float2 mMoments;
};

/////
SVGFFilterResult svgFilter(
    int32_t iX,
    int32_t iY,
    int32_t iStep,
    float fLuminancePhi)
{
    SVGFFilterResult ret;

    float3 retRadiance = float3(0.0f, 0.0f, 0.0f);
    float fRetVariance = 0.0f;

    // 5x5 A-Trous kernel
    float afKernel[25] =
    {
        1.0f / 256.0f, 1.0f / 64.0f, 3.0f / 128.0f, 1.0f / 64.0f, 1.0f / 256.0f,
        1.0f / 64.0f, 1.0f / 16.0f, 3.0f / 32.0f, 1.0f / 16.0f, 1.0f / 64.0f,
        3.0f / 128.0f, 3.0f / 32.0f, 9.0f / 64.0f, 3.0f / 32.0f, 3.0f / 128.0f,
        1.0f / 64.0f, 1.0f / 16.0f, 3.0f / 32.0f, 1.0f / 16.0f, 1.0f / 64.0f,
        1.0f / 256.0f, 1.0f / 64.0f, 3.0f / 128.0f, 1.0f / 64.0f, 1.0f / 256.0f
    };

    float kfNormalPower = 128.0f;
    float kfDepthPhi = 1.0e-2f;

    float2 imageUV = float2(
        float(iX) / float(defaultUniformBuffer.miScreenWidth),
        float(iY) / float(defaultUniformBuffer.miScreenHeight)
    );

    float4 worldPosition  = worldPositionTexture.Sample(
        textureSampler,
        imageUV
    );
    if (worldPosition.w == 0.0f)
    {
        return ret;
    }

    float4 normal  = normalTexture.Sample(
        textureSampler,
        imageUV
    );

    float fDepth = fract(worldPosition.w);

    float4 radiance  = indirectDiffuseRadianceTexture.Sample(
        textureSampler,
        imageUV
    );
    float fLuminance = computeLuminance(radiance.xyz);

    float3 totalRadiance = float3(0.0f, 0.0f, 0.0f);
    float fTotalWeight = 0.0f;
    float fTotalSquaredWeight = 0.0f;
    float fTotalWeightedVariance = 0.0f;
    uint32_t iStepSize = 1u << uint32_t(iStep);

    float2 totalMoments = float2(0.0f, 0.0f);

    for(int32_t iStepY = -2; iStepY <= 2; iStepY++)
    {
        int32_t iOffsetY  = iStepY * int32_t(iStepSize);
        int32_t iSampleY  = iY + iOffsetY;
        if(iSampleY < 0 || iSampleY >= int32_t(defaultUniformBuffer.miScreenHeight))
        {
            continue;
        }

        int32_t iCount  = 0;
        for(int32_t iStepX = -2; iStepX <= 2; iStepX++)
        {
            int32_t iOffsetX  = iStepX * int32_t(iStepSize);
            int32_t iSampleX  = iX + iOffsetX;
            if (iSampleX < 0 || iSampleX >= int32_t(defaultUniformBuffer.miScreenWidth))
            {
                continue;
            }

            float2 sampleUV = float2(
                               float(iSampleX) / float(defaultUniformBuffer.miScreenWidth),
                               float(iSampleY) / float(defaultUniformBuffer.miScreenHeight)
            );

            float4 sampleRadiance  = indirectDiffuseRadianceTexture.Sample(
                                        textureSampler,
                                        sampleUV
            );
            float4 sampleWorldPosition  = worldPositionTexture.Sample(
                                             textureSampler,
                                             sampleUV
            );
            float4 sampleNormal  = normalTexture.Sample(
                                   textureSampler,
                                   sampleUV
            );

            float2 moment = indirectDiffuseMomentTexture.Sample(
                             textureSampler,
                             sampleUV
            );

            float fSampleVariance = abs(moment.y - moment.x * moment.x);
            float fSampleLuminance = computeLuminance(sampleRadiance.xyz);

            if (sampleWorldPosition.w == 0.0f)
            {
                continue;
            }

            float fSampleDepth = fract(sampleWorldPosition.w);

            float fSampleNormalWeight = computeSVGFNormalStoppingWeight(
                                            sampleNormal.xyz,
                                            normal.xyz,
                                            kfNormalPower);
            float fSampleDepthWeight = computeSVGFDepthStoppingWeight(
                                         fDepth,
                                         fSampleDepth,
                                         kfDepthPhi);

            float fRetTotalVariance = 0.0f;
            float fSampleLuminanceWeight = computeSVGFLuminanceStoppingWeight(
                                             fSampleLuminance,
                                             fLuminance,
                                             iX,
                                             iY,
                                             fLuminancePhi);

            float fSampleWeight = fSampleNormalWeight * fSampleDepthWeight * fSampleLuminanceWeight;

            float fKernel = afKernel[iCount];
            iCount += 1;
            float fKernelWeight = fKernel * fSampleWeight;

            totalRadiance += sampleRadiance.xyz * fKernelWeight;
            fTotalWeight += fKernelWeight;
            fTotalSquaredWeight += fKernelWeight * fKernelWeight;

            fTotalWeightedVariance += fKernelWeight * fSampleVariance;

            totalMoments += moment.xy * fKernelWeight;
        }
    }

    retRadiance = totalRadiance / (fTotalWeight + 0.0001f);
    fRetVariance = fTotalWeightedVariance / (fTotalSquaredWeight + 0.0001f);

    ret.mRadiance = retRadiance;
    ret.mMoments = totalMoments / (fTotalWeight + 1.0e-4f);

    return ret;
}

/////
float computeLuminance(
    float3 radiance)
{
    return dot(radiance, float3(0.2126f, 0.7152f, 0.0722f));
}
