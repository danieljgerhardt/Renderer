#include "RootSignature.hlsl"

cbuffer CameraMatrices : register(b0) {
    float4x4 viewMatrix;        // 16 floats
    float4x4 projectionMatrix;  // 16 floats
    float4x4 modelMatrix;
    float3 color;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
};

[RootSignature(ROOTSIG)]
float4 main(VSOutput vs_Out) : SV_Target
{
    return float4(vs_Out.Normal * 0.5f + 0.5f, 1.0);
}