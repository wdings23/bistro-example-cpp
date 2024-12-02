#define PI 3.14159f

[[vk::binding(0, 0)]] RaytracingAccelerationStructure scene;
[[vk::binding(1, 0)]] RWTexture2D<float4> renderTarget;
[[vk::binding(2, 0)]] Texture2D<float4> worldPositionTexture;
[[vk::binding(3, 0)]] Texture2D<float4> normalTexture;

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

        float fNumHit = 0.0f;
        float fCount = 0.0f;
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
                    fNumHit += 1.0f;
                }
                fCount += 1.0f;
            }
        }

        float fHitPct = 1.0f - (fNumHit / fCount);
        renderTarget[rayIndex] = float4(fHitPct, fHitPct, fHitPct, 1.0f);
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