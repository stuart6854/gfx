struct VSIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VSOut
{
    float4 position : SV_POSITION;
    float3 positionWorld : TEXCOORD1;
    float3 normal : NORMAL;
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
    output.positionWorld = mul(constants.modelMat, float4(input.position, 1.0)).xyz;
    output.normal = input.normal;
    output.color = float4(1.0, 1.0, 1.0, 1.0);
    return output;
}

static const float3 LIGHT_COLOR = float3(1.0, 1.0, 1.0);
static const float3 LIGHT_POS = float3(3, 2, -1);

float4 ps_main(VSOut input) : SV_TARGET
{
    float3 objectColor = float3(1.0, 1.0, 1.0);

    float3 norm = normalize(input.normal);
    float3 lightDir = normalize(LIGHT_POS - input.positionWorld);

    float ambientStrength = 0.1;
    float3 ambientLight = ambientStrength * LIGHT_COLOR;

    float diff = max(dot(norm, lightDir), 0.0);
    float3 diffuseLight = diff * LIGHT_COLOR;

    float3 finalColor = (ambientLight + diffuseLight) * objectColor;
    return float4(finalColor, 1.0);
}