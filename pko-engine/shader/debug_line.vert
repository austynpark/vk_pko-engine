#version 450 core

layout (location = 0) in vec3 position;

layout(set = 0, binding = 0) uniform transforms {
	mat4 projection;
	mat4 view;
} global_ubo;

void main()
{
	gl_Position = global_ubo.projection * global_ubo.view * vec4(position, 1.0f);
}