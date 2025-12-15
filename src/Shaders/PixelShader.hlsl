#include "RootSignature.hlsl"

Texture2D texture : register(t0);
SamplerState texSampler : register(s0);

cbuffer CameraMatrices : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

[RootSignature(ROOTSIG)]
float4 main(VSOutput vsOut) : SV_Target
{
    //return float4(vsOut.Normal * 0.5f + 0.5f, 1.0);
    float3 texColor = texture.SampleLevel(texSampler, vsOut.UV, 0.0).rgb;
    return float4(texColor, 1.0);

}