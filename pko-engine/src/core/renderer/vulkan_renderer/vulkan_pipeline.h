#ifndef VULKAN_PIPELINE_H
#define VULKAN_PIPELINE_H

struct RenderContext;
struct Shader;
struct PipelineLayout;
struct PipelineDesc;
struct Pipeline;

void vulkan_pipeline_layout_create(RenderContext* context, Shader* shader,
                                   PipelineLayout** out_layout);

void vulkan_pipeline_layout_destroy(RenderContext* context, PipelineLayout* layout);

void vulkan_graphics_pipeline_create(RenderContext* context, PipelineDesc* desc,
                                     Pipeline** out_pipeline);

// void vulkan_compute_pipeline_create();

void vulkan_graphics_pipeline_destroy(RenderContext* context, Pipeline* pipeline);

#endif  // !VULKAN_PIPELINE_H
