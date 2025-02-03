
/*
**
*/
float2 getPreviousScreenUVRayTrace(
    in uint2 screenCoord)
{
    float2 screenUV = screenCoord / float2(float(defaultUniformBuffer.miScreenWidth), float(defaultUniformBuffer.miScreenHeight));

    float2 motionVector = motionVectorTexture[screenCoord].xy;
    float2 prevScreenUV = screenUV - motionVector;

    float3 worldPosition = worldPositionTexture[screenCoord].xyz;
    float3 normal = normalTexture[screenCoord].xyz;

    float fOneOverScreenWidth = 1.0f / float(defaultUniformBuffer.miScreenWidth);
    float fOneOverScreenHeight = 1.0f / float(defaultUniformBuffer.miScreenHeight);

    float fShortestWorldDistance = 99999.0f;
    float2 closestScreenUV = prevScreenUV;
    for(int32_t iY = -1; iY <= 1; iY++)
    {
        for(int32_t iX = -1; iX <= 1; iX++)
        {
            float2 sampleUV = prevScreenUV + float2(
                float(iX) * fOneOverScreenWidth,
                float(iY) * fOneOverScreenHeight 
            );

            sampleUV.x = clamp(sampleUV.x, 0.0f, 1.0f);
            sampleUV.y = clamp(sampleUV.y, 0.0f, 1.0f);

            uint2 sampleCoord = uint2(
                uint32_t(sampleUV.x * float(defaultUniformBuffer.miScreenWidth)),
                uint32_t(sampleUV.y * float(defaultUniformBuffer.miScreenHeight))
            );

            float3 checkWorldPosition = prevWorldPositionTexture[sampleCoord].xyz;
            float3 checkNormal = prevNormalTexture[sampleCoord].xyz;
            float fNormalDP = abs(dot(checkNormal, normal));

            float3 worldPositionDiff = checkWorldPosition - worldPosition;
            float fLengthSquared = dot(worldPositionDiff, worldPositionDiff);
            if(fNormalDP >= 0.99f && fShortestWorldDistance > fLengthSquared)
            {
                fShortestWorldDistance = fLengthSquared;
                closestScreenUV = sampleUV;
            }
        }
    }

    return closestScreenUV;
}

/*
**
*/
bool isDisoccluded2RayTrace(
    in uint2 screenCoord,
    in uint2 prevScreenCoord
)
{
    float3 worldPosition = worldPositionTexture[screenCoord].xyz;
    float3 prevWorldPosition = prevWorldPositionTexture[prevScreenCoord].xyz;
    float3 normal = normalTexture[screenCoord].xyz;
    float3 prevNormal = prevNormalTexture[prevScreenCoord].xyz;
    float4 motionVector = motionVectorTexture[screenCoord];

    float4 prevMotionVectorAndMeshIDAndDepth = prevMotionVectorTexture[prevScreenCoord];

    uint32_t iMesh = uint32_t(ceil(motionVector.w - 0.5f)) - 1;
    float fDepth = motionVector.w;
    //float fPrevDepth = prevMotionVectorAndMeshIDAndDepth.w;
    //float fCheckDepth = abs(fDepth - fPrevDepth);
    float3 worldPositionDiff = prevWorldPosition.xyz - worldPosition.xyz;
    float fCheckDP = abs(dot(normalize(normal.xyz), normalize(prevNormal.xyz)));
    uint32_t iPrevMesh = uint32_t(ceil(prevMotionVectorAndMeshIDAndDepth.w - 0.5f)) - 1;
    float fCheckWorldPositionDistance = dot(worldPositionDiff, worldPositionDiff);

    // return !(iMesh == iPrevMesh && fCheckDepth <= 0.01f && fCheckWorldPositionDistance <= 0.01f && fCheckDP >= 0.99f);
    return !(iMesh == iPrevMesh && fCheckWorldPositionDistance <= 0.01f && fCheckDP >= 0.99f);
}

/*
**
*/
bool isPrevUVOutOfBoundsRayTrace(
    in uint2 inputScreenCoord,
    in uint2 screenSize)
{
    float2 inputTexCoord = inputScreenCoord / float2(screenSize.x, screenSize.y);
    float4 motionVector = motionVectorTexture[inputScreenCoord];
    float2 backProjectedScreenUV = inputTexCoord - motionVector.xy;

    return (backProjectedScreenUV.x < 0.0f || backProjectedScreenUV.x > 1.0 || backProjectedScreenUV.y < 0.0f || backProjectedScreenUV.y > 1.0f);
}

