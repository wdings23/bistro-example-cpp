#include "default-uniform-headers.slang"
#include "vertex-format-headers.slang"

struct VertexStageOutput
{
    float4 sv_position : SV_Position;
    float4 worldPosition;
    float4 texCoord;
    float4 normal;
    float4 currClipSpace;
    float4 prevClipSpace;
};

[[vk::binding(3, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformData;

[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    output.sv_position = mul(
        defaultUniformData.mJitteredViewProjectionMatrix,
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
        defaultUniformData.mPrevJitteredViewProjectionMatrix,
        float4(assembledVertex.position.xyz, 1.0f)
    );

    return output;
}