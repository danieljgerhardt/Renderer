
#include "PtCommon.hlsli"

[shader("raygeneration")]
void RayGeneration()
{
    uint2 idx = DispatchRaysIndex().xy;
    float2 size = DispatchRaysDimensions().xy;
    
    float2 uv = idx / size;
    float3 target = float3((uv.x * 2 - 1) * 1.8 * (size.x / size.y),
                           (1 - uv.y) * 4 - 2 + cameraPos.y,
                           0);
    
    RayDesc ray;
    ray.Origin = cameraPos;
    ray.Direction = normalize(target - cameraPos.xyz);
    ray.TMin = 0.001f;
    ray.TMax = 1000.f;

    Payload payload;
    payload.allowReflection = true;
    payload.missed = false;
    
    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    outputTexture[idx] = float4(payload.color, 1.0f);
}