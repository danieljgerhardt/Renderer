
#include "PtCommon.hlsli"

[shader("raygeneration")]
void RayGeneration()
{
    uint2 idx = DispatchRaysIndex().xy;
    float2 size = DispatchRaysDimensions().xy;
    
    float2 uv = (idx + 0.5) / size;
    float2 ndc = uv * 2.0 - 1.0;
    
    float fovY = cameraPos.w == 0 ? 0.78 : cameraPos.w;
    float aspect = size.x / size.y;
    float tanHalfFov = tan(fovY * 0.5);
    
    float4 rayDir =
        forward +
        ndc.x * aspect * tanHalfFov * right +
        ndc.y * tanHalfFov * -up;
    
    RayDesc ray;
    ray.Origin = cameraPos.xyz;
    ray.Direction = normalize(rayDir.xyz);
    ray.TMin = 0.001f;
    ray.TMax = 1000.f;

    Payload payload;
    payload.allowReflection = true;
    payload.missed = false;
    payload.color = float3(0, 0, 0);
    
    TraceRay(scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);
    
    outputTexture[idx] = float4(payload.color, 1.0f);
}