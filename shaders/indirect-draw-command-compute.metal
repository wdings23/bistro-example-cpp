#include <metal_stdlib>

using namespace metal;

struct MeshRange
{
    uint32_t miStart;
    uint32_t miEnd;
};

// This is the argument buffer that contains the ICB.
struct ICBContainer
{
    command_buffer commandBuffer [[ id(0) ]];
};

struct VertexFormat
{
    float4 position;
    float4 texCoord;
    float4 normal;
};

kernel void createDrawCommands(
    uint iObjectIndex   [[ thread_position_in_grid ]],
    device MeshRange* aMeshRanges   [[ buffer(0) ]],
    device ICBContainer*    commandBufferContainer [[ buffer(1) ]],
    device uint32_t*        aiVisibleMeshBuffer [[ buffer(2) ]],
    device VertexFormat*    aVertexBuffer [[ buffer(3) ]],
    device uint32_t*        aiIndexBuffer [[ buffer(4) ]]
)
{
    //draw_indexed_primitives(
    //    primitive_type type, 
    //    uint index_count, 
    //    const device T *index_buffer, 
    //    uint instance_count, 
    //    uint base_vertex = 0, 
    //    uint base_instance = 0)


    uint32_t iNumVertices = aMeshRanges[iObjectIndex].miEnd - aMeshRanges[iObjectIndex].miStart;
    render_command cmd(commandBufferContainer->commandBuffer, iObjectIndex);
    if(aiVisibleMeshBuffer[iObjectIndex] > 0)
    {
        cmd.set_vertex_buffer(aVertexBuffer, 2);
        cmd.draw_indexed_primitives(
            primitive_type::triangle,
            iNumVertices,
            aiIndexBuffer,
            1, 
            aMeshRanges[iObjectIndex].miStart,
            iObjectIndex);
    }
}