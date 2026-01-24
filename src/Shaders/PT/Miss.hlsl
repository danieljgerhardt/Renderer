
#include "PtCommon.hlsli"

[shader("miss")]
void Miss(inout Payload payload)
{
    float slope = normalize(WorldRayDirection()).y;
    float t = saturate(slope * 5 + 0.5);
    
    const float3 skyTopCol = float3(0.24, 0.44, 0.72);
    const float3 skyBottomCol = float3(0.75, 0.86, 0.93);
    payload.allowReflection = false;
    payload.color = lerp(skyBottomCol, skyTopCol, t);
    payload.missed = true;
}