
Texture2D diffuseTexture : register(t0);
Texture2D metallicRoughness : register(t1);
Texture2D normalTexture : register(t2);

SamplerState texSampler : register(s0);

cbuffer rc_CameraMatrices : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4 cameraPos;
};

struct VSOutput
{
    float4 Position : SV_Position;
    float3 WorldPos : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

static const float3 light_pos[4] =
{
    float3(-10, 10, 10), float3(10, 10, 10), float3(-10, -10, 10), float3(10, -10, 10)
};

static const float3 light_col[4] =
{
    float3(300.f, 300.f, 300.f), float3(300.f, 300.f, 300.f), float3(300.f, 300.f, 300.f), float3(300.f, 300.f, 300.f)
};

static const float PI = 3.14159f;
static const float INV_PI = 0.318310155f;

float3 fresnel(float cosTheta, float3 R, float roughness)
{
    float3 oneMinusRough = float3(1.f - roughness, 1.f - roughness, 1.f - roughness);
    return R + (max(oneMinusRough, R) - R) * pow(clamp(1.f - cosTheta, 0.f, 1.f), 5.f);
}

float trowbridgeReitz(float3 wh, float3 nor, float roughness)
{
    float alpha2 = roughness * roughness;
    float NdotH = max(dot(nor, wh), 0.f);
    float denom = (NdotH * NdotH * ((alpha2 * alpha2) - 1.f) + 1.f);
    if (denom == 0.f)
    {
        return 0.f;
    }
    return (alpha2 * alpha2) / (PI * denom * denom);
}

float smithSchlickCos(float cos, float roughness)
{
    float k = (roughness + 1.f) * (roughness + 1.f) / 8.f;
    float denom = (cos * (1.f - k) + k);
    if (denom == 0.f)
    {
        return 0.f;
    }
    return cos / (cos * (1.f - k) + k);
}

float smithSchlickGGX(float3 wo, float3 wi, float3 nor, float roughness)
{
    float NdotWo = max(dot(nor, wo), 0.f);
    float NdotWi = max(dot(nor, wi), 0.f);
    return smithSchlickCos(NdotWo, roughness) * smithSchlickCos(NdotWi, roughness);
}

float3 reinhard(float3 Lo)
{
    return Lo / (Lo + float3(1.f, 1.f, 1.f));
}

float3 gammaCorrect(float3 Lo)
{
    float divRes = 1.f / 2.2f;
    return pow(Lo, float3(divRes, divRes, divRes));
}

float4 main(VSOutput vsOut) : SV_Target
{
    //float3 texColor = diffuseTexture.SampleLevel(texSampler, vsOut.UV, 0.f).rgb;
    //return float4(texColor, 1.0);
    
    float3 pos = vsOut.WorldPos.xyz;
    float3 nor = vsOut.Normal;
    float3 metalRough = metallicRoughness.SampleLevel(texSampler, vsOut.UV, 0.f).rgb;
    float metallic = metalRough.b;
    float roughness = metalRough.g;
    float3 albedo = diffuseTexture.SampleLevel(texSampler, vsOut.UV, 0.f).rgb;
    
    float ambientOccl = 1.0;

    float3 Lo = float3(0.f, 0.f, 0.f);

    float3 wo = normalize(cameraPos.xyz - pos);
    
    for (int i = 0; i < 4; i++)
    {
        float3 wi = normalize(light_pos[i] - pos);
        float3 wh = normalize((wo + wi) * 0.5f);
        float3 diff = light_pos[i] - pos;
        float attenuation = 1.f / dot(diff, diff);
        float3 radiance = light_col[i] * attenuation;
        float3 R = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);
        float3 F = fresnel(max(dot(nor, wo), 0.f), R, roughness);
        float D = trowbridgeReitz(wh, nor, roughness);
        float G = smithSchlickGGX(wo, wi, nor, roughness);

        float3 f_cook_torrance = D * G * F / (4.f * dot(nor, wo) * dot(nor, wi));
        float3 ks = F;
        float3 kd = float3(1.f, 1.f, 1.f) - ks;
        kd *= (1.f - metallic);
        float3 f_lambert = albedo * INV_PI;
        float3 f = kd * f_lambert + ks * f_cook_torrance;
        Lo += f * radiance * abs(dot(wi, nor));
    }

    float3 ambient = 0.03f * ambientOccl * albedo;
    Lo += ambient;

    Lo = reinhard(Lo);
    Lo = gammaCorrect(Lo);

    return float4(Lo, 1.f);
}