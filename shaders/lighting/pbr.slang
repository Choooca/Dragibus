implementing lighting;

static const float PI = 3.14159265359;

float3 FresnelSchlick(float cos_theta, float3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness) {

    float a = roughness * roughness;
    float a2 = a * a;
    float n_dot_h = max(dot(N, H), 0.0);
    float n_dot_h2 = n_dot_h * n_dot_h;

    float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / denom;
}

float GeometrySchlickGGX(float n_dot_v, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float denom = n_dot_v * (1.0 - k) + k;
    return n_dot_v / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float n_dot_v = max(dot(N, V), 0.0);
    float n_dot_l = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(n_dot_v, roughness);
    float ggx1 = GeometrySchlickGGX(n_dot_l, roughness);

    return ggx1 * ggx2;
}

float3 BRDF(float3 L, float3 N, float3 H, float3 V, float3 radiance, float3 albedo, float metallic, float roughness) {

    roughness = max(roughness, 0.045);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    float3 F0 = lerp(float3(0.04), albedo, metallic);
    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    float3 k_s = F;
    float3 k_d = float3(1.0) - k_s;
    k_d *= 1.0 - metallic;

    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;

    float n_dot_l = max(dot(N, L), 0.0);
    return (k_d * albedo / PI + specular) * radiance * n_dot_l;
}

float3 PBR_DirectionalLights(DirectionalLight light, float3 V, float3 N, float3 albedo, float metallic, float roughness) {

    float3 L = normalize(-light.direction);
    float3 H = normalize(V + L);
    return BRDF(L, N, H, V, light.color.rgb, albedo, metallic, roughness);
}

float3 PBR_PointLights(StructuredBuffer<PointLight> lights, float3 frag_pos, float3 V, float3 N, float3 albedo, float metallic, float roughness) {

    uint count, stride;
    lights.GetDimensions(count, stride);

    float3 color = 0.0;
    for (uint i = 0; i < count; ++i) {

        PointLight light = lights[i];

        float3 L = light.position - frag_pos;
        float dist = length(L);
        L = normalize(L);
        float attenuation = light.intensity / (dist * dist);
        float3 H = normalize(V + L);
        float3 radiance = light.color * attenuation;
        color += BRDF(L, N, H, V, radiance, albedo, metallic, roughness);
    }

    return color;
}

public float3 PBR(DirectionalLight directional_light, StructuredBuffer<PointLight> point_lights, float3 frag_pos, float3 V, float3 N, float3 albedo, float metallic, float roughness, float ao) {

    float3 color = PBR_DirectionalLights(directional_light, V, N, albedo, metallic, roughness);
    color += PBR_PointLights(point_lights, frag_pos, V, N, albedo, metallic, roughness);

    float3 ambient = float3(0.03) * albedo * ao;
    color += ambient;

    return color;
}