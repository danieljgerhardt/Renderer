
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

static const float PI = 3.14159265359f;

static const float u_Roughness = 0.5f;

float DistributionGGX(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.f);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.f) + 1.f);
    denom = PI * denom * denom;

    return nom / denom;
}

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley(uint i, uint N) {
    return float2(float(i) / float(N), RadicalInverse_VdC(i));
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.f * PI * Xi.x;
    float cosTheta = sqrt((1.f - Xi.y) / (1.f + (a * a - 1.f) * Xi.y));
    float sinTheta = sqrt(1.f - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates - halfway vector
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space H vector to world-space sample vector
    float3 up = abs(N.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);

    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float4 main(VSOutput psIn) : SV_TARGET 
{
    float3 wh = mul((float3x3) ViewMatrix, normalize(psIn.WorldPosition.xyz));
    float3 R = wh;
    float3 V = R;

    const uint SAMPLE_COUNT = 512u;
    float3 prefilteredColor = float3(0.f, 0.f, 0.f);
    float totalWeight = 0.f;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, wh, u_Roughness);
        float3 L = normalize(2.f * dot(V, H) * H - V);

        float NdotL = max(dot(wh, L), 0.f);
        if (NdotL > 0.f)
        {
            float D = DistributionGGX(wh, H, u_Roughness);
            float NdotH = max(dot(wh, H), 0.f);
            float HdotV = max(dot(H, V), 0.f);
            float pdf = D * NdotH / (4.f * HdotV) + 0.0001f;

            float resolution = 512.f;
            float saTexel = 4.f * PI / (6.f * resolution * resolution);
            float saSample = 1.f / (float(SAMPLE_COUNT) * pdf + 0.0001f);

            float mipLevel = u_Roughness == 0.f ? 0.f : 0.5f * log2(saSample / saTexel);

            prefilteredColor += envMap.SampleLevel(envMapSampler, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;

    return float4(prefilteredColor, 1.f);
}