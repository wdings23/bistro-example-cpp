#define UINT32_MAX 0xffffffff
#define FLT_MAX  1.0e+10
#define PI 3.14159f

#include "default-uniform-headers.slang"
#include "motion-vector-headers.slang"

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
    float2 mMoment : SV_TARGET1;
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
[[vk::binding(8, 0)]] Texture2D<float4> screenMotionVectorTexture;
[[vk::binding(9, 0)]] Texture2D<float4> ambientOcclusionTexture;

[[vk::binding(10, 0)]] SamplerState textureSampler;

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

    float3 radiance = temporalAccumulation(uv);
    out.mOutput = float4(radiance, 1.0f);

    float fLuminance = computeLuminance(radiance);
    out.mMoment = float2(fLuminance, fLuminance * fLuminance);

    return out;
}

/*
**
*/
float3 temporalAccumulation(float2 screenUV)
{
    int2 outputTextureSize;
    diffuseRadianceTexture.GetDimensions(outputTextureSize.x, outputTextureSize.y);
    float fOneOverWidth = 1.0f / float(outputTextureSize.x);
    float fOneOverHeight = 1.0f / float(outputTextureSize.y);

    float2 motionVector = motionVectorTexture.Sample(
        textureSampler,
        screenUV
    ) .xy;
    motionVector = motionVector * 2.0f - 1.0f;
    float2 prevScreenUV = screenUV - motionVector;
    
    int2 screenCoord = int2(
        int32_t(floor(screenUV.x * float(outputTextureSize.x) + 0.5f)),
        int32_t(floor(screenUV.y * float(outputTextureSize.y) + 0.5f)) 
    );

    int2 prevScreenCoord = int2(
        int32_t(floor(prevScreenUV.x * float(outputTextureSize.x) + 0.5f)),
        int32_t(floor(prevScreenUV.y * float(outputTextureSize.y) + 0.5f))
    );

    // float4 prevScreenInfo = getPreviousScreenCoord(screenUV);
    // float2 prevScreenUV = float2(
    //     float(prevScreenInfo.x) * fOneOverWidth,
    //     float(prevScreenInfo.y) * fOneOverHeight
    //);
    bool bDisoccluded = false; //(isPrevUVOutOfBounds(screenUV) == true || isDisoccluded3(screenUV, prevScreenUV) == true);

    float3 prevDiffuseRadiance = prevDiffuseRadianceTexture.Sample(
        textureSampler,
        prevScreenUV
    ) .xyz;
    

    float3 minRadiance, maxRadiance;
    getMinMaxRadiance(
        minRadiance,
        maxRadiance,
        prevScreenCoord,
        screenCoord,
        outputTextureSize
    );

    prevDiffuseRadiance = clamp(prevDiffuseRadiance, minRadiance, maxRadiance);

    // clamp incoming radiance
    float3 currDiffuseRadiance = diffuseRadianceTexture[screenCoord].xyz;
    currDiffuseRadiance = clamp(currDiffuseRadiance, minRadiance, maxRadiance);

    float fDstPct = 1.0f / 20.0f;
    float fSrcPct = 1.0f - fDstPct;
    
    //if(bDisoccluded)
    //{
    //    fDstPct = 1.0f;
    //    fSrcPct = 0.0f;
    //}

    // float3 ret = lerp(prevDiffuseRadiance, currDiffuseRadiance, fDstPct) * ambientOcclusionTexture[screenCoord].z;
    float3 ret = lerp(prevDiffuseRadiance, currDiffuseRadiance, fDstPct);

    return ret; 
}

/*
**
*/
float computeLuminance(
    float3 radiance)
{
    return dot(radiance, float3(0.2126f, 0.7152f, 0.0722f));
}

/*
**
*/
float4 getPreviousScreenCoord2(
    in float2 screenUV)
{
    float2 screenUVCopy = screenUV;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    int2 screenCoord = uint2(
        int32_t(screenUV.x * float(textureSize.x)),
        int32_t(screenUV.y * float(textureSize.y)) 
    );

    float3 worldPosition = worldPositionTexture[screenCoord].xyz;
    float3 normal = normalTexture[screenCoord].xyz;

    float fOneOverScreenWidth = 1.0f / float(textureSize.x);
    float fOneOverScreenHeight = 1.0f / float(textureSize.y);

    float2 motionVector = screenMotionVectorTexture[screenCoord].xy;
    int2 prevScreenCoord = screenCoord - int2(int32_t(motionVector.x), int32_t(motionVector.y));

    return float4(float(screenCoord.x), float(screenCoord.y), 0.0f, 1.0f);
}

/*
**
*/
void getMinMaxRadiance(
    out float3 minRadiance,
    out float3 maxRadiance,
    in int2 prevScreenCoord,
    in int2 screenCoord,
    in int2 textureSize
)
{
    int32_t iRadius = 3;

    float fOneOverWidth = 1.0f / float(textureSize.x);
    float fOneOverHeight = 1.0f / float(textureSize.y);

    float fNumSamples = 0.0f;
    float3 radianceMean = float3(0.0f, 0.0f, 0.0f);
    float3 aDiffuseRadiance[9];
    for (int32_t iSampleY = -iRadius; iSampleY <= iRadius; iSampleY++)
    {
        for (int32_t iSampleX = -iRadius; iSampleX <= iRadius; iSampleX++)
        {
            float2 sampleUV = float2(
                float(screenCoord.x + iSampleX) * fOneOverWidth,
                float(screenCoord.y + iSampleY) * fOneOverHeight
            );

            aDiffuseRadiance[int(fNumSamples)] = diffuseRadianceTexture.Sample(
                textureSampler,
                sampleUV
            ) .xyz;
            radianceMean += aDiffuseRadiance[int(fNumSamples)];
            fNumSamples += 1.0f;
        }
    }
    float fOneOverNumSamples = 1.0f / fNumSamples;
    radianceMean *= fOneOverNumSamples;

    uint32_t iIndex = 0;
    float3 totalDiffSquared = float3(0.0f, 0.0f, 0.0f);
    for (int32_t iSampleY = -iRadius; iSampleY <= iRadius; iSampleY++)
    {
        for (int32_t iSampleX = -iRadius; iSampleX <= iRadius; iSampleX++)
        {
            float3 diff = aDiffuseRadiance[iIndex] - radianceMean;
            totalDiffSquared += diff * diff;
            iIndex += 1;
        }
    }

    float3 standardDeviation = sqrt((totalDiffSquared) * fOneOverNumSamples);

    minRadiance = max(radianceMean - standardDeviation * 0.25f, float3(0.0f, 0.0f, 0.0f));
    maxRadiance = max(radianceMean + standardDeviation * 0.25f, float3(0.0f, 0.0f, 0.0f)); 
}

/*
**
*/
bool isDisoccluded3(
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

    return (fCheckWorldPositionDistance > 0.01f/* && fCheckDP < 0.99f*/);
}