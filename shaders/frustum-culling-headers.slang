#pragma once 

/////
float4 getFrustumPlane(
    uint iColumn,
    float fMult)
{
    float3 plane = float3(
        defaultUniformData.mViewProjectionMatrix[iColumn][0] * fMult + defaultUniformData.mViewProjectionMatrix[3][0],
        defaultUniformData.mViewProjectionMatrix[iColumn][1] * fMult + defaultUniformData.mViewProjectionMatrix[3][1],
        defaultUniformData.mViewProjectionMatrix[iColumn][2] * fMult + defaultUniformData.mViewProjectionMatrix[3][2]);
    float fPlaneW = defaultUniformData.mViewProjectionMatrix[iColumn][3] * fMult + defaultUniformData.mViewProjectionMatrix[3][3];
    float fLength = length(plane);
    plane = normalize(plane);
    
    float4 ret = float4(
        plane.xyz,
        fPlaneW / fLength
    );
    
    return ret;
}

//////
bool cullBBox(
    float3 minPosition,
    float3 maxPosition,
    uint iMesh)
{
    // frustum planes
    float4 leftPlane = getFrustumPlane(0u, 1.0f);
    float4 rightPlane = getFrustumPlane(0u, -1.0f);
    float4 bottomPlane = getFrustumPlane(1u, 1.0f);
    float4 upperPlane = getFrustumPlane(1u, -1.0f);
    float4 nearPlane = getFrustumPlane(2u, 1.0f);
    float4 farPlane = getFrustumPlane(2u, -1.0f);

    float3 v0 = float3(minPosition.x, minPosition.y, minPosition.z);
    float3 v1 = float3(maxPosition.x, minPosition.y, minPosition.z);
    float3 v2 = float3(minPosition.x, minPosition.y, maxPosition.z);
    float3 v3 = float3(maxPosition.x, minPosition.y, maxPosition.z);

    float3 v4 = float3(minPosition.x, maxPosition.y, minPosition.z);
    float3 v5 = float3(maxPosition.x, maxPosition.y, minPosition.z);
    float3 v6 = float3(minPosition.x, maxPosition.y, maxPosition.z);
    float3 v7 = float3(maxPosition.x, maxPosition.y, maxPosition.z);

    float fCount0 = 0.0f;
    fCount0 += sign(dot(leftPlane.xyz, v0) + leftPlane.w);
    fCount0 += sign(dot(leftPlane.xyz, v1) + leftPlane.w);
    fCount0 += sign(dot(leftPlane.xyz, v2) + leftPlane.w);
    fCount0 += sign(dot(leftPlane.xyz, v3) + leftPlane.w);
    fCount0 += sign(dot(leftPlane.xyz, v4) + leftPlane.w);
    fCount0 += sign(dot(leftPlane.xyz, v5) + leftPlane.w);
    fCount0 += sign(dot(leftPlane.xyz, v6) + leftPlane.w);
    fCount0 += sign(dot(leftPlane.xyz, v7) + leftPlane.w);

    float fCount1 = 0.0f;
    fCount1 += sign(dot(rightPlane.xyz, v0) + rightPlane.w);
    fCount1 += sign(dot(rightPlane.xyz, v1) + rightPlane.w);
    fCount1 += sign(dot(rightPlane.xyz, v2) + rightPlane.w);
    fCount1 += sign(dot(rightPlane.xyz, v3) + rightPlane.w);
    fCount1 += sign(dot(rightPlane.xyz, v4) + rightPlane.w);
    fCount1 += sign(dot(rightPlane.xyz, v5) + rightPlane.w);
    fCount1 += sign(dot(rightPlane.xyz, v6) + rightPlane.w);
    fCount1 += sign(dot(rightPlane.xyz, v7) + rightPlane.w);

    float fCount2 = 0.0f;
    fCount2 += sign(dot(upperPlane.xyz, v0) + upperPlane.w);
    fCount2 += sign(dot(upperPlane.xyz, v1) + upperPlane.w);
    fCount2 += sign(dot(upperPlane.xyz, v2) + upperPlane.w);
    fCount2 += sign(dot(upperPlane.xyz, v3) + upperPlane.w);
    fCount2 += sign(dot(upperPlane.xyz, v4) + upperPlane.w);
    fCount2 += sign(dot(upperPlane.xyz, v5) + upperPlane.w);
    fCount2 += sign(dot(upperPlane.xyz, v6) + upperPlane.w);
    fCount2 += sign(dot(upperPlane.xyz, v7) + upperPlane.w);

    float fCount3 = 0.0f;
    fCount3 += sign(dot(bottomPlane.xyz, v0) + bottomPlane.w);
    fCount3 += sign(dot(bottomPlane.xyz, v1) + bottomPlane.w);
    fCount3 += sign(dot(bottomPlane.xyz, v2) + bottomPlane.w);
    fCount3 += sign(dot(bottomPlane.xyz, v3) + bottomPlane.w);
    fCount3 += sign(dot(bottomPlane.xyz, v4) + bottomPlane.w);
    fCount3 += sign(dot(bottomPlane.xyz, v5) + bottomPlane.w);
    fCount3 += sign(dot(bottomPlane.xyz, v6) + bottomPlane.w);
    fCount3 += sign(dot(bottomPlane.xyz, v7) + bottomPlane.w);

    float fCount4 = 0.0f;
    fCount4 += sign(dot(nearPlane.xyz, v0) + nearPlane.w);
    fCount4 += sign(dot(nearPlane.xyz, v1) + nearPlane.w);
    fCount4 += sign(dot(nearPlane.xyz, v2) + nearPlane.w);
    fCount4 += sign(dot(nearPlane.xyz, v3) + nearPlane.w);
    fCount4 += sign(dot(nearPlane.xyz, v4) + nearPlane.w);
    fCount4 += sign(dot(nearPlane.xyz, v5) + nearPlane.w);
    fCount4 += sign(dot(nearPlane.xyz, v6) + nearPlane.w);
    fCount4 += sign(dot(nearPlane.xyz, v7) + nearPlane.w);

    return (fCount0 > -8.0f && fCount1 > -8.0f && fCount2 > -8.0f && fCount3 > -8.0f && fCount4 > -8.0f);  
}

