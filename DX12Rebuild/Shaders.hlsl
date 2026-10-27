cbuffer cbGameObjInfo : register(b0)
{
    matrix gmtxWorld;
    matrix gmtxView;
    matrix gmtxProjection;
};

Texture2D tex1 : register(t0);
SamplerState sample : register(s0);

struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(mul(mul(float4(input.pos, 1), gmtxWorld), gmtxView), gmtxProjection);
    output.color = input.color;
    output.uv = input.uv;
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_Target
{
    //return tex1.Sample(sample, input.uv);
    return input.color;
}