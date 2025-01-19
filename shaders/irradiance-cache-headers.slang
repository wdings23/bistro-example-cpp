#pragma once

struct IrradianceCacheEntry
{
    float4 mPosition;
    float4 mSampleCount;
    float4 mImageProbe[64]; // 8x8 image probe
};

/*
**
*/
uint32_t fetchIrradianceCacheIndex(
    in float3 position
)
{
    float3 scaledPosition = position * 10.0f;
    float fSignX = sign(position.x);
    float fSignY = sign(position.y);
    float fSignZ = sign(position.z);
    scaledPosition.x = float(floor(abs(position.x) + 0.5f)) * 0.1f * fSignX; 
    scaledPosition.y = float(floor(abs(position.y) + 0.5f)) * 0.1f * fSignY; 
    scaledPosition.z = float(floor(abs(position.z) + 0.5f)) * 0.1f * fSignZ; 

    uint32_t iHashKey = hash13(
        scaledPosition,
        5000u
    );

    return iHashKey;
}

/*
**
*/
/////
float3 getRadianceFromIrradianceCacheProbe(
    in float3 rayDirection,
    in uint32_t iIrradianceCacheIndex
)
{
    float2 probeImageUV = octahedronMap2(rayDirection);
    uint32_t iImageY = clamp(uint32_t(probeImageUV.y * float(PROBE_IMAGE_SIZE)), 0u, PROBE_IMAGE_SIZE - 1u);
    uint32_t iImageX = clamp(uint32_t(probeImageUV.x * float(PROBE_IMAGE_SIZE)), 0u, PROBE_IMAGE_SIZE - 1u);
    uint32_t iImageIndex = iImageY * PROBE_IMAGE_SIZE + iImageX;

    return irradianceCache[iIrradianceCacheIndex].mImageProbe[iImageIndex].xyz;
}
