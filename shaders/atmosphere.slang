// -------------------------------------
// Defines
#define EPS 1e-6f
#define PLANET_RADIUS 6371000.0f
#define PLANET_CENTER float3(0.0f, -PLANET_RADIUS, 0.0f)
#define ATMOSPHERE_HEIGHT 100000.0f
#define RAYLEIGH_HEIGHT     (ATMOSPHERE_HEIGHT * 0.08f)
#define MIE_HEIGHT (ATMOSPHERE_HEIGHT * 0.012f)
#define PI  3.14159f

// -------------------------------------
// Coefficients
#define C_RAYLEIGH (float3(5.802f, 13.558f, 33.100f) * 1e-6f)
#define C_MIE (float3(3.996f, 3.996f, 3.996f) * 1e-6f)
#define C_OZONE (float3(0.650f, 1.881f, 0.085f) * 1e-6f)

#define ATMOSPHERE_DENSITY 1.0f;
#define EXPOSURE 20.0f;


#include  "default-uniform-headers.slang"

// Per-vertex attributes to be assembled from bound vertex buffers.
struct AssembledVertex
{
    float4 position : POSITION;
    float4 uv : TEXCOORD;
};

// Output of the fragment shader
struct Fragment
{
    float4 mSkyRadiance : SV_TARGET0;
    float4 mSunLightRadiance : SV_TARGET1;
    float4 mDiffuseSkyRadiance : SV_TARGET2;
};

// Vertex  Shader

struct VertexStageOutput
{
    float4          sv_position     : SV_Position;
    float4          texcoord;
};

[[vk::binding(0, 0)]] Texture2D<float4> prevDiffuseSkyTexture;
[[vk::binding(1, 0)]] Texture2D<float4> prevSkyTexture;
[[vk::binding(2, 0)]] Texture2D<float4> prevSunRadianceTexture;
[[vk::binding(3, 0)]] SamplerState textureSampler;

