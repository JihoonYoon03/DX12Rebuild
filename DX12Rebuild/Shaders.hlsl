cbuffer cbGameObjInfo : register(b0)
{
    matrix gmtxWorld : packoffset(c0);
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 color : COLOR;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(float4(input.pos, 1), gmtxWorld);
    output.color = input.color;
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_Target
{
    return input.color;
}