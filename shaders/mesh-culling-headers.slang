
struct DrawIndexParam
{
    int32_t miIndexCount;
    int32_t miInstanceCount;
    int32_t miFirstIndex;
    int32_t miBaseVertex;
    int32_t miFirstInstance;
};

struct MeshBBox
{
    float4 mMinPosition;
    float4 mMaxPosition;
};

struct Range
{
    int miStart;
    int miEnd;
};

struct UniformData
{
    uint miNumMeshes;
};