[[vk::binding(0, 0)]] RWStructuredBuffer<int> InBuffer;
[[vk::binding(1, 0)]] RWStructuredBuffer<int> OutBuffer;

[numthreads(1, 1, 1)]
void Main(uint3 DTId : SV_DispatchThreadID)
{
    OutBuffer[DTId.x] = InBuffer[DTId.x] * InBuffer[DTId.x];
}