struct VSOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VSOut vs_main(uint vertexId : SV_VertexID)
{
    const float2 positions[] = {
        {  0.0, -0.5 },
        {  0.5,  0.5 },
        { -0.5,  0.5 }
    };

    const float3 colors[] = {
            { 1.0, 0.0, 0.0 },
            { 0.0, 1.0, 0.0 },
            { 0.0, 0.0, 1.0 }
        };

    VSOut output;
    output.position = float4(positions[vertexId], 0.0, 1.0);
    output.color = float4(colors[vertexId], 1.0);
    return output;
}

float4 ps_main(VSOut input) : SV_TARGET
{
    return input.color;
}