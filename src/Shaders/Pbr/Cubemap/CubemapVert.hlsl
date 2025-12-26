
struct VSInput
{
    float3 Position : POSITION;
};

struct VSOutput
{
    float4 WorldPosition : POSITION;
    float4 ClipPosition : SV_POSITION;
};

cbuffer rc_viewMatrix : register(b0)
{
    float4x4 ViewMatrix;
};

VSOutput main( VSInput vsIn )
{
    VSOutput o;
    o.WorldPosition = float4(vsIn.Position, 1.f);
    o.ClipPosition = mul(float4(vsIn.Position, 1.0f), ViewMatrix);
	return o;
}