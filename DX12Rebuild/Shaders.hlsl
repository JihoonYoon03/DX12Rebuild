cbuffer cbGameObjInfo : register(b0)
{
    matrix gmtxWorld;
    matrix gmtxView;
    matrix gmtxProjection;
};

cbuffer cbMaterial : register(b1)
{
    float4 matBaseColor;
    float  tiling;
};

Texture2D tex1 : register(t0);
Texture2D normalMap : register(t1);
SamplerState sample : register(s0);

struct VS_INPUT
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 pos : SV_Position;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
    float3 wpos : POSITION;
    float3 normal : NORMAL;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(mul(mul(float4(input.pos, 1), gmtxWorld), gmtxView), gmtxProjection);
    output.color = input.color;
    output.uv = input.uv;
    output.wpos = mul(float4(input.pos, 1), gmtxWorld);
    output.normal = mul(float4(input.normal, 0), gmtxWorld);
    
    return output;
}

float4 PSMain(VS_OUTPUT input) : SV_Target
{
    //임시 방향광
    float3 light;
    light.xz = -100.0f;
    light.y = 100.0f;
    light = normalize(light - input.wpos.xyz);
    
    return (input.color * matBaseColor) * (0.2f + 0.8f * saturate(dot(normalize(input.normal), light)));
}

float4 PSTex(VS_OUTPUT input) : SV_Target
{
    //임시 방향광
    float3 light;
    light.xz = -100.0f;
    light.y = 100.0f;
    light = normalize(light - input.wpos.xyz);
    return (tex1.Sample(sample, input.uv * tiling) * matBaseColor) * (0.2f + 0.8 * saturate(dot(normalize(input.normal), light)));
}

//Unlit
float4 PSSky(VS_OUTPUT input) : SV_Target
{
    return tex1.Sample(sample, input.uv * tiling);
}

float4 PSUnlit(VS_OUTPUT input) : SV_Target
{
    return input.color * matBaseColor;
}

//머터리얼이 텍스쳐 관리하도록 해서 수정 필요
float4 PSNormalMappingMain(VS_OUTPUT input) : SV_Target
{
    //임시 방향광
    float3 light;
    light.xyz = 100.0f;
    
    return 0.5f * dot(light, normalMap.Sample(sample, input.uv * tiling).xyz)
            + input.color * matBaseColor;
}