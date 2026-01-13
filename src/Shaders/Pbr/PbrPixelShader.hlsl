
Texture2D diffuseTexture : register(t0);
Texture2D metallicRoughness : register(t1);
//Texture2D normalTexture : register(t2);

TextureCube diffuseIrradiance : register(t2);
TextureCube glossyIrradiance : register(t3);
Texture2D brdfLookup : register(t4);

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
    float3 pos = vsOut.WorldPos.xyz;
    float3 nor = vsOut.Normal;
    float3 metalRough = metallicRoughness.SampleLevel(texSampler, vsOut.UV, 0.f).rgb;
    //float3 metalRough = float3(0.0, 1.0, 0.0);
    float metallic = metalRough.b;
    float roughness = metalRough.g;
    float3 albedo = diffuseTexture.SampleLevel(texSampler, vsOut.UV, 0.f).rgb;
    //float3 albedo = float3(1.0, 0.0, 0.0);
    
    float ambientOccl = 1.0;

    float3 Lo = float3(0.f, 0.f, 0.f);

    float3 wo = normalize(cameraPos.xyz - pos);
    float3 wh = nor;
    float3 wi = reflect(-wo, wh);
    
    float3 baseReflectivty = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 kS = fresnel(max(dot(wh, wo), 0.f), baseReflectivty, roughness);
    float3 kD = (1.f - kS);
    float3 irradiance = diffuseIrradiance.SampleLevel(texSampler, nor, 0.f).rgb;
    float3 diffuse = irradiance * albedo;
    
    const float MAX_REFLECTION_LOD = 3.0;
    
    float3 prefilteredColor = glossyIrradiance.SampleLevel(texSampler, wi, roughness * MAX_REFLECTION_LOD).rgb;
    float2 brdf = brdfLookup.SampleLevel(texSampler, float2(max(dot(wh, wo), 0.f), roughness), 0.f).rg;
    float3 specular = prefilteredColor * (baseReflectivty * brdf.x + brdf.y);
    float3 ambient = (kD * diffuse + specular) * ambientOccl;
    float3 color = ambient + Lo;

    color = reinhard(color);
    color = gammaCorrect(color);

    return float4(color, 1.f);
}