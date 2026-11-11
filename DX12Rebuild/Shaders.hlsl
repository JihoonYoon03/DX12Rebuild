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
    light.x = -600.0f;
    light.y = 100.0f;
    light.z = -1000.0f;
    light = normalize(light - input.wpos.xyz);
    
    return (input.color * matBaseColor) * (0.2f + 0.8f * saturate(dot(normalize(input.normal), light)));
}

float4 PSTex(VS_OUTPUT input) : SV_Target
{
    //임시 방향광
    float3 light;
    light.x = -600.0f;
    light.y = 100.0f;
    light.z = -1000.0f;
    light = normalize(light - input.wpos.xyz);
    return tex1.Sample(sample, input.uv * tiling) * matBaseColor
    * (0.2f + 0.8 * saturate(dot(normalize(input.normal), light)));
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
float4 PSNormalMap(VS_OUTPUT input) : SV_Target
{
    //임시 방향광
    float3 light;
    light.x = -600.0f;
    light.y = 100.0f;
    light.z = -1000.0f;
    light = normalize(light - input.wpos.xyz);
 
    float2 normalUV;
    normalUV.x = input.uv.x;
    normalUV.y = 1 - input.uv.y;
    
    //노멀은 요소별로 -1~1, 노멀맵은 0~1사이 값을 가지므로, 노멀맵 색상 *2 - 1 해줘야함
    //추가로, 노멀맵은 z가 위쪽 방향임을 가정하므로, 스위즐링을 통해 z와 y를 바꿔줘야 함.
    float3 normal = normalMap.Sample(sample, normalUV).xzy * 2 - 1;
    //원본 노멀맵의 G가 역방향이어서, 값에 음수를 곱해줘야 함.
    normal.z = -normal.z;
    
    //노멀맵은 일단 타일링 끄기.
    //기본 텍스쳐 색상 * 머터리얼 베이스 색상 * 노멀에 따른 빛 받는 양
    return tex1.Sample(sample, input.uv * tiling) * matBaseColor
        * (0.2f + 0.8f * saturate(dot(light, normalize(normal))));
}