#include "mesh-culling-headers.slang"
#include "vertex-format-headers.slang"

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

[[vk::binding(0, 1)]] RWStructuredBuffer<Material> aMaterials;
[[vk::binding(1, 1)]] RWStructuredBuffer<uint32_t> aMeshMaterialID;
[[vk::binding(2, 1)]] RWStructuredBuffer<Range> aMeshTriangleIndexRanges;
[[vk::binding(3, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformData;

struct PSOutput
{
    float4 mWorldPosition : SV_TARGET0;
    float4 mTexCoord : SV_TARGET1;
    float4 mNormal : SV_TARGET2;
};

struct VertexStageOutput
{
    float4 sv_position : SV_Position;
    float4 worldPosition;
    float4 texCoord;
    float4 normal;
};

[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    float4x4 viewMatrix = defaultUniformData.mViewMatrix;
    float4x4 projectionMatrix = defaultUniformData.mProjectionMatrix;

    projectionMatrix._22 *= -1.0f;

    float4x4 viewProjectionMatrix = mul(
        projectionMatrix,
        viewMatrix
    );

    output.sv_position = mul(
        viewProjectionMatrix,
        float4(
            assembledVertex.position.x,
            assembledVertex.position.y,
            assembledVertex.position.z,
            1.0f),
    );
    output.worldPosition = assembledVertex.position;
    output.texCoord = assembledVertex.texCoord;
    output.normal = assembledVertex.normal;

    return output;
}

[shader("fragment")]
PSOutput PSMain(VertexStageOutput in)
{
    PSOutput out;

    uint32_t iMesh = uint32_t(ceil(in.worldPosition.w - 0.5f));
    out.mWorldPosition = float4(in.worldPosition.xyz, float(iMesh));
    out.mTexCoord = float4(in.texCoord.x, in.texCoord.y, 0.0f, 1.0f);
    float3 normalXYZ = normalize(in.normal.xyz);
    out.mNormal.x = normalXYZ.x;
    out.mNormal.y = normalXYZ.y;
    out.mNormal.z = normalXYZ.z;
    out.mNormal.w = 1.0f;

    float2 texCoord = in.texCoord.xy;
    if(texCoord.x < 0.0f)
    {
        texCoord.x = fract(abs(1.0f - texCoord.x));
    }
    else if(texCoord.x > 1.0f)
    {
        texCoord.x = fract(texCoord.x);
    }

    if(texCoord.y < 0.0f)
    {
        texCoord.y = fract(abs(1.0f - texCoord.y));
    }
    else if(texCoord.y > 1.0f)
    {
        texCoord.y = fract(texCoord.y);
    }

    out.mTexCoord = float4(texCoord.x, texCoord.y, float(iMesh), 1.0f);

    out.mWorldPosition.x = in.worldPosition.x;
    out.mWorldPosition.y = in.worldPosition.y;
    out.mWorldPosition.z = in.worldPosition.z;
    out.mWorldPosition.w = 1.0f; //in.sv_position.z;

    // store depth and mesh id in worldPosition.w
    //out.mWorldPosition.w = clamp(in.worldPosition.z, 0.0f, 0.999f) + floor(in.worldPosition.w + 0.5f);

    return out;
}