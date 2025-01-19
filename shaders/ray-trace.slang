#define PI 3.14159f

#include "default-uniform-headers.slang"

struct RayPayload
{
    float4 mOutput;
};

[[vk::binding(0, 0)]] RaytracingAccelerationStructure scene;
[[vk::binding(1, 0)]] RWTexture2D<float4> renderTarget;
[[vk::binding(2, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(3, 0)]] Texture2D<float4> normalTexture;
[[vk::binding(4, 0)]] Texture2D<float4> previousHitCountRenderTarget;
[[vk::binding(5, 0)]] Texture2D<float4> motionVectorTexture;
[[vk::binding(6, 0)]] Texture2D<float4> prevWorldPositionTexture;
[[vk::binding(7, 0)]] Texture2D<float4> prevNormalTexture;
[[vk::binding(8, 0)]] Texture2D<float4> prevMotionVectorTexture;

[[vk::binding(0, 1)]] Texture2D<float4> blueNoiseTexture;
[[vk::binding(1, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

#include "motion-vector-headers.slang"

[shader("raygeneration")]
void rayGen()
{
    uint2 rayIndex = DispatchRaysIndex().xy;

    float4 worldPosition = worldPositionTexture[rayIndex];
    float3 normal = normalTexture[rayIndex].xyz;

    uint2 textureSize;
    blueNoiseTexture.GetDimensions(
        textureSize.x, 
        textureSize.y);

    int32_t iFrame = defaultUniformBuffer.miFrame;

    uint32_t iOffsetX = uint32_t(iFrame) % textureSize.x;
    uint32_t iOffsetY = (uint32_t(iFrame) / textureSize.x) % textureSize.y;

    uint2 screenCoord = uint2(
        (rayIndex.x + iOffsetX) % textureSize.x,
        (rayIndex.y + iOffsetY) % textureSize.y
    );

    float3 blueNoise = blueNoiseTexture[screenCoord].xyz;

    uint2 screenSize = uint2(
        defaultUniformBuffer.miScreenWidth,
        defaultUniformBuffer.miScreenHeight
    );

    if(worldPosition.w <= 0.0f)
    {
        renderTarget[rayIndex] = float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        float3 up = float3(0.0f, 1.0f, 0.0f);
        if (abs(normal.y) > 0.999f)
        {
            up = float3(1.0f, 0.0f, 0.0f);
        }
        float3 tangent = normalize(cross(up, normal));
        float3 binormal = normalize(cross(normal, tangent));

        // previous screen uv
        float2 prevScreenUV = getPreviousScreenUVRayTrace(
            rayIndex
        );

        float fNumHits = 0.0f;
        float fCount = 0.0f;

        uint2 prevScreenCoord = uint2(
            uint32_t(prevScreenUV.x * float(defaultUniformBuffer.miScreenWidth)),
            uint32_t(prevScreenUV.y * float(defaultUniformBuffer.miScreenHeight))
        );

        if (isDisoccluded2RayTrace(rayIndex, prevScreenCoord) == false &&
            isPrevUVOutOfBoundsRayTrace(rayIndex, screenSize) == false)
        {
            fNumHits = previousHitCountRenderTarget[prevScreenCoord].x;
            fCount = previousHitCountRenderTarget[prevScreenCoord].y;
        }

        float fPhi = 2.0f * PI * blueNoise.x;
        float fCosTheta = 1.0f - blueNoise.y;
        float fSinTheta = sqrt(1.0f - fCosTheta * fCosTheta);
        float3 h = float3(
            cos(fPhi) * fSinTheta,
            sin(fPhi) * fSinTheta,
            fCosTheta
        );

        float3 rayDirection = normalize(tangent * h.x + binormal * h.y + normal * h.z);

        RayDesc ray;
        float3 origin = float3(worldPosition.xyz);
        ray.Origin = origin;
        ray.Direction = rayDirection;
        ray.TMin = 0.001f;
        ray.TMax = 10000.0f;
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

        if (payload.mOutput.x > 0.0f)
        {
            fNumHits += 1.0f;
        }
        fCount += 1.0f;

        renderTarget[rayIndex] = float4(fNumHits, fCount, fNumHits / fCount, 1.0f);
//#endif // #if 0

#if 0
        uint32_t iNumLoops = 6;
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
                
                float3 rayDirection = normalize(tangent * h.x + binormal * h.y + normal * h.z);

                RayDesc ray;
                float3 origin = float3(worldPosition.xyz);
                ray.Origin = origin;
                ray.Direction = rayDirection; 
                ray.TMin = 0.001f;
                ray.TMax = 10000.0f;
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

                if(payload.mOutput.x > 0.0f)
                {
                    fNumHits += 1.0f;
                }
                fCount += 1.0f;
            }
        }

        float fHitPct = 1.0f - (fNumHits / fCount);
        renderTarget[rayIndex] = float4(fHitPct, fHitPct, fHitPct, 1.0f);
#endif // #if 0
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

    float fT = RayTCurrent();
    if(fT <= 1.0f / 8.0f)
    {
        payload.mOutput = float4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        payload.mOutput = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

[shader("miss")]
void missShader(
    inout RayPayload payload
)
{
    payload.mOutput = float4(0.0f, 0.0f, 1.0f, 1.0f);
}