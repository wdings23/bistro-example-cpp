#include "default-uniform-headers.slang"

#define FLT_MAX 1000000.0f
#define NUM_HISTORY 30.0f

[[vk::binding(0, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(1, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(2, 0)]] Texture2D<float4> radianceTexture;
[[vk::binding(3, 0)]] Texture2D<float4> prevRadianceTexture;
[[vk::binding(4, 0)]] Texture2D<float2> motionVectorTexture;
[[vk::binding(5, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

struct AssembledVertex
{
    float4 position : POSITION;
    float4 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 mRadiance : SV_TARGET0;
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
    radianceTexture.GetDimensions(textureSize.x, textureSize.y);
    float fOneOverWidth = 1.0f / float(textureSize.x);
    float fOneOverHeight = 1.0f / float(textureSize.y);

    float2 uv = float2(
        in.texCoord.x,
        in.texCoord.y
    );

    float2 motionVector = motionVectorTexture.Sample(
        textureSampler,
        uv
    );
    motionVector = motionVector * 2.0f - 1.0f;
    float2 prevUV = uv - motionVector;

    int32_t iSampleSize = 1;
    float3 minRadiance = float3(FLT_MAX, FLT_MAX, FLT_MAX);
    float3 maxRadiance = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    float3 totalPrevRadiance = float3(0.0f, 0.0f, 0.0f);
    float fTotalPrevWeight = 0.0f;
    for(int32_t iY = -iSampleSize; iY <= iSampleSize; iY++)
    {
        for(int32_t iX = -iSampleSize; iX <= iSampleSize; iX++)
        {
            float2 offset = float2( 
                float(iX) * fOneOverWidth,
                float(iY) * fOneOverHeight
            );

            float2 sampleUV = uv + offset;
            float3 sampleRadiance = radianceTexture.Sample( 
                textureSampler,
                sampleUV
            ).xyz;

            minRadiance = min(sampleRadiance, minRadiance);
            maxRadiance = max(sampleRadiance, maxRadiance);

            float fWeight = exp(-3.0f * float(iX * iX + iY * iY) / float(3 * 3));

            float2 samplePrevUV = prevUV + offset;
            float3 samplePrevRadiance = prevRadianceTexture.Sample(
                textureSampler,
                samplePrevUV
            ).xyz;
            totalPrevRadiance += samplePrevRadiance * fWeight;

            
            // fTotalPrevWeight += 1.0f;
            fTotalPrevWeight += fWeight;
        }
    }

    float3 radiance = radianceTexture.Sample(
        textureSampler,
        uv
    ) .xyz;
    //float3 prevRadiance = totalPrevRadiance / fTotalPrevWeight;
    float3 prevRadiance = prevRadianceTexture.Sample(
        textureSampler,
        prevUV
    ) .xyz;
    out.mRadiance.xyz = lerp(
        clamp(prevRadiance, minRadiance, maxRadiance),
        radiance,
        (1.0f / NUM_HISTORY)
    );
    out.mRadiance.w = 1.0f;

    return out;
}