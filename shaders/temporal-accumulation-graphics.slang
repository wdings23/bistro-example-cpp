#include "default-uniform-headers.slang"

#define PI 3.14159f

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
[[vk::binding(3, 0)]] Texture2D<float4> prevWorldPositionTexture;
[[vk::binding(4, 0)]] Texture2D<float4> prevNormalTexture;
[[vk::binding(5, 0)]] Texture2D<float4> prevDiffuseRadianceTexture;
[[vk::binding(6, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(7, 0)]] Texture2D<float4> prevMotionVectorTexture;
[[vk::binding(8, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] Texture2D<float4> blueNoiseTexture;
[[vk::binding(1, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

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
        1.0f - in.texCoord.y 
    );

    out.mOutput = float4(temporalAccumulation(uv), 1.0f);

    return out;
}

/*
**
*/
float3 temporalAccumulation(float2 screenUV)
{
    float2 prevScreenUV = getPreviousScreenUV(screenUV);
    bool bDisoccluded = (isPrevUVOutOfBounds(screenUV) == true || isDisoccluded2(screenUV, prevScreenUV) == true);

    float3 prevDiffuseRadiance = prevDiffuseRadianceTexture.Sample(
        textureSampler,
        prevScreenUV
    ).xyz;

    float3 currDiffuseRadiance = diffuseRadianceTexture.Sample(
        textureSampler,
        screenUV
    ).xyz;

    float fSrcPct = 0.9f;
    float fDstPct = 0.1f;

    if(bDisoccluded)
    {
        fDstPct = 1.0f;
        fSrcPct = 0.0f;
    }

    float3 ret = prevDiffuseRadiance * fSrcPct + currDiffuseRadiance * fDstPct;

    return ret; 
}

/*
**
*/
float2 getPreviousScreenUV(
    in float2 screenUV)
{
    
    float3 motionVector = motionVectorTexture.Sample(
        textureSampler, 
        screenUV).xyz;
    motionVector.xy = motionVector.xy * 2.0f - 1.0f;
    float2 prevScreenUV = screenUV - motionVector.xy;

    return prevScreenUV;
}

/*
**
*/
bool isPrevUVOutOfBounds(
    in float2 screenUV
)
{
    float4 motionVector = motionVectorTexture.Sample(
        textureSampler,
        screenUV
    );
    motionVector.xy = motionVector.xy * 2.0f - 1.0f;
    float2 backProjectedScreenUV = screenUV - motionVector.xy;

    return (backProjectedScreenUV.x < 0.0f || backProjectedScreenUV.x > 1.0 || backProjectedScreenUV.y < 0.0f || backProjectedScreenUV.y > 1.0f);
}

/*
**
*/
bool isDisoccluded2(
    in float2 screenUV,
    in float2 prevScreenUV
)
{
    uint2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    float3 worldPosition = worldPositionTexture.Sample(textureSampler, screenUV).xyz;
    float3 prevWorldPosition = prevWorldPositionTexture.Sample(textureSampler, prevScreenUV).xyz;
    float3 normal = normalTexture.Sample(textureSampler, screenUV).xyz;
    float3 prevNormal = prevNormalTexture.Sample(textureSampler, prevScreenUV).xyz;
    float4 motionVector = motionVectorTexture.Sample(textureSampler, screenUV);
    float4 prevMotionVectorAndMeshIDAndDepth = prevMotionVectorTexture.Sample(textureSampler, prevScreenUV);

    // uint32_t iMesh = uint32_t(ceil(motionVector.w - 0.5f)) - 1;
    // float fDepth = motionVector.z;
    // float fPrevDepth = prevMotionVectorAndMeshIDAndDepth.z;

    float fDepth = fract(motionVector.z);
    float fPrevDepth = fract(prevMotionVectorAndMeshIDAndDepth.z);

    uint32_t iMesh = uint32_t(motionVector.z - fDepth);
    uint32_t iPrevMesh = uint32_t(prevMotionVectorAndMeshIDAndDepth.z - fPrevDepth);

    float fCheckDepth = abs(fDepth - fPrevDepth);
    float3 worldPositionDiff = prevWorldPosition.xyz - worldPosition.xyz;
    float fCheckDP = abs(dot(normalize(normal.xyz), normalize(prevNormal.xyz)));
    // uint32_t iPrevMesh = uint32_t(ceil(prevMotionVectorAndMeshIDAndDepth.w - 0.5f)) - 1;
    float fCheckWorldPositionDistance = dot(worldPositionDiff, worldPositionDiff);

    // return !(fCheckDP >= 0.99f && fCheckWorldPositionDistance <= 0.0001f && iMesh == iPrevMesh);

    return !(iMesh == iPrevMesh && fCheckDepth <= 0.001f && fCheckWorldPositionDistance <= 0.001f && fCheckDP >= 0.999f);
}