/*
**
*/
float4 getPreviousScreenCoord(
    in float2 screenUV)
{
    float2 screenUVCopy = screenUV;

    int2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    int2 screenCoord = uint2(
        int32_t(screenUV.x * float(textureSize.x)),
        int32_t(screenUV.y * float(textureSize.y)) 
    );

    float3 worldPosition = worldPositionTexture[screenCoord].xyz;
    float3 normal = normalTexture[screenCoord].xyz;

    float fOneOverScreenWidth = 1.0f / float(textureSize.x);
    float fOneOverScreenHeight = 1.0f / float(textureSize.y);

    float2 motionVector = motionVectorTexture[screenCoord].xy;
    motionVector.xy = motionVector.xy * 2.0f - 1.0f;

    int2 motionVectorScreenCoord = uint2(
        int32_t(motionVector.x * float(textureSize.x)),
        int32_t(motionVector.y * float(textureSize.y)) 
    );
    int2 prevScreenCoord = screenCoord - motionVectorScreenCoord;

    if(screenCoord.x == prevScreenCoord.x && screenCoord.y == prevScreenCoord.y)
    {
        return float4(float(screenCoord.x), float(screenCoord.y), 0.0f, 1.0f);
    }

    float fShortestWorldDistance = FLT_MAX;
    float fLargestNormalDP = FLT_MAX;
#if 0
    for (int32_t iY = -2; iY <= 2; iY++)
    {
        for (int32_t iX = -2; iX <= 2; iX++)
        {
            int2 sampleScreenCoord = prevScreenCoord + int2(iX, iY);

            float3 checkWorldPosition = prevWorldPositionTexture[sampleScreenCoord].xyz;
            float3 checkNormal = prevNormalTexture[sampleScreenCoord].xyz;
            float fNormalDP = abs(dot(checkNormal, normal));

            float3 worldPositionDiff = checkWorldPosition - worldPosition;
            float fLengthSquared = dot(worldPositionDiff, worldPositionDiff);
            if (fNormalDP >= 0.99f && fShortestWorldDistance > fLengthSquared)
            {
                fShortestWorldDistance = fLengthSquared;

                prevScreenCoord.x = sampleScreenCoord.x;
                prevScreenCoord.y = sampleScreenCoord.y;

                fLargestNormalDP = fNormalDP;
            }
        }
    }
#endif // #if 0

    return float4(
        float(prevScreenCoord.x),
        float(prevScreenCoord.y),
        fShortestWorldDistance,
        fLargestNormalDP);
}

/*
**
*/
bool disocclusionCriteria(
    in uint32_t iMesh,
    in uint32_t iPrevMesh,
    in float fCheckDP,
    in float fCheckWorldPositionDistance
)
{
    return !(fCheckDP >= 0.99f && fCheckWorldPositionDistance <= 0.005f && iMesh == iPrevMesh);
}

