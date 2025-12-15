#include "RootSignature.hlsl"

Texture2D texture : register(t0);
SamplerState texSampler : register(s0);

cbuffer CameraMatrices : register(b0) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    uint InstanceID : SV_InstanceID; // Instance ID for indexing into model matrices - currently unused
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

[RootSignature(ROOTSIG)]
VSOutput main(VSInput input)
{
    float4 worldPos = mul(modelMatrix, float4(input.Position, 1.0));
    float4 viewPos = mul(viewMatrix, worldPos);
    
    VSOutput o;
    o.Position = mul(projectionMatrix, viewPos);
    o.Normal = input.Normal;
    o.UV = input.UV;
    
    return o;
}
