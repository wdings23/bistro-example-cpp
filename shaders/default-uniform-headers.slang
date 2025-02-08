struct DefaultUniformData
{
    int32_t miScreenWidth;
    int32_t miScreenHeight;
    int32_t miFrame;
    int32_t miNumMeshes;

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

    float4x4 mInverseViewProjectionMatrix;

    float4 mCameraPosition;
    float4 mCameraLookDir;

    float4 mLightRadiance;
    float4 mLightDirection;

    float4 mPrevLightRadiance;
    float4 mPrevLightDirection;

    float mfAmbientOcclusionDistanceThreshold;
    float mfEmissiveRadiancePct;
    float mfClearReservoirs;
    float mfFiller;
};