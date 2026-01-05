
TextureCube envMap : register(t0);

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

static const float PI = 3.14159265359;

float4 main(VSOutput psIn) : SV_TARGET
{
    float3 N = mul((float3x3) ViewMatrix, normalize(psIn.WorldPosition.xyz));
    
    float3 radiance = envMap.Sample(envMapSampler, N).rgb;

    // Convolution: fixed amount of sample vectors
    float3 irradiance = float3(0.f, 0.f, 0.f);

    float3 up = float3(0.f, 1.f, 0.f);
    float3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleDelta = 0.025f;
    float nrSamples = 0.f;
    for (float phi = 0.f; phi < 2.f * PI; phi += sampleDelta)
    {
        for (float theta = 0.f; theta < 0.5f * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += envMap.Sample(envMapSampler, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    // reinhard operator
    irradiance = PI * irradiance * (1.f / float(nrSamples));
    
    return float4(irradiance, 1.0f);
}