
#include "PtCommon.hlsli"

[shader("closesthit")]
void ClosestHit(inout Payload payload, BuiltInTriangleIntersectionAttributes attribs)
{
    float2 uv = attribs.barycentrics;
    
    payload.color = float3(uv, 0.0);
}