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
[[vk::binding(3, 0)]] SamplerState textureSampler;

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
    float fRadius = 3.0f;

    uint2 textureSize;
    blueNoiseTexture.GetDimensions(textureSize.x, textureSize.y);
    uint32_t iTotalTextureSize = textureSize.x * textureSize.y;
    uint32_t iTileSize = 32u;
    uint32_t iNumTilesPerRow = textureSize.x / iTileSize;
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
    
    float2 screenCoord = float2(
        uint32_t(texCoord.x * float(defaultUniformBuffer.miScreenWidth)),
        uint32_t(texCoord.y * float(defaultUniformBuffer.miScreenHeight))
    );

    float2 oneOverScreenDimension = float2(
        1.0f / float(defaultUniformBuffer.miScreenWidth),
        1.0f / float(defaultUniformBuffer.miScreenHeight)
    );

    int32_t iNumSamples = 24;
    float2 screenUV = texCoord.xy;
    float fTotalWeight = 1.0f;
    uint32_t iCurrIndex = uint32_t(defaultUniformBuffer.miFrame) * uint32_t(iNumSamples);
    for(int32_t iSample = 0; iSample < iNumSamples; iSample++)
    {
        uint32_t iTileX = (iCurrIndex + uint32_t(iSample)) % iNumTilesPerRow;
        uint32_t iTileY = ((iCurrIndex + uint32_t(iSample)) / iNumTilesPerRow) % (iNumTilesPerRow * iNumTilesPerRow);

        uint32_t iTileOffsetX = (iCurrIndex + uint32_t(iSample)) % iTileSize;
        uint32_t iTileOffsetY = ((iCurrIndex + uint32_t(iSample)) / iTileSize) % (iTileSize * iTileSize);

        uint32_t iOffsetX = iTileOffsetX + iTileX * iTileSize;
        uint32_t iOffsetY = iTileOffsetY + iTileY * iTileSize; 

        screenCoord.x = (screenCoord.x + iOffsetX) % textureSize.x;
        screenCoord.y = (screenCoord.y + iOffsetY) % textureSize.y;
        var sampleBlueNoiseUV = float2(
            float(screenCoord.x) / float(textureSize.x),
            float(screenCoord.y) / float(textureSize.y) 
        );

        var blueNoise = blueNoiseTexture.Sample(
            textureSampler,
            sampleBlueNoiseUV
        ).xyz;

        let fTheta = blueNoise.x * 2.0f * PI;
        let fCosTheta = cos(fTheta);
        let fSinTheta = sin(fTheta);

        float kfRadiusMultSpeed = 100.0f;

        //float fCurrRadius = max(
        //    fRadius * ((kfRadiusMultSpeed - min(ret.w, (kfRadiusMultSpeed - 1.0f))) / kfRadiusMultSpeed),
        //    4.0f);

        float fCurrRadius = fRadius;

        float fSampleRadius = (fCurrRadius * max(blueNoise.y, 0.7f));
        float2 offset = float2(
            fCosTheta * fSampleRadius,
            fSinTheta * fSampleRadius
        );
        float2 uvOffset = offset * oneOverScreenDimension;

        float2 sampleUV = screenUV + uvOffset;
        float4 sampleWorldPosition = worldPositionTexture.Sample(
            textureSampler,
            sampleUV
        );
        float3 sampleNormal = normalTexture.Sample(
            textureSampler,
            sampleUV
        ).xyz;
        float3 diff = sampleWorldPosition.xyz - worldPosition.xyz;

        uint32_t iSampleMesh = uint32_t(sampleWorldPosition.w - fract(sampleWorldPosition.w));

        float fPlaneDistance = abs(dot(diff, normal));
        float fDP = dot(sampleNormal, normal);
        if(/*length(diff) > kfPositionThreshold ||*/
            abs(fDP) < kfAngleThreshold || 
            iSampleMesh != iMesh)
        {
            continue;
        }

        float3 radiance = diffuseRadianceTexture.Sample(
            textureSampler,
            sampleUV  
        ).xyz;
        
        float fWeight = 1.0f; 
        ret.x += radiance.x * fWeight;
        ret.y += radiance.y * fWeight;
        ret.z += radiance.z * fWeight;
        fTotalWeight += fWeight;
    }

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
