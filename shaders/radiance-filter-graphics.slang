#include "default-uniform-headers.slang"

#define PI 3.14159f

struct ConstantBufferData
{
    uint32_t miRadius;
};

struct AxisInfo
{
    float3 mTangent;
    float3 mBinormal;
};

struct AssembledVertex
{
    float4 position : POSITION;
    float4 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 mDiffuseOutput : SV_TARGET0;
    float4 mEmissiveOutput : SV_TARGET1;
    float4 mSpecularOutput : SV_TARGET2;
    float4 mAmbientOcclusionOutput : SV_TARGET3;
};

struct VertexStageOutput
{
    float4 sv_position : SV_Position;
    float4 texCoord;
};

struct BlurOutput
{
    float3                          mDiffuseRadiance;
    float3                          mEmissiveRadiance;
    float3                          mSpecularRadiance;
    float                           mfAmbientOcclusion;
};

[[vk::binding(0, 0)]] Texture2D<float4> diffuseRadianceTexture;
[[vk::binding(1, 0)]] Texture2D<float4> emissiveRadianceTexture;
[[vk::binding(2, 0)]] Texture2D<float4> specularRadianceTexture;

[[vk::binding(3, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(4, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(5, 0)]] Texture2D<float4> ambientOcclusionTexture;
[[vk::binding(6, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] Texture2D<float4> blueNoiseTexture0;
[[vk::binding(1, 1)]] Texture2D<float4> blueNoiseTexture1;
[[vk::binding(2, 1)]] Texture2D<float4> blueNoiseTexture2;
[[vk::binding(3, 1)]] Texture2D<float4> blueNoiseTexture3;
[[vk::binding(4, 1)]] Texture2D<float4> blueNoiseTexture4;
[[vk::binding(5, 1)]] ConstantBuffer<ConstantBufferData> constantBuffer;
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

    float2 uv = float2(
        in.texCoord.x,
        in.texCoord.y 
    );

    BlurOutput blurOutput = blur2(uv);

    out.mDiffuseOutput = float4(blurOutput.mDiffuseRadiance.xyz, 1.0f);
    out.mEmissiveOutput = float4(blurOutput.mEmissiveRadiance.xyz, 1.0f);
    out.mSpecularOutput = float4(blurOutput.mSpecularRadiance.xyz, 1.0f);
    out.mAmbientOcclusionOutput = float4(blurOutput.mfAmbientOcclusion, blurOutput.mfAmbientOcclusion, blurOutput.mfAmbientOcclusion, 1.0f);

    return out;
}

/*
**
*/
BlurOutput blur2(
    float2 texCoord)
{
    float fTwoPI = 6.283185f;
    float kfPositionThreshold = 1.0f;
    float kfAngleThreshold = 0.9f;
    float fRadius = 8.0f;

    uint2 blueNoiseTextureSize;
    blueNoiseTexture0.GetDimensions(blueNoiseTextureSize.x, blueNoiseTextureSize.y);
    uint32_t iTotalTextureSize = blueNoiseTextureSize.x * blueNoiseTextureSize.y;
    uint32_t iTileSize = 32u;
    uint32_t iNumTilesPerRow = blueNoiseTextureSize.x / iTileSize;
    uint32_t iNumTotalTiles = iNumTilesPerRow * iNumTilesPerRow;

    float fOneOverScreenWidth = 1.0f / float(defaultUniformBuffer.miScreenWidth);
    float fOneOverScreenHeight = 1.0f / float(defaultUniformBuffer.miScreenHeight);

    float4 worldPosition = worldPositionTexture.Sample(
        textureSampler,
        texCoord
    );

    float3 normal = normalTexture.Sample(
        textureSampler,
        texCoord
    ).xyz;

    uint32_t iMesh = uint32_t(worldPosition.w - fract(worldPosition.w));

    AxisInfo axisInfo = getAxis(normal);
    float fPlaneD = dot(normal, worldPosition.xyz) * -1.0f;

    BlurOutput ret;
    ret.mDiffuseRadiance = float3(0.0f);
    ret.mEmissiveRadiance = float3(0.0f);
    ret.mSpecularRadiance = float3(0.0f);
    ret.mfAmbientOcclusion = 0.0f;

    int2 screenCoord = int2(
        int32_t(texCoord.x * float(defaultUniformBuffer.miScreenWidth)),
        int32_t(texCoord.y * float(defaultUniformBuffer.miScreenHeight))
    );

    float2 oneOverScreenDimension = float2(
        1.0f / float(defaultUniformBuffer.miScreenWidth),
        1.0f / float(defaultUniformBuffer.miScreenHeight)
    );

    float fAO = ambientOcclusionTexture[screenCoord].x / ambientOcclusionTexture[screenCoord].y;

    int32_t iRadius = constantBuffer.miRadius;
    iRadius = max(int32_t((fAO) * float(iRadius)), 3);
    float fTotalWeight = 1.0f;
    float fTotalAOWeight = 0.0f;
    for (int32_t iY = -iRadius; iY <= iRadius; iY++)
    {
        for (int32_t iX = -iRadius; iX <= iRadius; iX++)
        {
            int2 offset = int2(iX, iY);
            int2 sampleScreenCoord = screenCoord + offset;

            float3 diffuseRadiance = diffuseRadianceTexture[sampleScreenCoord].xyz;
            float3 emissiveRadiance = emissiveRadianceTexture[sampleScreenCoord].xyz;
            float3 specularRadiance = specularRadianceTexture[sampleScreenCoord].xyz;

            float3 sampleWorldPosition = worldPositionTexture[sampleScreenCoord].xyz;
            float3 sampleNormal = normalTexture[sampleScreenCoord].xyz;
            float3 diff = sampleWorldPosition - worldPosition.xyz;
            float fDP = max(dot(sampleNormal, normal.xyz), 0.0f);
            float fPlaneDistance = dot(diff, normal) + fPlaneD;
            float fPlaneWeight = min(abs(1.0f / (fPlaneDistance + 0.001f)), 1.0f);

            float fWeight = clamp(fPlaneWeight, 0.0f, 1.0f) * fDP;
            ret.mDiffuseRadiance += diffuseRadiance * fWeight;
            ret.mEmissiveRadiance += emissiveRadiance * fWeight;
            ret.mSpecularRadiance += specularRadiance * fWeight;

            if (iX >= -2 && iX <= 2 && iY >= -2 && iY <= 2)
            {
                fAO = ambientOcclusionTexture[sampleScreenCoord].x / ambientOcclusionTexture[sampleScreenCoord].y;
                ret.mfAmbientOcclusion += (1.0f - fAO) * fWeight;
                fTotalAOWeight += fWeight;
            }

            fTotalWeight += fWeight;
        }
    }

    float fOneOverTotalWeight = 1.0f / fTotalWeight;
    ret.mDiffuseRadiance *= fOneOverTotalWeight;
    ret.mEmissiveRadiance *= fOneOverTotalWeight;
    ret.mSpecularRadiance *= fOneOverTotalWeight;

    ret.mfAmbientOcclusion /= fTotalAOWeight;

    return ret;
}

/*
**
*/
AxisInfo getAxis(
    in float3 normal)
{
    AxisInfo ret;

    float3 up  = float3(0.0f, 1.0f, 0.0f);
    if (abs(normal.y) > abs(normal.x) && abs(normal.y) > abs(normal.z))
    {
        up = float3(1.0f, 0.0f, 0.0f);
    }

    ret.mTangent = normalize(cross(up, normal));
    ret.mBinormal = normalize(cross(normal, ret.mTangent));

    return ret;
}