/*
**
*/
//////
bool cullBBoxDepth(
    float3 minPosition,
    float3 maxPosition,
    uint32_t iMesh)
{
    // NOTE: z pyramid textures didn't all in account of all pixels, just the 4 adjacent ones when down scaling
    // scale to pow(2) size to fix this?

    float3 v0 = float3(minPosition.x, minPosition.y, minPosition.z);
    float3 v1 = float3(maxPosition.x, minPosition.y, minPosition.z);
    float3 v2 = float3(minPosition.x, minPosition.y, maxPosition.z);
    float3 v3 = float3(maxPosition.x, minPosition.y, maxPosition.z);

    float3 v4 = float3(minPosition.x, maxPosition.y, minPosition.z);
    float3 v5 = float3(maxPosition.x, maxPosition.y, minPosition.z);
    float3 v6 = float3(minPosition.x, maxPosition.y, maxPosition.z);
    float3 v7 = float3(maxPosition.x, maxPosition.y, maxPosition.z);

    float4 xform0 = mul(float4(v0.xyz, 1.0f), defaultUniformData.mJitteredViewProjectionMatrix);
    float4 xform1 = mul(float4(v1.xyz, 1.0f), defaultUniformData.mJitteredViewProjectionMatrix);
    float4 xform2 = mul(float4(v2.xyz, 1.0f), defaultUniformData.mJitteredViewProjectionMatrix);
    float4 xform3 = mul(float4(v3.xyz, 1.0f), defaultUniformData.mJitteredViewProjectionMatrix);

    float4 xform4 = mul(float4(v4.xyz, 1.0f), defaultUniformData.mJitteredViewProjectionMatrix);
    float4 xform5 = mul(float4(v5.xyz, 1.0f), defaultUniformData.mJitteredViewProjectionMatrix);
    float4 xform6 = mul(float4(v6.xyz, 1.0f), defaultUniformData.mJitteredViewProjectionMatrix);
    float4 xform7 = mul(float4(v7.xyz, 1.0f), defaultUniformData.mJitteredViewProjectionMatrix);

    float3 xyz0  = xform0.xyz / xform0.w;
    float3 xyz1  = xform1.xyz / xform1.w;
    float3 xyz2  = xform2.xyz / xform2.w;
    float3 xyz3  = xform3.xyz / xform3.w;

    float3 xyz4  = xform4.xyz / xform4.w;
    float3 xyz5  = xform5.xyz / xform5.w;
    float3 xyz6  = xform6.xyz / xform6.w;
    float3 xyz7  = xform7.xyz / xform7.w;

    float3 minXYZ = min(min(min(min(min(min(min(xyz0, xyz1), xyz2), xyz3), xyz4), xyz5), xyz6), xyz7);
    float3 maxXYZ = max(max(max(max(max(max(max(xyz0, xyz1), xyz2), xyz3), xyz4), xyz5), xyz6), xyz7);
    minXYZ = float3(minXYZ.x * 0.5f + 0.5f, minXYZ.y * 0.5f + 0.5f, minXYZ.z);
    maxXYZ = float3(maxXYZ.x * 0.5f + 0.5f, maxXYZ.y * 0.5f + 0.5f, maxXYZ.z);

    minXYZ.x = clamp(minXYZ.x, 0.0f, 1.0f);
    minXYZ.y = clamp(minXYZ.y, 0.0f, 1.0f);
    maxXYZ.x = clamp(maxXYZ.x, 0.0f, 1.0f);
    maxXYZ.y = clamp(maxXYZ.y, 0.0f, 1.0f);

    if (maxXYZ.z > 1.0f)
    {
        minXYZ.z = 0.0f;
    }

    float fMinDepth = minXYZ.z;
    float fAspectRatio = float(defaultUniformData.miScreenHeight) / float(defaultUniformData.miScreenWidth);
    float2 diffXY = abs(maxXYZ.xy - minXYZ.xy);
    float fMaxComp = max(diffXY.x, diffXY.y * fAspectRatio) * 512.0f; // compute LOD from 1 to 8
    uint32_t iLOD = min(uint32_t(log2(fMaxComp)), 8u);
    float2 uv = float2(maxXYZ.xy + minXYZ.xy) * 0.5f;
    uv.y = 1.0f - uv.y;
    uv = clamp(uv, float2(0.0f, 0.0f), float2(1.0f, 1.0f));
    var fSampleDepth = 0.0f;
    if (iLOD == 0u)
    {
        fSampleDepth = depthTexture0.SampleLevel(
            sampler,
            uv,
            0.0f
        ).x;
    }
    else if (iLOD == 1u)
    {
        fSampleDepth = depthTexture1.SampleLevel(
            sampler,
            uv,
            0.0f
        ) .x;
    }
    else if (iLOD == 2u)
    {
        fSampleDepth = depthTexture2.SampleLevel(
            sampler,
            uv,
            0.0f
        ) .x;
    }
    else if (iLOD == 3u)
    {
        fSampleDepth = depthTexture3.SampleLevel(
            sampler,
            uv,
            0.0f
        ).x;
    }
    else if (iLOD == 4u)
    {
        fSampleDepth = depthTexture4.SampleLevel(
            sampler,
            uv,
            0.0f
        ).x;
    }
    else if (iLOD == 5u)
    {
        fSampleDepth = depthTexture5.SampleLevel(
            sampler,
            uv,
            0.0f
        ).x;
    }
    else if (iLOD == 6u)
    {
        fSampleDepth = depthTexture6.SampleLevel(
            sampler,
            uv,
            0.0f
        ).x;
    }
    else
    {
        fSampleDepth = depthTexture7.SampleLevel(
            sampler,
            uv,
            0.0f
        ).x;
    }

    return (fMinDepth > fSampleDepth);
}