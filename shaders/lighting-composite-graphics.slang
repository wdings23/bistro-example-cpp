#define PI 3.14159f

#include "octahedron-map-headers.slang"

#include "default-uniform-headers.slang"

// Per-vertex attributes to be assembled from bound vertex buffers.
struct AssembledVertex
{
    float4 position : POSITION;
    float4 uv : TEXCOORD;
};

// Output of the fragment shader
struct Fragment
{
    float4 color;
};

// Vertex  Shader

struct VertexStageOutput
{
    float4          sv_position     : SV_Position;
    float4          texcoord;
};

[[vk::binding(0, 0)]] Texture2D<float4> diffuseRadianceTexture;
[[vk::binding(1, 0)]] Texture2D<float4> specularRadianceTexture;
[[vk::binding(2, 0)]] Texture2D<float4> emissiveRadianceTexture;
[[vk::binding(3, 0)]] Texture2D<float4> shadowTexture;
[[vk::binding(4, 0)]] Texture2D<float4> skyTexture;
[[vk::binding(5, 0)]] Texture2D<float4> albedoTexture;
[[vk::binding(6, 0)]] Texture2D<float4> texCoordTexture;
[[vk::binding(7, 0)]] Texture2D<float4> reservoirTexture;
[[vk::binding(8, 0)]] Texture2D<float> ambientOcclusionTexture;

[[vk::binding(9, 0)]] Texture2D<float4> currSpecularRadianceTexture;

[[vk::binding(10, 0)]] SamplerState sampler;

[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    float4 position = assembledVertex.position;
    float4 uv = assembledVertex.uv;

    output.sv_position = float4(position.xyz, 1.0f);
    output.texcoord = uv;

    return output;
}

// Fragment Shader

[shader("fragment")]
float4 PSMain(
    VertexStageOutput input) : SV_Target
{
    float2 texCoord = input.texcoord.xy;
    float4 sampleTexCoord = texCoordTexture.Sample(
        sampler,
        float2(input.texcoord.x, input.texcoord.y)
    );

    float3 albedo = albedoTexture.Sample(
        sampler,
        texCoord
    ) .xyz;

    float fAO = ambientOcclusionTexture.Sample(
        sampler,
        texCoord
    );

    float3 ret = float3(0.0f, 0.0f, 0.0f);
    if (sampleTexCoord.w <= 0.0f)
    {
        ret = albedo.xyz;
    }
    else
    {
        float fReservoirWeight = reservoirTexture.Sample(
            sampler,
            texCoord
        ) .w;

        float3 diffuseRadiance = diffuseRadianceTexture.Sample(
            sampler,
            texCoord
        ).xyz;

        float3 specularRadiance = specularRadianceTexture.Sample(
            sampler,
            texCoord
        ) .xyz;

        float3 emissiveRadiance = emissiveRadianceTexture.Sample(
            sampler,
            texCoord
        ) .xyz;

        float3 shadow = shadowTexture.Sample(
            sampler,
            texCoord
        ).xyz;

        float fKD = currSpecularRadianceTexture.Sample(
            sampler, 
            texCoord).w;

        ret = (shadow + diffuseRadiance * (1.0f - fKD) + specularRadiance) * albedo * fAO + emissiveRadiance;
    }
    return float4(ret.xyz, 1.0f); 
}