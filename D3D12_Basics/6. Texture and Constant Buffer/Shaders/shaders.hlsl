
Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse : register(s0);

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
    PSInput result;

    result.position = input.position;
    result.TexCoord = input.TexCoord;
    result.color = input.color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = texDiffuse.Sample(samplerDiffuse, input.TexCoord);
    return texColor * input.color;
}
