
#ifndef PT_COMMON_HLSLI
#define PT_COMMON_HLSLI

static const float3 light = float3(0, 200, 0);

struct Payload
{
    float3 color;
    bool allowReflection;
    bool missed;
};

cbuffer rc_Camera : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4 cameraPos;
};

RaytracingAccelerationStructure scene : register(t0);

RWTexture2D<float4> outputTexture : register(u0);

#endif // PT_COMMON_HLSLI