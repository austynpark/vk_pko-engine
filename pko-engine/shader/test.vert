#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;

layout (location = 0) out VS_OUT {
    vec2 uv;
    vec3 norm;
} vs_out;

layout(push_constant) uniform constants {
    mat4 model;
    mat3 normal_matrix;
} object_ubo;

layout (set = 0, binding = 0) uniform transforms {
    mat4 projection;
    mat4 view;
} global_ubo;

void main()
{
    vs_out.uv = uv;
    vs_out.norm = normal;
    gl_Position = global_ubo.projection * global_ubo.view * object_ubo.model * vec4(position, 1.0);
}