/*
**
*/
bool isDisoccludedRayTrace2(
    in float2 screenUV,
    in float2 prevScreenUV
)
{
    uint2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);
    uint2 screenCoord = uint2(
        uint32_t(screenUV.x * float(textureSize.x)),
        uint32_t(screenUV.y * float(textureSize.y))
    );

    uint2 prevScreenCoord = uint2(
        uint32_t(prevScreenUV.x * float(textureSize.x)),
        uint32_t(prevScreenUV.y * float(textureSize.y))
    );

    float3 worldPosition = worldPositionTexture[screenCoord].xyz;
    float3 prevWorldPosition = prevWorldPositionTexture[prevScreenCoord].xyz;
    float3 normal = normalTexture[screenCoord].xyz;
    float3 prevNormal = prevNormalTexture[prevScreenCoord].xyz;
    float4 motionVector = motionVectorTexture[screenCoord];
    float4 prevMotionVectorAndMeshIDAndDepth = prevMotionVectorTexture[prevScreenCoord];

    // uint32_t iMesh = uint32_t(ceil(motionVector.w - 0.5f)) - 1;
    // float fDepth = motionVector.z;
    // float fPrevDepth = prevMotionVectorAndMeshIDAndDepth.z;

    float fDepth = fract(motionVector.z);
    float fPrevDepth = fract(prevMotionVectorAndMeshIDAndDepth.z);

    uint32_t iMesh = uint32_t(motionVector.z - fDepth);
    uint32_t iPrevMesh = uint32_t(prevMotionVectorAndMeshIDAndDepth.z - fPrevDepth);

    float fCheckDepth = abs(fDepth - fPrevDepth);
    float3 worldPositionDiff = prevWorldPosition.xyz - worldPosition.xyz;
    float fCheckDP = abs(dot(normalize(normal.xyz), normalize(prevNormal.xyz)));
    // uint32_t iPrevMesh = uint32_t(ceil(prevMotionVectorAndMeshIDAndDepth.w - 0.5f)) - 1;
    float fCheckWorldPositionDistance = dot(worldPositionDiff, worldPositionDiff);

    // return !(fCheckDP >= 0.99f && fCheckWorldPositionDistance <= 0.01f && iMesh == iPrevMesh);
    return disocclusionCriteria(
        iMesh,
        iPrevMesh,
        fCheckDP,
        fCheckWorldPositionDistance
    );

    // return !(iMesh == iPrevMesh && fCheckDepth <= 0.001f && fCheckWorldPositionDistance <= 0.001f && fCheckDP >= 0.999f);
}

/*
**
*/
bool isDisoccluded2(
    in float2 screenUV,
    in float2 prevScreenUV
)
{
    uint2 textureSize;

    float3 worldPosition = worldPositionTexture.Sample(textureSampler, screenUV).xyz;
    float3 prevWorldPosition = prevWorldPositionTexture.Sample(textureSampler, prevScreenUV).xyz;
    float3 normal = normalTexture.Sample(textureSampler, screenUV).xyz;
    float3 prevNormal = prevNormalTexture.Sample(textureSampler, prevScreenUV).xyz;
    float4 motionVector = motionVectorTexture.Sample(textureSampler, screenUV);
    float4 prevMotionVectorAndMeshIDAndDepth = prevMotionVectorTexture.Sample(textureSampler, prevScreenUV);

    float fDepth = fract(motionVector.z);
    float fPrevDepth = fract(prevMotionVectorAndMeshIDAndDepth.z);

    uint32_t iMesh = uint32_t(motionVector.z - fDepth);
    uint32_t iPrevMesh = uint32_t(prevMotionVectorAndMeshIDAndDepth.z - fPrevDepth);

    float fCheckDepth = abs(fDepth - fPrevDepth);
    float3 worldPositionDiff = prevWorldPosition.xyz - worldPosition.xyz;
    float fCheckDP = abs(dot(normalize(normal.xyz), normalize(prevNormal.xyz)));
    float fCheckWorldPositionDistance = dot(worldPositionDiff, worldPositionDiff);

    return disocclusionCriteria(
        iMesh,
        iPrevMesh,
        fCheckDP,
        fCheckWorldPositionDistance
    );
}

/*
**
*/
bool isPrevUVOutOfBounds(
    in float2 inputTexCoord
)
{
    float4 motionVector = motionVectorTexture.Sample(
        textureSampler,
        inputTexCoord
    );
    motionVector.xy = motionVector.xy * 2.0f - 1.0f;

    float2 backProjectedScreenUV = inputTexCoord - motionVector.xy;
    return (backProjectedScreenUV.x < 0.0f || backProjectedScreenUV.x > 1.0 || backProjectedScreenUV.y < 0.0f || backProjectedScreenUV.y > 1.0f);
}

/*
**
*/
bool isPrevUVOutOfBoundsRayTrace(
    in uint2 inputTexCoord
)
{
    uint2 textureSize;
    worldPositionTexture.GetDimensions(textureSize.x, textureSize.y);

    float2 inputTexUV = float2(
        float(inputTexCoord.x) / float(textureSize.x),
        float(inputTexCoord.y) / float(textureSize.y)
    );
    float4 motionVector = motionVectorTexture[inputTexCoord];
    motionVector.xy = motionVector.xy * 2.0f - 1.0f;
    float2 backProjectedScreenUV = inputTexUV - motionVector.xy;

    return (backProjectedScreenUV.x < 0.0f || backProjectedScreenUV.x > 1.0 || backProjectedScreenUV.y < 0.0f || backProjectedScreenUV.y > 1.0f);
}