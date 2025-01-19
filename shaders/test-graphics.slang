#define FLT_MAX  1.0e+10

#include "default-uniform-headers.slang"
#include "octahedron-map-headers.slang"

struct HashEntry
{
    uint32_t miPageCoordinate;
    uint32_t miPageIndex;
    uint32_t miTextureIDAndMIP;
    uint32_t miUpdateFrame;
};

struct TextureAtlasOutput
{
    float4 mColor;
    bool mbLoaded;
};

[[vk::binding(0, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(1, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(2, 0)]] Texture2D<float4> prevWorldPositionTexture;
[[vk::binding(3, 0)]] Texture2D<float4> prevNormalTexture;
[[vk::binding(4, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(5, 0)]] Texture2D<float4> prevMotionVectorTexture;
[[vk::binding(6, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformData;

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

    float2 uv = float2(
        in.texCoord.x,
        1.0f - in.texCoord.y 
    );

    float3 normal = normalTexture.Sample(
        textureSampler,
        uv
    ).xyz;

    float fDisocclusion = 0.0f;
    float2 prevScreenUV = getPreviousScreenUV(uv);
    if(isPrevUVOutOfBounds(uv))
    {
        fDisocclusion = 1.0f;
    }
    else
    {
        fDisocclusion = (isDisoccluded2(uv, prevScreenUV) == true) ? 1.0f : 0.0f;
    }

    // out.mOutput = float4(normal.xyz + float3(fDisocclusion, 0.0f, 0.0f), 1.0f);
    out.mOutput = float4(fDisocclusion, fDisocclusion, fDisocclusion, 1.0f);

    return out;
}

/*
**
*/
float2 getPreviousScreenUV(
    in float2 screenUV)
{
    float2 screenUVCopy = screenUV;
    
    float2 motionVector = motionVectorTexture.Sample(textureSampler, screenUV).xy;
    motionVector.xy = motionVector.xy * 2.0f - 1.0f;
    
    float2 prevScreenUV = screenUVCopy - motionVector;

    return prevScreenUV;
#if 0
    float3 worldPosition = worldPositionTexture.Sample(textureSampler, screenUV).xyz;
    float3 normal = normalTexture.Sample(textureSampler, screenUV).xyz;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    float fOneOverScreenWidth = 1.0f / float(textureSize.x);
    float fOneOverScreenHeight = 1.0f / float(textureSize.y);

    float fShortestWorldDistance = FLT_MAX;
    float2 closestScreenUV = prevScreenUV;

    for (int32_t iY = -2; iY <= 2; iY++)
    {
        for (int32_t iX = -2; iX <= 2; iX++)
        {
            float2 sampleUV = prevScreenUV + float2(
                float(iX) * fOneOverScreenWidth,
                float(iY) * fOneOverScreenHeight 
            );

            sampleUV.x = clamp(sampleUV.x, 0.0f, 1.0f);
            sampleUV.y = clamp(sampleUV.y, 0.0f, 1.0f);

            float3 checkWorldPosition = prevWorldPositionTexture.Sample(textureSampler, sampleUV).xyz;
            float3 checkNormal = prevNormalTexture.Sample(textureSampler, sampleUV).xyz;
            float fNormalDP = abs(dot(checkNormal, normal));

            float3 worldPositionDiff = checkWorldPosition - worldPosition;
            float fLengthSquared = dot(worldPositionDiff, worldPositionDiff);
            if (fNormalDP >= 0.99f && fShortestWorldDistance > fLengthSquared)
            {
                fShortestWorldDistance = fLengthSquared;
                closestScreenUV = sampleUV;
            }
        }
    }

    return closestScreenUV;
#endif // #if 0
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

    float3 worldPosition = worldPositionTexture.Sample(textureSampler, screenUV).xyz;
    float3 prevWorldPosition = prevWorldPositionTexture.Sample(textureSampler, prevScreenUV).xyz;
    float3 normal = normalTexture.Sample(textureSampler, screenUV).xyz;
    float3 prevNormal = prevNormalTexture.Sample(textureSampler, prevScreenUV).xyz;
    float4 motionVector = motionVectorTexture.Sample(textureSampler, screenUV);
    float4 prevMotionVectorAndMeshIDAndDepth = prevMotionVectorTexture.Sample(textureSampler, prevScreenUV);

    float fDepth = fract(motionVector.z);
    float fPrevDepth = fract(prevMotionVectorAndMeshIDAndDepth.z);

    uint32_t iMesh = uint32_t(motionVector.z - fDepth);
    uint32_t iPrevMesh = uint32_t(prevMotionVectorAndMeshIDAndDepth.z - fPrevDepth);

    float fCheckDepth = abs(fDepth - fPrevDepth);
    float3 worldPositionDiff = prevWorldPosition.xyz - worldPosition.xyz;
    float fCheckDP = abs(dot(normalize(normal.xyz), normalize(prevNormal.xyz)));
    float fCheckWorldPositionDistance = dot(worldPositionDiff, worldPositionDiff);

    // return !(iMesh == iPrevMesh && fCheckDepth <= 0.001f && fCheckWorldPositionDistance <= 0.001f && fCheckDP >= 0.999f);
    return !(iMesh == iPrevMesh && fCheckWorldPositionDistance <= 0.001f && fCheckDepth <= 0.01f && fCheckDP >= 0.99f);
}

/*
**
*/
bool isPrevUVOutOfBounds(
    in float2 inputTexCoord
)
{
    float4 motionVector = motionVectorTexture.Sample(
        textureSampler,
        inputTexCoord
    );
    motionVector.xy = motionVector.xy * 2.0f - 1.0f;

    float2 backProjectedScreenUV = inputTexCoord - motionVector.xy;
    return (backProjectedScreenUV.x < 0.0f || backProjectedScreenUV.x > 1.0 || backProjectedScreenUV.y < 0.0f || backProjectedScreenUV.y > 1.0f);
}