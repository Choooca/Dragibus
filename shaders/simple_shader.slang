struct VSInput {
    float3 position;
    float3 normal;
    float4 tangent;
    float2 tex_coord0;
};

struct DirectionalLight {
    float3 direction;
    float4 color;
};

struct GlobalTransformation {
    float4x4 view;
    float4x4 proj;
};

struct NodeTransformation {
    float4x4 model;
    float4x4 normal;
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

static const float ambient = 0.2;

[[vk::push_constant]]
NodeTransformation node_transform;

[[vk::binding(0, 0)]]
ConstantBuffer<DirectionalLight> directional_light;

[[vk::binding(0, 1)]]
ConstantBuffer<GlobalTransformation> global_transform;

struct VertexOutput {
    float4 position : SV_Position;
    float3 normal;
    float2 texcoord_0;
    float3x3 TBN;
};

[[vk::binding(0, 2)]]
Sampler2D material_textures[MATERIAL_TEXTURE_COUNT];

[[vk::binding(1, 2)]]
ConstantBuffer<MaterialUniform> material_ubo;

[shader("vertex")]
VertexOutput VertMain(VSInput input) {
    VertexOutput output;
    output.position = mul(global_transform.proj, mul(global_transform.view, mul(node_transform.model, float4(input.position, 1.0))));
    output.texcoord_0 = input.tex_coord0;
    output.normal = mul(float3x3(node_transform.normal), input.normal);

    float3 T = normalize(mul(node_transform.model, float4(input.tangent.xyz, 0.0)).xyz);
    float3 B = normalize(mul(node_transform.model, float4(cross(input.normal, input.tangent.xyz) * input.tangent.w, 0.0)).xyz);
    float3 N = normalize(mul(node_transform.normal, float4(input.normal, 0.0)).xyz);
    output.TBN = transpose(float3x3(T, B, N));

    return output;
}

[shader("fragment")]
float4 FragMain(VertexOutput input) : SV_Target
{
    float3 normal = material_textures[NORMAL].Sample(input.texcoord_0).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(mul(input.TBN, normal));

    float diffuse = max(dot(normalize(normal), -directional_light.direction), 0);
    float4 tex_color = material_textures[BASE_COLOR].Sample(input.texcoord_0) * material_ubo.base_color;
    float3 shaded_color = tex_color.rgb * directional_light.color.xyz * (diffuse + ambient);
    return float4(shaded_color, tex_color.a);
}