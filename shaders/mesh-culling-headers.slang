struct DefaultUniformData
{
    uint miScreenWidth;
    uint miScreenHeight;
    uint miFrame;
    uint miNumMeshes;

    float mfRand0;
    float mfRand1;
    float mfRand2;
    float mfRand3;

    float4x4 mViewProjectionMatrix;
    float4x4 mPrevViewProjectionMatrix;
    float4x4 mViewMatrix;
    float4x4 mProjectionMatrix;

    float4x4 mJitteredViewProjectionMatrix;
    float4x4 mPrevJitteredViewProjectionMatrix;

    float4 mCameraPosition;
    float4 mCameraLookDir;

    float4 mLightRadiance;
    float4 mLightDirection;

    float mfAmbientOcclusionDistanceThreshold;
};

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