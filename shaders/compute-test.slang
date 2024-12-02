[[vk::binding(0, 0)]] RWStructuredBuffer<float> result;
[[vk::binding(1, 0)]] StructuredBuffer<float> buffer0;



[shader("compute")]
[numthreads(256, 1, 1)]
void CSMain(
    uint3 threadID: SV_DispatchThreadID)
{
    uint iThreadIndex = threadID.x;
    result[iThreadIndex] = iThreadIndex;
}