
struct VertexOutput {
    float4 pos : SV_Position;
    float2 uv;
}

[[vk::binding(0, 0)]]
Sampler2D screen_image;

[[vk::push_constant]]
float exposure;

[shader("vertex")]
VertexOutput VertMain(uint vertex_index: SV_VertexID) {
    VertexOutput output;
    output.uv = float2((vertex_index << 1) & 2, vertex_index & 2);
    output.pos = float4(output.uv * 2.0f - 1.0f, 0.0f, 1.0f);
    return output;
}

float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[shader("fragment")]
float4 FragMain(VertexOutput input) {
    float3 hdr = screen_image.Sample(input.uv).rgb;
    return float4(ACESFilm(hdr * exposure), 1.0f);
}