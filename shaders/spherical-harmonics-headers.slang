// https://media.contentapi.ea.com/content/dam/eacom/frostbite/files/gdc2018-precomputedgiobalilluminationinfrostbite.pdf
// http://orlandoaguilar.github.io/sh/spherical/harmonics/irradiance/map/2017/02/12/SphericalHarmonics.html
#if 0
/*
**
*/
void encodeToSphericalHarmonicCoefficients(
    in float3 radiance,
    in float3 direction,
    in float2 texCoord,
    in float2 prevTexCoord,
    in bool bDisoccluded,
    in float fNumSamples
)
{
    // float fPct = (fNumSamples <= 20.0f) ? 1.0f / 20.0f : 10.0f / fNumSamples;
    //float fPct = 0.001f;
    //if(fNumSamples >= 10.0f)
    //{
    //    fPct = 10.0f / fNumSamples;
    //}

    uint2 textureSize;
    radianceOutput.GetDimensions(textureSize.x, textureSize.y);

    uint2 prevCoord = uint2(
        uint32_t(prevTexCoord.x * float(textureSize.x)),
        uint32_t(prevTexCoord.y * float(textureSize.y))
    );

    float4 SHCoefficent0 = prevSphericalHarmonicCoefficientTexture0[prevCoord];
    float4 SHCoefficent1 = prevSphericalHarmonicCoefficientTexture1[prevCoord];
    float4 SHCoefficent2 = prevSphericalHarmonicCoefficientTexture2[prevCoord];

    if(bDisoccluded)
    {
        SHCoefficent0 = float4(0.0f, 0.0f, 0.0f, 0.0f);
        SHCoefficent1 = float4(0.0f, 0.0f, 0.0f, 0.0f);
        SHCoefficent2 = float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    float4 afC = float4(
        0.282095f,
        0.488603f,
        0.488603f,
        0.488603f
    );

    float4 A = float4(
        0.886227f,
        1.023326f,
        1.023326f,
        1.023326f
    );

    // encode coefficients with direction
    float4 coefficient = float4(
        afC.x * A.x,
        afC.y * direction.y * A.y,
        afC.z * direction.z * A.z,
        afC.w * direction.x * A.w
    );

    // encode with radiance
    float3 aResults[4];
    aResults[0] = radiance.xyz * coefficient.x;
    aResults[1] = radiance.xyz * coefficient.y;
    aResults[2] = radiance.xyz * coefficient.z;
    aResults[3] = radiance.xyz * coefficient.w;

    SHCoefficent0.x += aResults[0].x;
    SHCoefficent0.y += aResults[0].y;
    SHCoefficent0.z += aResults[0].z;
    SHCoefficent0.w += aResults[1].x;

    SHCoefficent1.x += aResults[1].y;
    SHCoefficent1.y += aResults[1].z;
    SHCoefficent1.z += aResults[2].x;
    SHCoefficent1.w += aResults[2].y;

    SHCoefficent2.x += aResults[2].z;
    SHCoefficent2.y += aResults[3].x;
    SHCoefficent2.z += aResults[3].y;
    SHCoefficent2.w += aResults[3].z;

    uint2 coord = uint2(
        int32_t(texCoord.x * float(textureSize.x)),
        int32_t(texCoord.y * float(textureSize.y))
    );

    sphericalHarmonicCoefficientTexture0[coord] = SHCoefficent0;
    sphericalHarmonicCoefficientTexture1[coord] = SHCoefficent1;
    sphericalHarmonicCoefficientTexture2[coord] = SHCoefficent2;
}
#endif // #if 0

/*
**
*/
void encodeToSphericalHarmonicCoefficients2(
    in float3 radiance,
    in float3 direction,
    in uint2 screenCoord,
    in float4 prevSphericalHarmonicsCoefficients0,
    in float4 prevSphericalHarmonicsCoefficients1,
    in float4 prevSphericalHarmonicsCoefficients2
)
{

    float4 SHCoefficent0 = prevSphericalHarmonicsCoefficients0;
    float4 SHCoefficent1 = prevSphericalHarmonicsCoefficients1;
    float4 SHCoefficent2 = prevSphericalHarmonicsCoefficients2;

    float4 afC = float4(
        0.282095f,
        0.488603f,
        0.488603f,
        0.488603f
    );

    float4 A = float4(
        0.886227f,
        1.023326f,
        1.023326f,
        1.023326f
    );

    // encode coefficients with direction
    float4 coefficient = float4(
        afC.x * A.x,
        afC.y * direction.y * A.y,
        afC.z * direction.z * A.z,
        afC.w * direction.x * A.w
    );

    // encode with radiance
    float3 aResults[4];
    aResults[0] = radiance.xyz * coefficient.x;
    aResults[1] = radiance.xyz * coefficient.y;
    aResults[2] = radiance.xyz * coefficient.z;
    aResults[3] = radiance.xyz * coefficient.w;

    SHCoefficent0.x += aResults[0].x;
    SHCoefficent0.y += aResults[0].y;
    SHCoefficent0.z += aResults[0].z;
    SHCoefficent0.w += aResults[1].x;

    SHCoefficent1.x += aResults[1].y;
    SHCoefficent1.y += aResults[1].z;
    SHCoefficent1.z += aResults[2].x;
    SHCoefficent1.w += aResults[2].y;

    SHCoefficent2.x += aResults[2].z;
    SHCoefficent2.y += aResults[3].x;
    SHCoefficent2.z += aResults[3].y;
    SHCoefficent2.w += aResults[3].z;

    sphericalHarmonicCoefficientTexture0[screenCoord] = SHCoefficent0;
    sphericalHarmonicCoefficientTexture1[screenCoord] = SHCoefficent1;
    sphericalHarmonicCoefficientTexture2[screenCoord] = SHCoefficent2;
}

/*
**
*/
float3 decodeFromSphericalHarmonicCoefficients(
    in float3 direction,
    in float3 maxRadiance,
    in float2 texCoord,
    in float fRayCount
)
{
    uint2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    uint2 coord = uint2(
        uint32_t(texCoord.x * float(textureSize.x)),
        uint32_t(texCoord.y * float(textureSize.y))
    );

    float4 SHCoefficent0 = sphericalHarmonicCoefficientTexture0[coord];
    float4 SHCoefficent1 = sphericalHarmonicCoefficientTexture1[coord];
    float4 SHCoefficent2 = sphericalHarmonicCoefficientTexture2[coord];

    float3 aTotalCoefficients[4];
    float fFactor = (4.0f * 3.14159f) / max(fRayCount, 1.0f);
    //if (fRayCount < 10.0f)
    //{
    //    fFactor = 1.0f;
    //}

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
    decoded = clamp(decoded, float3(0.0f, 0.0f, 0.0f), maxRadiance);

    return decoded;
}