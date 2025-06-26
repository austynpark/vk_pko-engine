#version 450 core
layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5),
    vec2(0.0, -0.5)
);

void main()
{
    //vs_out.uv = uv;
    //gl_Position = global_ubo.projection * global_ubo.view * object_ubo.model * vec4(position, 1.0);
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = vec3(1.0f, 0.0f, 0.0f);
}

