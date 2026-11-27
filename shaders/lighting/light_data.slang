implementing lighting;

public struct DirectionalLight {
    public float3 direction;
    public float4 color;
};

public struct PointLight {
    public float3 position;
    public float3 color;
    public float intensity;
};