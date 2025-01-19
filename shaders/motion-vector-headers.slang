
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