implementing lighting;

static const float SPECULAR_STRENGTH = 1.0;

public float3 BlinnPhong(float3 L, float3 V, float3 N, float3 light_color, float3 albedo, float shininess) {
    float n_dot_l = dot(N, L);

    if (n_dot_l <= 0.0)
        return float3(0.0, 0.0, 0.0);

    float3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess) * SPECULAR_STRENGTH;

    return (albedo * n_dot_l + spec) * light_color;
}