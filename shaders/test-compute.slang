#define FLT_MAX  1.0e+10

#include "default-uniform-headers.slang"
#include "octahedron-map-headers.slang"
#include "motion-vector-headers.slang"

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

struct VertexFormat
{
    float4 mPosition;
    float4 mTexCoord;
    float4 mNormal;
};

[[vk::binding(0, 0)]] RWTexture2D<float4> outputTexture;
[[vk::binding(1, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(2, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(3, 0)]] Texture2D<float4> prevWorldPositionTexture;
[[vk::binding(4, 0)]] Texture2D<float4> prevNormalTexture;
[[vk::binding(5, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(6, 0)]] Texture2D<float4> prevMotionVectorTexture;
[[vk::binding(7, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] StructuredBuffer<VertexFormat> vertexBuffer;
[[vk::binding(1, 1)]] StructuredBuffer<uint32_t> indexBuffer;
[[vk::binding(2, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

#define NUM_THREADS 256

[shader("compute")]
[numthreads(NUM_THREADS, 1, 1)]
void CSMain(
    uint3 localThread : SV_GroupThreadID,
    uint3 workGroup : SV_GroupID)
{
    uint2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);
    int32_t iTotalThreadID = localThread.x + workGroup.x * NUM_THREADS;

    int32_t iX = iTotalThreadID % textureSize.x;
    int32_t iY = iTotalThreadID / textureSize.y;

    if(iX >= textureSize.x || iY >= textureSize.y)
    {
        return;
    }

    int2 texCoord = int2(iX, iY);

    float2 motionVector = motionVectorTexture[texCoord].xy;

    float fOneOverWeight = 1.0f / float(textureSize.x);
    float fOneOverHeight = 1.0f / float(textureSize.y);

    float fDisocclusion = 0.0f;
    int2 prevTexCoord = getPreviousScreenCoord(texCoord);
    if(isPrevUVOutOfBounds(texCoord))
    {
        fDisocclusion = 1.0f;
    }
    else
    {
        float2 uv = float2(
            float(texCoord.x) * fOneOverWeight,
            float(texCoord.y) * fOneOverHeight
        );

        float2 prevUV = float2(
            float(prevTexCoord.x) * fOneOverWeight,
            float(prevTexCoord.y) * fOneOverHeight
        );

        fDisocclusion = (isDisoccludedRayTrace2(texCoord, prevTexCoord) == true) ? 1.0f : 0.0f;
    }

    outputTexture[texCoord] = float4(fDisocclusion, fDisocclusion, fDisocclusion, vertexBuffer[indexBuffer[0]].mPosition.x);
}

/*
**
*/
int2 getPreviousScreenCoord(
    in int2 screenCoord)
{
    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    float2 screenUV = float2( 
        float(screenCoord.x) / float(textureSize.x),
        float(screenCoord.y) / float(textureSize.y)
    );
    float2 motionVector = motionVectorTexture[screenCoord].xy;
    float2 prevScreenUV = screenUV - motionVector;

    float3 worldPosition = worldPositionTexture[screenCoord].xyz;
    float3 normal = normalTexture[screenCoord].xyz;

    float fOneOverScreenWidth = 1.0f / float(textureSize.x);
    float fOneOverScreenHeight = 1.0f / float(textureSize.y);

    float fShortestWorldDistance = FLT_MAX;
    int2 closestCoord = screenCoord;

    for(int32_t iY = -2; iY <= 2; iY++)
    {
        for(int32_t iX = -2; iX <= 2; iX++)
        {
            int2 sampleCoord = int2(
                screenCoord.x + iX,
                screenCoord.y + iY 
            );

            float3 checkWorldPosition = prevWorldPositionTexture[sampleCoord].xyz;
            float3 checkNormal = prevNormalTexture[sampleCoord].xyz;
            float fNormalDP = abs(dot(checkNormal, normal));

            float3 worldPositionDiff = checkWorldPosition - worldPosition;
            float fLengthSquared = dot(worldPositionDiff, worldPositionDiff);
            if (fNormalDP >= 0.99f && fShortestWorldDistance > fLengthSquared)
            {
                fShortestWorldDistance = fLengthSquared;
                closestCoord = sampleCoord;
            }
        }
    }

    return closestCoord;
}
