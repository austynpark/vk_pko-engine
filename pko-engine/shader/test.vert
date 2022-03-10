#version 450 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;

layout(push_constant) uniform transforms
{
    mat4 projection;
    mat4 view;
};

//uniform mat4 model;
//uniform mat4 normalMatrix;

void main()
{
    //vs_out.fragNormal = (normalMatrix * vec4(vNormal, 0.0)).xyz; 

    gl_Position = projection * view * vec4(vPos, 1.0);
}
