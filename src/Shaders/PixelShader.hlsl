#include "RootSignature.hlsl"

cbuffer CameraMatrices : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float3 Color : COLOR0;
};

[RootSignature(ROOTSIG)]
float4 main(VSOutput vs_Out) : SV_Target
{
    //return float4(vs_Out.Normal * 0.5f + 0.5f, 1.0);
    return float4(vs_Out.Color, 1.0);

}