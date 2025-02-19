// Per-vertex attributes to be assembled from bound vertex buffers.
struct AssembledVertex
{
    float4 position : POSITION;
    float4 uv : TEXCOORD;
};

struct VertexStageOutput
{
    float4          sv_position     : SV_Position;
    float4          texcoord;
};

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
