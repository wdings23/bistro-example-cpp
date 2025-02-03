#include "irradiance-cache-headers.slang"

struct BouncedRadianceInfo
{
    float3          mRadiance;
    float           mfDistanceAttenuation;
    uint32_t        miIntersectionType;
    float           mfAnglePct;
};

/*
**
*/
float3 getIrradianceCacheRadiance(
    in float3 position,
    in float3 worldPosition)
{
    float fReflectivity = 1.0f;

    float3 ret = float3(0.0f, 0.0f, 0.0f);

    uint32_t iHitIrradianceCacheIndex = fetchIrradianceCacheIndex(position);
    float4 irradianceCachePosition = irradianceCache[iHitIrradianceCacheIndex].mPosition;
    if (irradianceCachePosition.w > 0.0f)
    {
        float3 diff = irradianceCachePosition.xyz - worldPosition;
        float3 positionToCacheDirection = normalize(irradianceCachePosition.xyz - worldPosition) * -1.0f;
        float3 irradianceCacheProbeRadiance = getRadianceFromIrradianceCacheProbe(
            positionToCacheDirection,
            iHitIrradianceCacheIndex);
        float fLengthSquared = dot(diff, diff);
        ret.xyz = (irradianceCacheProbeRadiance.xyz * fReflectivity) / max(fLengthSquared, 1.0f);
    }

    return ret;
}

/*
**
*/
BouncedRadianceInfo getBouncedRadiance(
    in float3 worldPosition,
    in float3 normal,
    in float3 hitPosition,
    in float3 hitNormal,
    in float2 hitUV,
    in float2 motionVector,
    in float fReflectivity,
    in float4x4 viewProjectionMatrix,
    in int2 textureSize,
    in int32_t iMaterialID,
    in Texture2D<float4> prevRadianceTexture
)
{
    BouncedRadianceInfo ret;
    ret.miIntersectionType = 0;
    ret.mfDistanceAttenuation = 1.0f;
    ret.mRadiance = float3(0.0f, 0.0f, 0.0f);
    ret.mfAnglePct = 1.0f;

    float3 direction = hitPosition - worldPosition;
    ret.mfDistanceAttenuation = 1.0f / max(dot(direction, direction), 1.0f);
    ret.mfAnglePct = max(dot(normalize(direction), normal), 0.0f);

    float4 clipSpacePosition = mul(viewProjectionMatrix, float4(hitPosition, 1.0f));
    clipSpacePosition.x /= clipSpacePosition.w;
    clipSpacePosition.y /= clipSpacePosition.w;
    clipSpacePosition.z /= clipSpacePosition.w;

    float2 prevScreenUV = clipSpacePosition.xy - motionVector;
    prevScreenUV.x = prevScreenUV.x * 0.5f + 0.5f;
    prevScreenUV.y = 1.0f - (prevScreenUV.y * 0.5f + 0.5f);

    clipSpacePosition.x = clipSpacePosition.x * 0.5f + 0.5f;
    clipSpacePosition.y = 1.0f - (clipSpacePosition.y * 0.5f + 0.5f);
    clipSpacePosition.z = clipSpacePosition.z * 0.5f + 0.5f;
    uint2 clipSpaceCoord = uint2(
        uint32_t(floor(clipSpacePosition.x * float(textureSize.x) + 0.5f)),
        uint32_t(floor(clipSpacePosition.y * float(textureSize.y) + 0.5f))
    );

    if (clipSpaceCoord.x >= 0 && clipSpaceCoord.x < textureSize.x &&
        clipSpaceCoord.y >= 0 && clipSpaceCoord.y < textureSize.y &&
        clipSpacePosition.z >= 0.0f && clipSpacePosition.z <= 1.0f)
    {
        uint2 prevClipSpaceCoord = uint2( 
            uint32_t(floor(prevScreenUV.x * float(textureSize.x) + 0.5f)),
            uint32_t(floor(prevScreenUV.y * float(textureSize.y) + 0.5f))
        );

        float3 checkHitPosition = worldPositionTexture[clipSpaceCoord.xy].xyz;
        float3 diff = checkHitPosition - hitPosition.xyz;
        if (dot(diff, diff) <= 0.1f)
        {
            ret.mRadiance = directSunOutputTexture[clipSpaceCoord.xy].xyz * fReflectivity;
            if(prevClipSpaceCoord.x >= 0 && prevClipSpaceCoord.x < textureSize.x &&
               prevClipSpaceCoord.x >= 0 && prevClipSpaceCoord.x < textureSize.x)
            {
                ret.mRadiance += prevRadianceTexture[prevClipSpaceCoord].xyz;
            }
            ret.mRadiance *= albedoTexture[clipSpaceCoord.xy].xyz;

            ret.miIntersectionType = 1;
        }
    }

    //if (ret.miIntersectionType == 0)
    {
        // not on screen, check irradiance cache
        float3 irradianceCacheRadiance = getIrradianceCacheRadiance(
            hitPosition.xyz,
            worldPosition.xyz);

        Material material = materialData[iMaterialID];
        uint32_t iInitialAtlasWidth = 1, iInitialAtlasHeight = 1;
        initialTextureAtlas.GetDimensions(iInitialAtlasWidth, iInitialAtlasHeight);

        int32_t iInitialPageSize = 16;
        int32_t iNumInitialPagesPerRow = iInitialAtlasWidth / iInitialPageSize;
        uint32_t iTextureID = materialData[iMaterialID].miAlbedoTextureID;

        int2 pageOffset = int2(
            int32_t(floor(hitUV.x * float(iInitialPageSize) + 0.5f)),
            int32_t(floor(hitUV.x * float(iInitialPageSize) + 0.5f))
            );

        int2 textureAtlasCoord = int2(
            (iTextureID % iNumInitialPagesPerRow) * iInitialPageSize + pageOffset.x,
            (iTextureID / iNumInitialPagesPerRow) * iInitialPageSize + pageOffset.y
            );
        float3 albedo = initialTextureAtlas[textureAtlasCoord].xyz;
        irradianceCacheRadiance *= albedo;

        if(length(irradianceCacheRadiance) > length(ret.mRadiance))
        {
            ret.mRadiance = irradianceCacheRadiance;
            ret.miIntersectionType = 2;
        }
    }

    return ret;

}