#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec2 vUV;

layout(location = 0) out vec4 outColor;

const int MAX_TEXTURES = 256;
const uint INVALID_TEX = 255u;

layout(set = 0, binding = 2) uniform texture2D uTextures[MAX_TEXTURES];
layout(set = 0, binding = 3) uniform sampler uSampler;

layout(push_constant) uniform constants
{
    uint drawId;
    uint materialIndex;
    uint packedMaterial;
} uDraw;

vec4 sample_texture(uint index, vec2 uv)
{
    if (index == INVALID_TEX)
    {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }

    return texture(sampler2D(uTextures[nonuniformEXT(index)], uSampler), uv);
}

void main()
{
    uint packed = uDraw.packedMaterial;
    uint base_color_index = (packed >> 0) & 0xffu;
    vec4 base_color = sample_texture(base_color_index, vUV);

    outColor = base_color;
}
