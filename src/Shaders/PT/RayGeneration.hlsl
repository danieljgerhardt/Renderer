
#include "PtCommon.hlsli"

[shader("raygeneration")]
void RayGeneration()
{
    uint2 idx = DispatchRaysIndex().xy;
    float2 size = DispatchRaysDimensions().xy;
    
    float3 testCamPos = float3(0, 1.5, -7);
    
    float2 uv = idx / size;
    float3 target = float3((uv.x * 2 - 1) * 1.8 * (size.x / size.y),
                           (1 - uv.y) * 4 - 2 + testCamPos.y,
                           0);
    
    RayDesc ray;
    ray.Origin = testCamPos.xyz;
    ray.Direction = normalize(target - testCamPos.xyz);
    ray.TMin = 0.001f;
    ray.TMax = 1000.f;

    Payload payload;
    payload.allowReflection = true;
    payload.missed = false;
    payload.color = float3(0, 0, 0);
    
    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    outputTexture[idx] = float4(payload.color, 1.0f);
}