[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;


[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    float4 position = assembledVertex.position;
    float4 uv = assembledVertex.uv;

    output.sv_position = float4(position.xyz, 1.0f);
    output.texcoord = uv;

    return output;
}

// Fragment Shader

[shader("fragment")]
Fragment PSMain(
    VertexStageOutput input) : SV_Target
{
    Fragment ret;

    float3 directionDiff = defaultUniformBuffer.mLightDirection.xyz - defaultUniformBuffer.mPrevLightDirection.xyz;
    float fLengthSquared = dot(directionDiff, directionDiff);

    if(defaultUniformBuffer.miFrame > 10 && fLengthSquared <= 0.00001f)
    {
        float2 uv = float2(
            input.texcoord.x,
            input.texcoord.y 
        );
        float4 prevDiffuseSkyRadiance = prevDiffuseSkyTexture.Sample(
            textureSampler,
            uv
        );

        float4 prevSkyRadiance = prevSkyTexture.Sample(
            textureSampler,
            uv
        );

        float4 prevSunRadiance = prevSunRadianceTexture.Sample(
            textureSampler,
            uv
        );

        ret.mDiffuseSkyRadiance = prevDiffuseSkyRadiance;
        ret.mSkyRadiance = prevSkyRadiance;
        ret.mSunLightRadiance = prevSunRadiance;

        return ret;
    }

    float3 direction = decodeOctahedronMap(input.texcoord.xy).xyz;
    float3 transmittance = float3(0.0f, 0.0f, 0.0f);

    //float3 lightDirection = normalize(float3(-0.5f, 1.0f, 0.0f));
    //float3 lightRadiance = float3(1.0f, 1.0f, 1.0f);
    float3 lightDirection = normalize(defaultUniformBuffer.mLightDirection.xyz);
    float3 lightRadiance = defaultUniformBuffer.mLightRadiance.xyz;

    ScatteringResult sky = IntegrateScattering(
        float3(0.0f, 0.0f, 0.0f),
        direction,
        PLANET_RADIUS,
        lightDirection,
        lightRadiance,
        transmittance);

    ScatteringResult sun = IntegrateScattering(
        float3(0.0f, 0.0f, 0.0f),
        lightDirection,
        PLANET_RADIUS,
        lightDirection,
        lightRadiance,
        transmittance);

    ret.mSkyRadiance = float4(sky.radiance * sky.transmittance, 1.0f);
    ret.mSunLightRadiance = float4(sun.radiance * sun.transmittance, 1.0f);
#if 0
    ret.mDiffuseSkyRadiance = float4(
        convolve(
            input.texcoord.xy,
            lightDirection,
            lightRadiance,
            64), 
        1.0f
    );
#endif // #if 0

    return ret;
}

// -------------------------------------
// Math
float2 SphereIntersection(
    in float3 rayStart,
    in float3 rayDir,
    in float3 sphereCenter,
    in float sphereRadius)
{
    float3 rayStartDiff = rayStart - sphereCenter;
    float a = dot(rayDir, rayDir);
    float b = 2.0f * dot(rayStartDiff, rayDir);
    float c = dot(rayStartDiff, rayStartDiff) - (sphereRadius * sphereRadius);
    float d = b * b - 4.0f * a * c;
    if (d < 0.0f)
    {
        return float2(-1.0f, -1.0f);
    }
    else
    {
        d = sqrt(d);
        return float2(-b - d, -b + d) / (2.0f * a);
    }
}
float2 PlanetIntersection(
    float3 rayStart,
    float3 rayDir)
{
    return SphereIntersection(rayStart, rayDir, PLANET_CENTER, PLANET_RADIUS);
}
float2 AtmosphereIntersection(
    float3 rayStart,
    float3 rayDir)
{
    return SphereIntersection(rayStart, rayDir, PLANET_CENTER, PLANET_RADIUS + ATMOSPHERE_HEIGHT);
}

// -------------------------------------
// Phase functions
float PhaseRayleigh(float costh)
{
    return 3.0f * (1.0f + costh * costh) / (16.0f * PI);
}
float PhaseMie(
    float costh,
    float g)
{
    float gCopy = min(g, 0.9381f);
    float k = 1.55f * gCopy - 0.55f * gCopy * gCopy * gCopy;
    float kcosth = k * costh;
    return (1.0f - k * k) / ((4.0f * PI) * (1.0f - kcosth) * (1.0f - kcosth));
}

// -------------------------------------
// Atmosphere
float AtmosphereHeight(float3 positionWS)
{
    // return distance(positionWS, PLANET_CENTER) - PLANET_RADIUS;

    return length(positionWS - PLANET_CENTER) - PLANET_RADIUS;
}
float DensityRayleigh(float h)
{
    return exp(-max(0.0f, h / RAYLEIGH_HEIGHT));
}
float DensityMie(float h)
{
    return exp(-max(0.0f, h / MIE_HEIGHT));
}
float DensityOzone(float h)
{
    // The ozone layer is represented as a tent function with a width of 30km, centered around an altitude of 25km.
    return max(0.0f, 1.0f - abs(h - 25000.0f) / 15000.0f);
}
float3 AtmosphereDensity(float h)
{
    return float3(DensityRayleigh(h), DensityMie(h), DensityOzone(h));
}

// Optical depth is a unitless measurement of the amount of absorption of a participating medium (such as the atmosphere).
// This function calculates just that for our three atmospheric elements:
// R: Rayleigh
// G: Mie
// B: Ozone
// If you find the term "optical depth" confusing, you can think of it as "how much density was found along the ray in total".
float3 IntegrateOpticalDepth(
    float3 rayStart,
    float3 rayDir)
{
    float2 intersection = AtmosphereIntersection(rayStart, rayDir);
    float rayLength = intersection.y;

    int32_t sampleCount = 8;
    float stepSize = rayLength / float(sampleCount);

    float3 opticalDepth = float3(0.0f, 0.0f, 0.0f);

    for (int32_t i = 0; i < sampleCount; i++)
    {
        float3 localPosition = rayStart + rayDir * (float(i) + 0.5f) * stepSize;
        float localHeight = AtmosphereHeight(localPosition);
        float3 localDensity = AtmosphereDensity(localHeight);

        opticalDepth += localDensity * stepSize;
    }

    return opticalDepth;
}

// Calculate a luminance transmittance value from optical depth.
float3 Absorb(float3 opticalDepth)
{
    // Note that Mie results in slightly more light absorption than scattering, about 10%
    float3 ret = ((C_RAYLEIGH * opticalDepth.x) + (C_MIE * (opticalDepth.y * 1.1f)) + (C_OZONE * opticalDepth.z)) * -ATMOSPHERE_DENSITY;
    return float3(
        exp(ret.x),
        exp(ret.y),
        exp(ret.z));
}

struct ScatteringResult
{
    float3 radiance;
    float3 transmittance;
};

// Integrate scattering over a ray for a single directional light source.
// Also return the transmittance for the same ray as we are already calculating the optical depth anyway.
ScatteringResult IntegrateScattering(
    float3 rayStart,
    float3 rayDir,
    float rayLength,
    float3 lightDir,
    float3 lightColor,
    float3 transmittance)
{
    // We can reduce the number of atmospheric samples required to converge by spacing them exponentially closer to the camera.
    // This breaks space view however, so let's compensate for that with an exponent that "fades" to 1 as we leave the atmosphere.
    float3 rayStartCopy = rayStart;
    float rayHeight = AtmosphereHeight(rayStartCopy);
    float sampleDistributionExponent = 1.0f + clamp(1.0f - rayHeight / ATMOSPHERE_HEIGHT, 0.0f, 1.0f) * 8; // Slightly arbitrary max exponent of 9

    float2 intersection = AtmosphereIntersection(rayStartCopy, rayDir);
    float rayLengthCopy = min(rayLength, intersection.y);
    if (intersection.x > 0)
    {
        // Advance ray to the atmosphere entry point
        rayStartCopy += rayDir * intersection.x;
        rayLengthCopy -= intersection.x;
    }

    float costh = dot(rayDir, lightDir);
    float phaseR = PhaseRayleigh(costh);
    float phaseM = PhaseMie(costh, 0.85f);

    int32_t sampleCount = 64;

    float3 opticalDepth = float3(0.0f, 0.0f, 0.0f);
    float3 rayleigh = float3(0.0f, 0.0f, 0.0f);
    float3 mie = float3(0.0f, 0.0f, 0.0f);

    float prevRayTime = 0.0f;

    for (int32_t i = 0; i < sampleCount; i++)
    {
        float rayTime = pow(float(i) / float(sampleCount), sampleDistributionExponent) * rayLengthCopy;
        // Because we are distributing the samples exponentially, we have to calculate the step size per sample.
        float stepSize = (rayTime - prevRayTime);

        float3 localPosition = rayStartCopy + rayDir * rayTime;
        float localHeight = AtmosphereHeight(localPosition);
        float3 localDensity = AtmosphereDensity(localHeight);

        opticalDepth += localDensity * stepSize;

        // The atmospheric transmittance from rayStart to localPosition
        float3 viewTransmittance = Absorb(opticalDepth);

        float3 opticalDepthlight = IntegrateOpticalDepth(localPosition, lightDir);
        // The atmospheric transmittance of light reaching localPosition
        float3 lightTransmittance = Absorb(opticalDepthlight);

        rayleigh += viewTransmittance * lightTransmittance * phaseR * localDensity.x * stepSize;
        mie += viewTransmittance * lightTransmittance * phaseM * localDensity.y * stepSize;

        prevRayTime = rayTime;
    }

    var ret : ScatteringResult;
    ret.transmittance = Absorb(opticalDepth);
    ret.radiance = (rayleigh * C_RAYLEIGH + mie * C_MIE) * lightColor * EXPOSURE;

    return ret;
}

/////
float3 decodeOctahedronMap(float2 uv)
{
    float2 newUV = uv * 2.0f - float2(1.0f, 1.0f);
    float2 absUV = float2(abs(newUV.x), abs(newUV.y));
    float3 v = float3(newUV.x, newUV.y, 1.0f - (absUV.x + absUV.y));

    if (absUV.x + absUV.y > 1.0f)
    {
        v.x = (abs(newUV.y) - 1.0f) * -sign(newUV.x);
        v.y = (abs(newUV.x) - 1.0f) * -sign(newUV.y);
    }

    v.y *= -1.0f;

    return v;
}

/*
**
*/
float3 convolve(
    in float2 uv,
    in float3 lightDirection,
    in float3 lightRadiance,
    in uint32_t iNumLoops)
{
    float3 normal = decodeOctahedronMap(uv).xyz;
    float3 transmittance = float3(0.0f, 0.0f, 0.0f);

    float3 up = float3(0.0f, 1.0f, 0.0f);
    if (abs(normal.y) > 0.999f)
    {
        up = float3(1.0f, 0.0f, 0.0f);
    }
    float3 tangent = normalize(cross(up, normal));
    float3 binormal = normalize(cross(normal, tangent));

    float3 totalRadiance = float3(0.0f, 0.0f, 0.0f);
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

            float3 rayDirection = normalize(tangent * h.x + binormal * h.y + normal * h.z);
            ScatteringResult sky = IntegrateScattering(
                float3(0.0f, 0.0f, 0.0f),
                rayDirection,
                PLANET_RADIUS,
                lightDirection,
                lightRadiance,
                transmittance);

            totalRadiance += sky.radiance * sky.transmittance;
            fCount += 1.0f;
        }
    }

    return totalRadiance / fCount;

    
}