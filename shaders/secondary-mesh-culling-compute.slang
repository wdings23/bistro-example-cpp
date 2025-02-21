#include "default-uniform-headers.slang"

struct DrawIndexParam
{
    uint32_t miIndexCount;
    uint32_t miInstanceCount;
    uint32_t miFirstIndex;
    int32_t miBaseVertex;
    uint32_t miFirstInstance;
};

struct MeshBBox
{
    float4 mMinPosition;
    float4 mMaxPosition;
};

struct Range
{
    uint32_t miStart;
    uint32_t miEnd;
};

struct UniformData
{
    uint32_t miNumMeshes;
};

[[vk::binding(0, 0)]] RWStructuredBuffer<DrawIndexParam> aDrawCalls;
[[vk::binding(1, 0)]] RWStructuredBuffer<uint> aNumDrawCalls;
[[vk::binding(2, 0)]] RWStructuredBuffer<uint> aiVisibleMeshID;
[[vk::binding(3, 0)]] Texture2D<float> depthTexture0;
[[vk::binding(4, 0)]] Texture2D<float> depthTexture1;
[[vk::binding(5, 0)]] Texture2D<float> depthTexture2;
[[vk::binding(6, 0)]] Texture2D<float> depthTexture3;
[[vk::binding(7, 0)]] Texture2D<float> depthTexture4;
[[vk::binding(8, 0)]] Texture2D<float> depthTexture5;
[[vk::binding(9, 0)]] Texture2D<float> depthTexture6;
[[vk::binding(10, 0)]] Texture2D<float> depthTexture7;
[[vk::binding(11, 0)]] SamplerState sampler;

[[vk::binding(0, 1)]] ConstantBuffer<UniformData> uniformData;
[[vk::binding(1, 1)]] RWStructuredBuffer<Range> aMeshTriangleIndexRanges;
[[vk::binding(2, 1)]] RWStructuredBuffer<MeshBBox> aMeshBBox;
[[vk::binding(3, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

#include "frustum-culling-headers.slang"

#define NUM_THREADS 256

[shader("compute")]
[numthreads(NUM_THREADS, 1, 1)]
void CSMain(
    uint3 localThread : SV_GroupThreadID,
    uint3 workGroup : SV_GroupID)
{
    uint iMesh = localThread.x + workGroup.x * NUM_THREADS;
    if(iMesh >= uniformData.miNumMeshes)
    {
        return;
    }

    uint32_t iVisible = aiVisibleMeshID[iMesh];
    if (iVisible > 0)
    {
        InterlockedAdd(aNumDrawCalls[3], 1u);
    }


    // check bbox against the depth pyramid
    bool bOccluded = cullBBoxDepth(
        aMeshBBox[iMesh].mMinPosition.xyz,
        aMeshBBox[iMesh].mMaxPosition.xyz,
        iMesh
    );

    bool bInside = cullBBox(
        aMeshBBox[iMesh].mMinPosition.xyz,
        aMeshBBox[iMesh].mMaxPosition.xyz,
        iMesh);    

    float3 center = (aMeshBBox[iMesh].mMaxPosition.xyz + aMeshBBox[iMesh].mMinPosition.xyz) * 0.5f;
    
    float4 xformCenter = mul(float4(center.xyz, 1.0f), defaultUniformBuffer.mViewProjectionMatrix);
    xformCenter.x /= xformCenter.w;
    xformCenter.y /= xformCenter.w;
    xformCenter.z /= xformCenter.w;

    aiVisibleMeshID[iMesh] = 0u;

    uint32_t iNumIndices = aMeshTriangleIndexRanges[iMesh].miEnd - aMeshTriangleIndexRanges[iMesh].miStart;
    uint32_t iIndexAddressOffset = aMeshTriangleIndexRanges[iMesh].miStart;
    uint iDrawCommandIndex = 0u;
    if(bInside && !bOccluded)
    {
        InterlockedAdd(aNumDrawCalls[0], 1u);
        InterlockedAdd(aNumDrawCalls[2], 1u, iDrawCommandIndex);

        aDrawCalls[iDrawCommandIndex].miIndexCount = iNumIndices;
        aDrawCalls[iDrawCommandIndex].miInstanceCount = 1u;
        aDrawCalls[iDrawCommandIndex].miFirstIndex = iIndexAddressOffset;
        aDrawCalls[iDrawCommandIndex].miBaseVertex = 0;
        aDrawCalls[iDrawCommandIndex].miFirstInstance = 0u;

        // mark as visible
        aiVisibleMeshID[iMesh] = 1u;
    }

    InterlockedAdd(aNumDrawCalls[3], 1u);
}

