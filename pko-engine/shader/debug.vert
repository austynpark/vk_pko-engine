#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec4 weights;
layout (location = 4) in ivec4 bone_ids;

const int MAX_BONES = 64;

layout(push_constant) uniform constants {
    mat4 model;
    mat3 normal_matrix;
} object_ubo;

layout (set = 0, binding = 0) uniform transforms {
    mat4 projection;
    mat4 view;
} global_ubo;

layout (set = 1, binding = 0) uniform bone_transforms {
    mat4 transforms[MAX_BONES];
} bone;

void main()
{
    mat4 bone_transform = bone.transforms[bone_ids[0]];
    gl_Position = global_ubo.projection * global_ubo.view * object_ubo.model * bone_transform * vec4(position, 1.0);
}