
cbuffer CONSTANT_BUFFER_DEFAULT : register(b0)
{
    float4 g_Offset;
};


struct VSInput
{
    float4 position : POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

PSInput VSMain(VSInput input)
{
    PSInput result = (PSInput)0;

    result.position = input.position;
    result.position.xy += g_Offset.xy;
    result.TexCoord = input.TexCoord;
    result.color = input.color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
