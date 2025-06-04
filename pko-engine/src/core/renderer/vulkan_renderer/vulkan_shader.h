#ifndef VULKAN_SHADER_H
#define VULKAN_SHADER_H

#include "vulkan_types.inl"

void vulkan_shader_create(RenderContext* pContext, Shader** ppOutShader,
                          const ShaderLoadDesc* pLoadDesc);
void vulkan_shader_destroy(RenderContext* context, Shader* pShader);

#endif  // !VULKAN_SHADER_H
