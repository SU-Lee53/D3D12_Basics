
Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse : register(s0);

cbuffer CONSTANT_BUFFER_DEFAULT : register(b0)
{
    matrix g_matWorld;
    matrix g_matView;
    matrix g_matProj;
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
    PSInput result = (PSInput) 0;

    //matrix matViewProj = mul(g_matView, g_matProj);
    //matrix matWVP = mul(g_matWorld, matViewProj);
    
    result.position = mul(input.position, g_matWorld);
    result.position = mul(result.position, g_matView);
    result.position = mul(result.position, g_matProj);
    result.TexCoord = input.TexCoord;
    result.color = input.color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    float4 texColor = texDiffuse.Sample(samplerDiffuse, input.TexCoord);
    return texColor * input.color;
}
