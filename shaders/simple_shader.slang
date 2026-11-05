struct VSInput {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 tex_coord0;
};

struct UniformBuffer {
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

struct MaterialUniform {
    float4 base_color;
    float3 emissive;
    float metallic_factor;
    float roughness_factor;
};

static const int BASE_COLOR = 0;
static const int EMMISIVE = 1;
static const int NORMAL = 2;
static const int OCCLUSION = 3;
static const int METALLIC_ROUGHNESS = 4;
static const int MATERIAL_TEXTURE_COUNT = 5;

[[vk::binding(0, 0)]]
ConstantBuffer<UniformBuffer> ubo;

struct VertexOutput {
    float4 position : SV_Position;
    float2 texcoord_0;
};

[[vk::binding(0, 1)]]
Sampler2D material_textures[MATERIAL_TEXTURE_COUNT];

[[vk::binding(1, 1)]]
ConstantBuffer<MaterialUniform> material_ubo;

[shader("vertex")]
VertexOutput VertMain(VSInput input) {
    VertexOutput output;
    output.position = mul(ubo.proj, mul(ubo.view, mul(ubo.model, float4(input.position, 1.0))));
    output.texcoord_0 = input.tex_coord0;
    return output;
}

[shader("fragment")]
float4 FragMain(VertexOutput in_vert) : SV_Target
{
    return material_textures[BASE_COLOR].Sample(in_vert.texcoord_0);
}