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
ConstantBuffer<UniformBuffer> ubo;

struct VertexOutput {
    float4 position : SV_Position;
    float2 texcoord_0;
};

Sampler2D texture;

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
    return texture.Sample(in_vert.texcoord_0);
}