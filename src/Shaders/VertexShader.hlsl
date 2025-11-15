#include "RootSignature.hlsl"

cbuffer CameraMatrices : register(b0) {
    float4x4 viewMatrix;        // 16 floats
    float4x4 projectionMatrix;  // 16 floats
    float4x4 modelMatrix;
    float3 color;
};

struct VSInput
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD0;
    uint InstanceID : SV_InstanceID; // Instance ID for indexing into model matrices
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL;
};

[RootSignature(ROOTSIG)]
VSOutput main(VSInput input)
{
    float4 worldPos = mul(modelMatrix, float4(input.Position, 1.0));
    float4 viewPos = mul(viewMatrix, worldPos);
    
    VSOutput o;
    o.Position = mul(projectionMatrix, viewPos);
    o.Normal = input.Normal;
    
    return o;
}
