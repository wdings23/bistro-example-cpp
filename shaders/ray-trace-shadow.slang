#define PI 3.14159f

#include "default-uniform-headers.slang"
#include "octahedron-map-headers.slang"

[[vk::binding(0, 0)]] RaytracingAccelerationStructure scene;
[[vk::binding(1, 0)]] RWTexture2D<float4> renderTarget;
[[vk::binding(2, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(3, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(4, 0)]] Texture2D<float4> sunLightTexture;
[[vk::binding(5, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer; 

struct RayPayload
{
    float4 mOutput;
};

[shader("raygeneration")]
void rayGen()
{
    uint2 rayIndex = DispatchRaysIndex().xy;

    float4 worldPosition = worldPositionTexture[rayIndex];
    float3 normal = normalTexture[rayIndex].xyz;
    float3 lightDirection = normalize(float3(-0.5f, 1.0f, 0.0f));

    renderTarget[rayIndex] = float4(0.0f, 0.0f, 0.0f, 0.0f);
    if(worldPosition.w <= 0.0f)
    {
        renderTarget[rayIndex] = float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        float fNumHit = 0.0f;
        float fCount = 0.0f;
        uint32_t iNumLoops = 6;
        
        RayDesc ray;
        float3 origin = float3(worldPosition.xyz);
        ray.Origin = origin;
        ray.Direction = normalize(defaultUniformBuffer.mLightDirection.xyz); 
        ray.TMin = 0.01f;
        ray.TMax = 1000.0f;
        RayPayload payload = { 
            float4(0.0f, 0.0f, 0.0f, 0.0f)
        };

        TraceRay(
            scene,
            RAY_FLAG_FORCE_OPAQUE,
            0xff,
            0,
            0,
            0,
            ray,
            payload
        );

        float fDP = max(dot(normal, ray.Direction), 0.0f);
        float4 radianceOutput = (payload.mOutput.x <= 0.0f) ? 
            //sunLightTexture[int2(0, 0)].xyz :
            float4(1.0f, 1.0f, 1.0f, 1.0f) * float4(fDP, fDP, fDP, 1.0f) :
            float4(0.01f, 0.01f, 0.01f, 1.0f);
        renderTarget[rayIndex] = radianceOutput;
    }
}

[shader("closesthit")]
void hitTriangle(
    inout RayPayload payload,
    in BuiltInTriangleIntersectionAttributes attr
)
{
    float3 barycentrics = float3(1.0f - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    //payload.mOutput = float4(barycentrics, 1.0f);

    payload.mOutput = float4(0.0f, 0.0f, 0.0f, 1.0f);
}

[shader("miss")]
void missShader(
    inout RayPayload payload
)
{
    payload.mOutput = float4(1.0f, 1.0f, 1.0f, 1.0f);
}

/*
**
*/
float3 getSunLight(
    in float3 rayDirection)
{
    float2 skyUV = octahedronMap2(rayDirection);

    return sunLightTexture.Sample(
        textureSampler,
        skyUV
    ) .xyz;
}
