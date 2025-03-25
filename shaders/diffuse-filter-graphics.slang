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
    float4 mOutput : SV_TARGET0;
};

struct VertexStageOutput
{
    float4 sv_position : SV_Position;
    float4 texCoord;
};

[[vk::binding(0, 0)]] Texture2D<float4> diffuseRadianceTexture;
[[vk::binding(1, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(2, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(3, 0)]] Texture2D<float4> ambientOcclusionTexture;
[[vk::binding(4, 0)]] SamplerState textureSampler;

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

    out.mOutput = blur2(uv);

    return out;
}


/*
**
*/
float4 blur2(
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

    float4 ret = diffuseRadianceTexture.Sample(
        textureSampler,
        texCoord);
    
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
    iRadius = max(int32_t((fAO) * float(iRadius)), 2);
    ret = float4(0.0f);
    float fTotalWeight = 1.0f;
    for (int32_t iY = -iRadius; iY <= iRadius; iY++)
    {
        for (int32_t iX = -iRadius; iX <= iRadius; iX++)
        {
            int2 offset = int2(iX, iY);
            int2 sampleScreenCoord = screenCoord + offset;

            float3 radiance = diffuseRadianceTexture[sampleScreenCoord].xyz;
            float3 sampleWorldPosition = worldPositionTexture[sampleScreenCoord].xyz;
            float3 sampleNormal = normalTexture[sampleScreenCoord].xyz;
            float3 diff = sampleWorldPosition - worldPosition.xyz;
            float fDP = max(dot(sampleNormal, normal.xyz), 0.0f);
            float fPlaneDistance = dot(diff, normal) + fPlaneD;
            float fPlaneWeight = min(abs(1.0f / (fPlaneDistance + 0.001f)), 1.0f);

            float fWeight = clamp(fPlaneWeight, 0.0f, 1.0f) * fDP;
            ret.x += radiance.x * fWeight;
            ret.y += radiance.y * fWeight;
            ret.z += radiance.z * fWeight;
            fTotalWeight += fWeight;
        }
    }

#if 0
    float fTotalWeight = 1.0f;
    uint32_t iFrame = defaultUniformBuffer.miFrame;
    int32_t iNumSamples = 32;
    for (int32_t i = 0; i < iNumSamples; i++)
    {
        uint32_t iTotalFrames = defaultUniformBuffer.miFrame * iNumSamples + i;
        int2 blueNoiseScreenCoord = int2(
            (screenCoord.x + iTotalFrames) % blueNoiseTextureSize.x,
            ((screenCoord.y + iTotalFrames) / blueNoiseTextureSize.x) % blueNoiseTextureSize.y
        );

        float3 blueNoise = float3(0.0f, 0.0f, 0.0f);
        if (iFrame % 5 == 0)
        {
            blueNoise = blueNoiseTexture0[blueNoiseScreenCoord].xyz;
        }
        else if (iFrame % 5 == 1)
        {
            blueNoise = blueNoiseTexture1[blueNoiseScreenCoord].xyz;
        }
        else if (iFrame % 5 == 2)
        {
            blueNoise = blueNoiseTexture2[blueNoiseScreenCoord].xyz;
        }
        else if (iFrame % 5 == 3)
        {
            blueNoise = blueNoiseTexture3[blueNoiseScreenCoord].xyz;
        }
        else if (iFrame % 5 == 4)
        {
            blueNoise = blueNoiseTexture4[blueNoiseScreenCoord].xyz;
        }

        float fCurrRadius = max(blueNoise.y * fRadius, 1.0f); 

        float fTheta = blueNoise.x * 2.0f * PI;
        float fSinTheta = sin(fTheta);
        float fCosTheta = cos(fTheta);

        int2 offset = int2(
            int32_t(fCosTheta * fCurrRadius),
            int32_t(fSinTheta * fCurrRadius)
        );

        int2 sampleScreenCoord = screenCoord + offset;

        float3 radiance = diffuseRadianceTexture[sampleScreenCoord].xyz;
        float3 sampleWorldPosition = worldPositionTexture[sampleScreenCoord].xyz;
        float3 sampleNormal = normalTexture[sampleScreenCoord].xyz;
        float3 diff = sampleWorldPosition - worldPosition.xyz;
        float fDP = max(dot(sampleNormal, normal.xyz), 0.0f);
        float fPlaneDistance = dot(diff, normal) + fPlaneD;
        float fPlaneWeight = min(abs(1.0f / (fPlaneDistance + 0.001f)), 1.0f);

        float fWeight = clamp(fPlaneWeight, 0.0f, 1.0f) * fDP;
        ret.x += radiance.x * fWeight;
        ret.y += radiance.y * fWeight;
        ret.z += radiance.z * fWeight;
        fTotalWeight += fWeight;
    }
#endif // #if 0

    ret.x /= fTotalWeight;
    ret.y /= fTotalWeight;
    ret.z /= fTotalWeight;

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
