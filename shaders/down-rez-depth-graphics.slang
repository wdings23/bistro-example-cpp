#include "mesh-culling-headers.slang"

[[vk::binding(0, 0)]] Texture2D<float> depthTexture;
[[vk::binding(1, 0)]] SamplerState sampler;

struct AssembledVertex
{
    float4 position : POSITION;
    float4 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 mDepthOutput : SV_TARGET0;
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
    output.texCoord = float4(
        assembledVertex.texCoord.x,
        1.0f - assembledVertex.texCoord.y,
        0.0f,
        0.0f
    );

    return output;
}

[shader("fragment")]
PSOutput PSMain(VertexStageOutput in)
{
    PSOutput out;

    float4 depth = depthTexture.Gather(
        sampler,
        in.texCoord.xy
    );

    float fMaxDepth = max(max(max(depth.x, depth.y), depth.z), depth.w);
    out.mDepthOutput = float4(fMaxDepth, fMaxDepth, fMaxDepth, 1.0f);

    return out;
}
