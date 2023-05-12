struct VSOut
{
    float4 position : SV_POSITION;
};

VSOut vs_main(uint vertexId : SV_VertexID)
{
    const float2 positions[] = {
        {  0.0, -0.5 },
        {  0.5,  0.5 },
        { -0.5,  0.5 }
    };

    VSOut output;
    output.position = float4(positions[vertexId], 0.0, 1.0);
    return output;
}

float4 ps_main(VSOut input) : SV_TARGET
{
    return float4(1.0, 0.0, 1.0, 1.0);
}