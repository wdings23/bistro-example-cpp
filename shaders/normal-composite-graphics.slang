#include "default-uniform-headers.slang"

[[vk::binding(0, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(1, 0)]] Texture2D<float4> normalMapTexture;
[[vk::binding(2, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformData; 

struct AssembledVertex
{
    float4 position : POSITION;
    float4 texCoord : TEXCOORD;
};

struct PSOutput
{
    float4 mOutput0 : SV_TARGET0;
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

    float3 normalMap = normalMapTexture.Sample(
        textureSampler,
        uv
    ) .xyz;

    float3 up = (abs(normal.y) > 0.99f) ? float3(1.0f, 0.0f, 0.0f) : float3(0.0f, 1.0f, 0.0f);
    float3 tangent = normalize(cross(up, normal));
    float3 binormal = normalize(cross(normal, tangent));
    float3 xformNormal = tangent * normalMap.x + binormal * normalMap.y + normal * normalMap.z;

    out.mOutput0 = float4(xformNormal.xyz, 1.0f);

    return out;
}