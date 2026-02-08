#version 450

layout(location = 0) in vec3 inPosition; // binding 0
layout(location = 1) in vec3 inNormal;   // binding 1
layout(location = 2) in vec2 inUV;       // binding 2

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec2 vUV;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 viewProj;
} uCamera;

struct DrawData
{
    mat4 model;
    uint material_index;
    uint pad0;
    uint pad1;
    uint pad2;
};

layout(set = 0, binding = 1) readonly buffer Draws
{
    DrawData draws[];
} uDraws;

layout(push_constant) uniform constants
{
    uint drawId;
    uint materialIndex;
    uint packedMaterial;
} uDraw;

void main()
{
    DrawData draw = uDraws.draws[uDraw.drawId];
    gl_Position = uCamera.viewProj * draw.model * vec4(inPosition, 1.0);
    vNormal = mat3(draw.model) * inNormal;
    vUV = inUV;
}
