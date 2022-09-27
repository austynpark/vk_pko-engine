#version 450 core
layout (location = 0) out vec4 frag_color;

layout (location = 0) in VS_IN {
    vec2 uv;
	vec3 normal;
} vs_in;

//TODO: move binding to 1, 2 after Material
//layout(set = 2, binding = 0) uniform sampler2D diffuse_texture;
//layout(set = 2, binding = 1) uniform sampler2D specular_texture;

void main()
{
	//vec3 color = texture(diffuse_texture,vs_in.uv).xyz;
	frag_color = vec4(vs_in.normal, 1.0f);
}