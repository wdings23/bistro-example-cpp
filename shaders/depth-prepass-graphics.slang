#include "default-uniform-headers.slang"
#include "mesh-culling-headers.slang"
#include "vertex-format-headers.slang"

[[vk::binding(0, 1)]] ConstantBuffer<DefaultUniformData> defaultUniformBuffer;

struct PSOutput
{
    float mfDepth : SV_TARGET0;
};

struct VertexStageOutput
{
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput VSMain(
    AssembledVertex assembledVertex)
{
    VertexStageOutput output;

    float4x4 viewMatrix = defaultUniformBuffer.mViewMatrix;
    float4x4 projectionMatrix = defaultUniformBuffer.mProjectionMatrix;

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

    return output;
}

[shader("fragment")]
PSOutput PSMain(VertexStageOutput in)
{
    PSOutput out;
    out.mfDepth = 1.0f;

    return out;
}