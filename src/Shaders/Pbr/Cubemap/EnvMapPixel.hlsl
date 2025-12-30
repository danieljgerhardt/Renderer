
TextureCube envMap : register(t0);

SamplerState envMapSampler : register(s0);

struct VSOutput
{
    float4 WorldPosition : POSITION;
    float4 ClipPosition : SV_POSITION;
};

float4 main(VSOutput psIn) : SV_TARGET
{
    float3 envColor = envMap.Sample(envMapSampler, normalize(psIn.WorldPosition.xyz)).rgb;
    
    //envColor = envColor / (envColor + float3(1.0f, 1.0f, 1.0f));
    //envColor = pow(envColor, float3(1.0f / 2.2f, 1.0f / 2.2f, 1.0f / 2.2f));
    
	return float4(envColor, 1.0f);
}