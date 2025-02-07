#pragma once

struct IrradianceCacheEntry
{
    float4 mPosition;
    float4 mSampleCount;
    float4 mSphericalHarmonics0;
    float4 mSphericalHarmonics1;
    float4 mSphericalHarmonics2;
};

/*
**
*/
uint32_t fetchIrradianceCacheIndex(
    in float3 position
)
{
    float3 scaledPosition = position;
    float fSignX = sign(position.x);
    float fSignY = sign(position.y);
    float fSignZ = sign(position.z);
    scaledPosition.x = float(floor(abs(scaledPosition.x) + 0.5f)) * 0.1f * fSignX;
    scaledPosition.y = float(floor(abs(scaledPosition.y) + 0.5f)) * 0.1f * fSignY;
    scaledPosition.z = float(floor(abs(scaledPosition.z) + 0.5f)) * 0.1f * fSignZ; 

    uint32_t iHashKey = hash13(
        scaledPosition,
        50000u
    );

    return iHashKey;
}

/*
**
*/
/////
float3 getRadianceFromIrradianceCacheProbe(
    in float3 direction,
    in uint32_t iIrradianceCacheIndex
)
{
    //float2 probeImageUV = octahedronMap2(rayDirection);
    //uint32_t iImageY = clamp(uint32_t(probeImageUV.y * float(PROBE_IMAGE_SIZE)), 0u, PROBE_IMAGE_SIZE - 1u);
    //uint32_t iImageX = clamp(uint32_t(probeImageUV.x * float(PROBE_IMAGE_SIZE)), 0u, PROBE_IMAGE_SIZE - 1u);
    //uint32_t iImageIndex = iImageY * PROBE_IMAGE_SIZE + iImageX;
    //return irradianceCache[iIrradianceCacheIndex].mImageProbe[iImageIndex].xyz;

    float4 SHCoefficent0 = irradianceCache[iIrradianceCacheIndex].mSphericalHarmonics0;
    float4 SHCoefficent1 = irradianceCache[iIrradianceCacheIndex].mSphericalHarmonics1;
    float4 SHCoefficent2 = irradianceCache[iIrradianceCacheIndex].mSphericalHarmonics2;

    float3 aTotalCoefficients[4];
    float fFactor = 1.0f / max(irradianceCache[iIrradianceCacheIndex].mSampleCount.x, 1.0f);

    aTotalCoefficients[0] = float3(SHCoefficent0.x, SHCoefficent0.y, SHCoefficent0.z) * fFactor;
    aTotalCoefficients[1] = float3(SHCoefficent0.w, SHCoefficent1.x, SHCoefficent1.y) * fFactor;
    aTotalCoefficients[2] = float3(SHCoefficent1.z, SHCoefficent1.w, SHCoefficent2.x) * fFactor;
    aTotalCoefficients[3] = float3(SHCoefficent2.y, SHCoefficent2.z, SHCoefficent2.w) * fFactor;

    float fC1 = 0.42904276540489171563379376569857f;
    float fC2 = 0.51166335397324424423977581244463f;
    float fC3 = 0.24770795610037568833406429782001f;
    float fC4 = 0.88622692545275801364908374167057f;

    float3 decoded =
        aTotalCoefficients[0] * fC4 +
        (aTotalCoefficients[3] * direction.x + aTotalCoefficients[1] * direction.y + aTotalCoefficients[2] * direction.z) *
            fC2 * 2.0f;

    return decoded;
}
