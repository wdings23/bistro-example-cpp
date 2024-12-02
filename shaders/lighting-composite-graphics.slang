#define PI 3.14159f

#include "octahedron-map-headers.slang"

struct DefaultUniforms
{
    uint        miScreenWidth;
    uint        miScreenHeight;
    int         miFrame;
    uint        miNumMeshes;

    float       mfRand0;
    float       mfRand1;
    float       mfRand2;
    float       mfRand3;

    float4x4    mViewProjectionMatrix;
    float4x4    mPrevViewProjectionMatrix;
    float4x4    mViewMatrix;
    float4x4    mProjectionMatrix;
    float4x4    mJitteredViewProjectionMatrix;
    float4x4    mPrevJitteredViewProjectionMatrix;

    float4      mCameraPosition;
    float4      mCameraLookDir;

    float4      mLightRadiance;
    float4      mLightDirection;
	float4      mfAmbientOcclusionDistanceThreshold;
};

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

[[vk::binding(0, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(1, 0)]] Texture2D<float4> aoTexture;
[[vk::binding(2, 0)]] Texture2D<float4> shadowTexture;
[[vk::binding(3, 0)]] Texture2D<float4> skyTexture;
[[vk::binding(4, 0)]] Texture2D<float4> sunLightTexture;
[[vk::binding(5, 0)]] SamplerState sampler;
[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniforms> defaultUniformData;

[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    float4 position = assembledVertex.position;
    float4 uv = assembledVertex.uv;
    uv.y = 1.0f - uv.y;

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

    float3 normal = normalTexture.Sample(
        sampler,
        texCoord
    ).xyz;

    float3 ao = aoTexture.Sample(
        sampler,
        texCoord
    ).xyz;

    float3 shadow = shadowTexture.Sample(
        sampler,
        texCoord
    ).xyz;

    float3 up = float3(0.0f, 1.0f, 0.0f);
    if (abs(normal.y) > 0.999f)
    {
        up = float3(1.0f, 0.0f, 0.0f);
    }
    float3 tangent = normalize(cross(up, normal));
    float3 binormal = normalize(cross(normal, tangent));

    float3 totalAmbient = float3(0.0f, 0.0f, 0.0f);
    uint32_t iNumLoops = 6;
    float fCount = 0.0f;
    for (uint32_t iY = 0; iY < iNumLoops; iY++)
    {
        float fPctY = float(iY) / float(iNumLoops);
        for (uint32_t iX = 0; iX < iNumLoops; iX++)
        {
            float fPctX = float(iX) / float(iNumLoops);

            float fPhi = 2.0f * PI * fPctY;
            float fCosTheta = 1.0f - fPctX;
            float fSinTheta = sqrt(1.0f - fCosTheta * fCosTheta);
            float3 h = float3(
                cos(fPhi) * fSinTheta,
                sin(fPhi) * fSinTheta,
                fCosTheta);
            float3 direction = normalize(tangent * h.x + binormal * h.y + normal * h.z);

            float2 octahedronUV = octahedronMap2(
                direction
            );

            totalAmbient += skyTexture.Sample(
                sampler,
                octahedronUV
            ).xyz;

            fCount += 1.0f;
        }
    }

    totalAmbient /= fCount;
    //if(shadow.x <= 0.0f)
    //{
    //    totalAmbient *= 0.4f;
    //}

    float2 octahedronUV = octahedronMap2(
        normal
    );
    float3 sunLight = sunLightTexture.Sample(
        sampler,
        octahedronUV
    ) .xyz;

    float3 ret = (shadow * sunLight + totalAmbient) * ao;
    return float4(ret.xyz, 1.0f); 
}