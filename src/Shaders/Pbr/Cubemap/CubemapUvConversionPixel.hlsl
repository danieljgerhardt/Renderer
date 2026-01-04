
Texture2D envMap : register(t0);

SamplerState envMapSampler : register(s0);

struct VSOutput
{
    float4 WorldPosition : POSITION;
    float4 ClipPosition : SV_POSITION;
};

cbuffer rc_viewMatrix : register(b0)
{
    float4x4 ViewMatrix;
};

static const float2 normalizeUv = float2(0.1591f, 0.3183f); // (1 / (2 * PI), 1 / PI))

float2 sampleSphericalMap(float3 v) {
    float2 uv = float2(atan2(v.x, v.z), asin(clamp(v.y, -1.0f, 1.0f)));
    uv *= normalizeUv;
    uv += 0.5f;
    return uv;
}

float4 main(VSOutput psIn) : SV_TARGET
{
    float3 direction = mul((float3x3) ViewMatrix, normalize(psIn.WorldPosition.xyz));
    float2 uv = sampleSphericalMap(normalize(direction));
    float3 color = envMap.SampleLevel(envMapSampler, uv, 0.0).rgb;
    
    return float4(color, 1.0f);
}