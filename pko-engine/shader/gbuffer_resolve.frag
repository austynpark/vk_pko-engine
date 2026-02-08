#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 0) out vec4 frag_color;

layout(set = 0, binding = 0) uniform texture2D uGBufferAlbedo;
layout(set = 0, binding = 1) uniform sampler uSampler;

void main()
{
    vec4 albedo = texture(sampler2D(uGBufferAlbedo, uSampler), v_uv);
    frag_color = albedo;
}
