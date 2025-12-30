struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 WorldPosition : POSITION;
    float4 ClipPosition : SV_POSITION;
};

cbuffer rc_cameraMatrix : register(b0)
{
    float4x4 viewProjMatrix;
};

VSOutput main(VSInput vsIn)
{
    VSOutput o;
    o.WorldPosition = mul(viewProjMatrix, float4(vsIn.Position, 1.f));
    o.ClipPosition = o.WorldPosition.xyww;
    return o;
}