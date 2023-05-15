struct VSIn
{
    float3 position : POSITION;
};

struct VSOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct UniformBufferData
{
    float4x4 projMat;
    float4x4 viewMat;
};
cbuffer UniformBuffer : register(b0)
{
    UniformBufferData ubo;
};

[[vk::push_constant]]
struct PushConstants
{
    float4x4 modelMat;
} constants;

VSOut vs_main(VSIn input)
{
    float4x4 viewProj = mul(ubo.projMat, ubo.viewMat);
    float4x4 modelViewProj = mul(viewProj, constants.modelMat);

    VSOut output;
    output.position = mul(modelViewProj, float4(input.position, 1.0));
    output.color = float4(1.0, 1.0, 1.0, 1.0);
    return output;
}

float4 ps_main(VSOut input) : SV_TARGET
{
    return input.color;
}