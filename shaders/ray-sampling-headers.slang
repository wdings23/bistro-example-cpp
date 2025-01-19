#pragma once


struct Ray
{
    float4 mOrigin;
    float4 mDirection;
    float4 mfT;
};

/*
**
*/
Ray uniformSampling(
    float3 worldPosition,
    float3 normal,
    float fRand0,
    float fRand1)
{
    float fPhi = 2.0f * PI * fRand0;
    float fCosTheta = 1.0f - fRand1;
    float fSinTheta = sqrt(1.0f - fCosTheta * fCosTheta);
    float3 h = float3(
        cos(fPhi) * fSinTheta,
        sin(fPhi) * fSinTheta,
        fCosTheta);

    float3 up = float3(0.0f, 1.0f, 0.0f);
    if(abs(normal.y) > 0.999f)
    {
        up = float3(1.0f, 0.0f, 0.0f);
    }
    float3 tangent = normalize(cross(up, normal));
    float3 binormal = normalize(cross(normal, tangent));
    float3 rayDirection = normalize(tangent * h.x + binormal * h.y + normal * h.z);

    var ray: Ray;
    ray.mOrigin = float4(worldPosition + normal * 0.05f, 1.0f);
    ray.mDirection = float4(rayDirection, 1.0f);
    ray.mfT = float4(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);

    return ray;
}

