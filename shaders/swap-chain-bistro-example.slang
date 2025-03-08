#include "default-uniform-headers.slang"
#include "mesh-culling-headers.slang"
#include "tone-mapping-headers.slang"

[[vk::binding(0, 0)]] Texture2D<float4> texture0;
[[vk::binding(1, 0)]] Texture2D<float4> texture1;
[[vk::binding(2, 0)]] SamplerState sampler;
[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

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

#if defined(METAL)
    float2 texCoord = float2(in.texCoord.x, in.texCoord.y);
#else 
    float2 texCoord = float2(in.texCoord.x, 1.0f - in.texCoord.y);
#endif // METAL



    out.mOutput.xyz =
        texture0.Sample(sampler, texCoord).xyz +
        texture1.Sample(sampler, texCoord).xyz;

    //out.mOutput.xyz = ACESFilm(
    //    clamp(out.mOutput.xyz, 0.0f, 1.0f)
    //);
    out.mOutput.w = 1.0f;

    //float3 sdrColor = ACESFilm(out.mOutput.xyz);
    //out.mOutput.x = sdrColor.x;
    //out.mOutput.y = sdrColor.y;
    //out.mOutput.z = sdrColor.z;
    //out.mOutput.w = 1.0f;

    return out;
}


