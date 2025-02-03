#include "default-uniform-headers.slang"
#include "mesh-culling-headers.slang"
#include "vertex-format-headers.slang"

struct Material
{
    float4 mDiffuse;
    float4 mSpecular;
    float4 mEmissive;

    uint32_t miID;
    uint32_t miAlbedoTextureID;
    uint32_t miNormalTextureID;
    uint32_t miEmissiveTextureID;
};

[[vk::binding(0, 1)]] RWStructuredBuffer<Material> aMaterials;
[[vk::binding(1, 1)]] RWStructuredBuffer<uint32_t> aMeshMaterialID;
[[vk::binding(2, 1)]] RWStructuredBuffer<Range> aMeshTriangleIndexRanges;
[[vk::binding(3, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformData;

struct PSOutput
{
    float4 mWorldPosition : SV_TARGET0;
    float4 mTexCoord : SV_TARGET1;
    float4 mNormal : SV_TARGET2;
    float4 mMotionVector : SV_TARGET3;
    float2 mScreenMotionVector : SV_TARGET4;
};

struct VertexStageOutput
{
    float4 sv_position : SV_Position;
    float4 worldPosition;
    float4 texCoord;
    float4 normal;
    float4 currClipSpace;
    float4 prevClipSpace;
};

[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    float4x4 viewMatrix = defaultUniformData.mViewMatrix;
    float4x4 projectionMatrix = defaultUniformData.mProjectionMatrix;

    projectionMatrix._22 *= -1.0f;
    
    float4x4 viewProjectionMatrix = mul(
        projectionMatrix,
        viewMatrix
    );

    output.sv_position = mul(
        viewProjectionMatrix,
        float4(
            assembledVertex.position.x,
            assembledVertex.position.y,
            assembledVertex.position.z,
            1.0f),
    );
    output.worldPosition = assembledVertex.position;
    output.texCoord = assembledVertex.texCoord;
    output.normal = assembledVertex.normal;

    output.currClipSpace = mul(
        defaultUniformData.mViewProjectionMatrix,
        float4(assembledVertex.position.xyz, 1.0f)
    );
    output.prevClipSpace = mul(
        defaultUniformData.mPrevViewProjectionMatrix,
        float4(assembledVertex.position.xyz, 1.0f)
    );

    return output;
}

[shader("fragment")]
PSOutput PSMain(VertexStageOutput in)
{
    PSOutput out;

    uint32_t iMesh = uint32_t(ceil(in.worldPosition.w - 0.5f));
    out.mWorldPosition = float4(in.worldPosition.xyz, float(iMesh));
    out.mTexCoord = float4(in.texCoord.x, in.texCoord.y, 0.0f, 1.0f);
    float3 normalXYZ = normalize(in.normal.xyz);
    out.mNormal.x = normalXYZ.x;
    out.mNormal.y = normalXYZ.y;
    out.mNormal.z = normalXYZ.z;
    out.mNormal.w = 1.0f;

    float2 texCoord = in.texCoord.xy;
    if(texCoord.x < 0.0f)
    {
        texCoord.x = fract(abs(1.0f - texCoord.x));
    }
    else if(texCoord.x > 1.0f)
    {
        texCoord.x = fract(texCoord.x);
    }

    if(texCoord.y < 0.0f)
    {
        texCoord.y = fract(abs(1.0f - texCoord.y));
    }
    else if(texCoord.y > 1.0f)
    {
        texCoord.y = fract(texCoord.y);
    }

    out.mTexCoord = float4(texCoord.x, 1.0f - texCoord.y, float(iMesh), 1.0f);

    out.mWorldPosition.x = in.worldPosition.x;
    out.mWorldPosition.y = in.worldPosition.y;
    out.mWorldPosition.z = in.worldPosition.z;
    out.mWorldPosition.w = 1.0f; //in.sv_position.z;

    // store depth and mesh id in worldPosition.w
    //out.mWorldPosition.w = clamp(in.worldPosition.z, 0.0f, 0.999f) + floor(in.worldPosition.w + 0.5f);

    float4 currClipSpacePos = mul(float4(in.worldPosition.xyz, 1.0f), transpose(defaultUniformData.mViewProjectionMatrix));
    float4 prevClipSpacePos = mul(float4(in.worldPosition.xyz, 1.0f), transpose(defaultUniformData.mPrevViewProjectionMatrix));

    float fOneOverW = 1.0f / currClipSpacePos.w;
    currClipSpacePos.x *= fOneOverW;
    currClipSpacePos.y *= fOneOverW;
    currClipSpacePos.z *= fOneOverW;
    currClipSpacePos.x = currClipSpacePos.x * 0.5f + 0.5f;
    currClipSpacePos.y = 1.0f - (currClipSpacePos.y * 0.5f + 0.5f);

    fOneOverW = 1.0f / prevClipSpacePos.w;
    prevClipSpacePos.x *= fOneOverW;
    prevClipSpacePos.y *= fOneOverW;
    prevClipSpacePos.z *= fOneOverW;
    prevClipSpacePos.x = prevClipSpacePos.x * 0.5f + 0.5f;
    prevClipSpacePos.y = 1.0f - (prevClipSpacePos.y * 0.5f + 0.5f);

#if 0
    float3 currClipSpacePos = float3(
        in.currClipSpace.x / in.currClipSpace.w,
        in.currClipSpace.y / in.currClipSpace.w,
        in.currClipSpace.z / in.currClipSpace.w
    ) * 
    0.5f +
    float3(0.5f, 0.5f, 0.5f);
    currClipSpacePos.y = 1.0f - currClipSpacePos.y;

    float3 prevClipSpacePos = float3(
        in.prevClipSpace.x / in.prevClipSpace.w,
        in.prevClipSpace.y / in.prevClipSpace.w,
        in.prevClipSpace.z / in.prevClipSpace.w
    ) *
    0.5f +
    float3(0.5f, 0.5f, 0.5f);
    prevClipSpacePos.y = 1.0f - prevClipSpacePos.y;
#endif // #if 0

    float fDepth = clamp(currClipSpacePos.z, 0.0f, 0.99999f);
    out.mMotionVector = float4(
        (currClipSpacePos.x - prevClipSpacePos.x) * 0.5f + 0.5f,
        (currClipSpacePos.y - prevClipSpacePos.y) * 0.5f + 0.5f,
        float(iMesh) + fDepth,
        1.0f
    );

    int2 screenCoord = int2(
        int32_t(currClipSpacePos.x * float(defaultUniformData.miScreenWidth)),
        int32_t(currClipSpacePos.y * float(defaultUniformData.miScreenHeight))
    );

    int2 prevScreenCoord = int2(
        int32_t(prevClipSpacePos.x * float(defaultUniformData.miScreenWidth)),
        int32_t(prevClipSpacePos.y * float(defaultUniformData.miScreenHeight))
    );

    int2 screenMotionVector = screenCoord - prevScreenCoord;
    out.mScreenMotionVector = float2(
        float(screenMotionVector.x),
        float(screenMotionVector.y)
    );

    return out;
}
