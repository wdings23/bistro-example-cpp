#include "default-uniform-headers.slang"

struct ConstantBufferData
{
    uint32_t miStep;
};

[[vk::binding(0, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(1, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(2, 0)]] Texture2D<float4> radianceTexture;
[[vk::binding(3, 0)]] Texture2D<float2> momentTexture;

[[vk::binding(4, 0)]] Texture2D<float4> ambientOcclusionTexture;

[[vk::binding(5, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] ConstantBuffer<ConstantBufferData> constantBuffer;
[[vk::binding(1, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

struct SVGFFilterResult
{
    float3 mRadiance;
    float2 mMoments;

    float mfAmbientOcclusion;
};

struct AssembledVertex
{
    float4 position : POSITION;
    float4 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 mRadiance : SV_TARGET0;
    float2 mMoment : SV_TARGET1;

    float mAmbientOcclusion : SV_TARGET2;
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

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    float4 worldPosition = worldPositionTexture.Sample(
        textureSampler,
        in.texCoord.xy
    );
    if (worldPosition.w <= 0.0f)
    {
        out.mRadiance = float4(0.0f, 0.0f, 0.0f, 0.0f);
        out.mMoment = float2(0.0f, 0.0f);
    }

    uint32_t iX = uint32_t(in.texCoord.x * float(textureSize.x));
    uint32_t iY = uint32_t(in.texCoord.y * float(textureSize.x));
    SVGFFilterResult svgfRet = svgFilter(
        iX,
        iY,
        constantBuffer.miStep,
        10.0f
    );

    out.mRadiance = float4(
        svgfRet.mRadiance,
        1.0f
    );

    out.mMoment = svgfRet.mMoments;

    out.mAmbientOcclusion = svgfRet.mfAmbientOcclusion;

    return out;
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
            float2 moment = momentTexture.Sample(
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

/*
**
*/
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
        ret.mRadiance = retRadiance;
        ret.mMoments = float2(0.0f, 0.0f);
        return ret;
    }

    float4 normal  = normalTexture.Sample(
        textureSampler,
        imageUV
    );

    float fDepth = fract(worldPosition.w);

    float4 radiance  = radianceTexture.Sample(
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
    float fTotalAmbientOcclusion = 0.0f;

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

            float4 sampleRadiance  = radianceTexture.Sample(
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

            float2 moment = momentTexture.Sample(
                textureSampler,
                sampleUV
            );

            float fSampleAO = ambientOcclusionTexture.Sample(
                textureSampler,
                sampleUV
            ).z;

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

            fTotalAmbientOcclusion += fSampleAO * fKernelWeight;
        }
    }

    retRadiance = totalRadiance / (fTotalWeight + 0.0001f);
    fRetVariance = fTotalWeightedVariance / (fTotalSquaredWeight + 0.0001f);

    ret.mRadiance = retRadiance;
    ret.mMoments = totalMoments / (fTotalWeight + 1.0e-4f);

    ret.mfAmbientOcclusion = fTotalAmbientOcclusion / (fTotalWeight + 0.0001f);

    return ret;
}

/*
**
*/
float computeLuminance(
    float3 radiance)
{
    return dot(radiance, float3(0.2126f, 0.7152f, 0.0722f));
}