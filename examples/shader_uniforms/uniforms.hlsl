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

VSOut vs_main(uint vertexId : SV_VertexID)
{
    const float2 positions[] = {
        {  0.0, 0.5 },
        { -0.5,  -0.5 },
        {  0.5,  -0.5 }
    };

    const float3 colors[] = {
            { 1.0, 0.0, 0.0 },
            { 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 1.0 }
        };

    float4x4 viewProj = mul(ubo.projMat, ubo.viewMat);
    float4x4 modelViewProj = mul(viewProj, constants.modelMat);

    VSOut output;
    output.position = mul(modelViewProj, float4(positions[vertexId], 0.0, 1.0));
    output.color = float4(colors[vertexId], 1.0);
    return output;
}

float4 ps_main(VSOut input) : SV_TARGET
{
    return input.color;
}