#include "vulkan_shader.h"

#include <mmgr/mmgr.h>

#include <SPIRV-Cross/spirv_cross.hpp>
#include <fstream>
#include <iostream>

#include "core/file_handle.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"

#include <stb_ds.h>

const char* shaderPathName = "shader/";
const char* spvExtension = ".spv";

b8 vulkan_shader_module_create(RenderContext* context, ShaderModule** ppOutModule,
                               const char* fileName);

b8 vulkan_shader_module_create(RenderContext* context, ShaderModule** ppOutModule,
                               const char* fileName)
{
    assert(context);
    assert(ppOutModule);
    assert(fileName);

    ShaderModule* pShaderModule = (ShaderModule*)malloc(sizeof(ShaderModule));
    memset(*ppOutModule, 0, sizeof(ShaderModule));

    size_t len = strlen(fileName) + strlen(shaderPathName) + strlen(spvExtension) + 1;
    char* fullPath = (char*)malloc(len * sizeof(char));

    strcpy_s(fullPath, len, shaderPathName);
    strcat_s(fullPath, len, fileName);
    strcat_s(fullPath, len, spvExtension);

    std::ifstream file(fullPath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("failed to open file!");
        return false;
    }

    size_t fileSize = (size_t)file.tellg();
    pShaderModule->codes.resize(fileSize / sizeof(u32));
    file.seekg(0);
    file.read((char*)pShaderModule->codes.data(), fileSize);
    file.close();

    pShaderModule->code_size = pShaderModule->codes.size() * sizeof(u32);

    VkShaderModuleCreateInfo create_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    create_info.pCode = pShaderModule->codes.data();
    create_info.codeSize = pShaderModule->code_size;

    VK_CHECK(vkCreateShaderModule(context->device_context.handle, &create_info, context->allocator,
                                  &pShaderModule->module));

    free(fullPath);
    fullPath = nullptr;

    *ppOutModule = pShaderModule;

    return true;
}

void vulkan_shader_reflect(ShaderReflection** ppOutShaderReflection, ShaderModule* pShaderModule)
{
    ShaderReflection* pShaderReflection = (ShaderReflection*)malloc(sizeof(ShaderReflection));
    memset(pShaderReflection, 0, sizeof(ShaderReflection));
    // pOut_ShaderReflection->stage_flag = get_file_extension(shaderModule-)
    spirv_cross::Compiler* compiler = new spirv_cross::Compiler(
        pShaderModule->codes.data(), pShaderModule->code_size / sizeof(u32));
    auto active = compiler->get_active_interface_variables();

    spirv_cross::ShaderResources shaderResources = compiler->get_shader_resources(active);
    compiler->set_enabled_interface_variables(std::move(active));

    u64 allResourceCount = 0;
    // shader stage input / output
    allResourceCount = shaderResources.stage_inputs.size();
    allResourceCount += shaderResources.stage_outputs.size();
    // shader buffers
    allResourceCount += shaderResources.uniform_buffers.size();
    allResourceCount += shaderResources.storage_buffers.size();
    // shader textures
    allResourceCount += shaderResources.separate_images.size();
    // shader sampler
    allResourceCount += shaderResources.separate_samplers.size();
    // shader sampled images
    allResourceCount += shaderResources.sampled_images.size();
    // shader push constant
    allResourceCount += shaderResources.push_constant_buffers.size();
    // shaderResources.gl_plain_uniforms
    allResourceCount += shaderResources.gl_plain_uniforms.size();

    pShaderReflection->resources =
        (ShaderResource*)calloc(allResourceCount, sizeof(ShaderResource));
    pShaderReflection->resource_count = 0;

    // stage input/output
    for (uint32_t i = 0; i < shaderResources.stage_inputs.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->resources[pShaderReflection->resource_count];
        resource.mIndex = pShaderReflection->resource_count++;
        u32 name_size = shaderResources.stage_inputs[i].name.length();
        resource.name = new char[name_size + 1];
        memcpy((char*)resource.name, shaderResources.stage_inputs[i].name.data(), name_size);
        ((char*)resource.name)[name_size] = 0;
        resource.mBinding = -1;
        resource.mSet = -1;

        spirv_cross::SPIRType type =
            compiler->get_type(shaderResources.stage_inputs[i].base_type_id);
        // bit width * vecsize = size
        resource.mSize = (type.width / 8) * type.vecsize;
        resource.mMemberCount = type.member_types.size();
        resource.mMembers =
            (resource.mMemberCount != 0)
                ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
                : nullptr;

        if (resource.mMemberCount != 0)
            memset(resource.mMembers, 0, sizeof(ShaderVariable) * resource.mMemberCount);

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            resource.name = new char[name_size + 1];
            memcpy((char*)resource.name, shaderResources.stage_inputs[i].name.data(), name_size);
            ((char*)resource.name)[name_size] = 0;

            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset =
                type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 ->
                // 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].name = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].name, compiler->get_member_name(type.self, m).data(),
                   member_name_size);
            ((char*)resource.mMembers[m].name)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.stage_outputs.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->resources[pShaderReflection->resource_count];
        resource.mIndex = pShaderReflection->resource_count++;
        u32 name_size = shaderResources.stage_outputs[i].name.length();
        resource.name = new char[name_size + 1];
        memcpy((char*)resource.name, shaderResources.stage_outputs[i].name.data(), name_size);
        ((char*)resource.name)[name_size] = 0;
        resource.mBinding = -1;
        resource.mSet = -1;

        // spirv_cross::Resource resource =
        // shaderResources.stage_inputs[resourceCount++];
        spirv_cross::SPIRType type =
            compiler->get_type(shaderResources.stage_outputs[i].base_type_id);

        // bit width * vecsize = size
        resource.mSize = (type.width / 8) * type.vecsize;
        resource.mMemberCount = type.member_types.size();
        resource.mMembers =
            (resource.mMemberCount != 0)
                ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
                : nullptr;

        if (resource.mMemberCount != 0)
            memset(resource.mMembers, 0, sizeof(ShaderVariable) * resource.mMemberCount);

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            spirv_cross::SPIRType member_type =
                compiler->get_type(shaderResources.stage_outputs[i].base_type_id);
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset =
                member_type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!member_type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (member_type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 ->
                // 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].name = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].name, compiler->get_member_name(type.self, m).data(),
                   member_name_size);
            ((char*)resource.mMembers[m].name)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.uniform_buffers.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->resources[pShaderReflection->resource_count];
        resource.mIndex = pShaderReflection->resource_count++;
        u32 name_size = shaderResources.uniform_buffers[i].name.length();
        resource.name = new char[name_size + 1];
        memcpy((char*)resource.name, shaderResources.uniform_buffers[i].name.data(), name_size);
        ((char*)resource.name)[name_size] = 0;
        resource.mBinding =
            compiler->get_decoration(shaderResources.uniform_buffers[i].id, spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.uniform_buffers[i].id,
                                                 spv::DecorationDescriptorSet);

        spirv_cross::SPIRType type =
            compiler->get_type(shaderResources.uniform_buffers[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers =
            (resource.mMemberCount != 0)
                ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
                : nullptr;
        resource.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        if (resource.mMemberCount != 0)
            memset(resource.mMembers, 0, sizeof(ShaderVariable) * resource.mMemberCount);

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            spirv_cross::SPIRType member_type =
                compiler->get_type(shaderResources.uniform_buffers[i].base_type_id);
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset =
                member_type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!member_type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (member_type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 ->
                // 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].name = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].name, compiler->get_member_name(type.self, m).data(),
                   member_name_size);
            ((char*)resource.mMembers[m].name)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.storage_buffers.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->resources[pShaderReflection->resource_count];
        resource.mIndex = pShaderReflection->resource_count++;
        u32 name_size = shaderResources.storage_buffers[i].name.length();
        resource.name = new char[name_size + 1];
        memcpy((char*)resource.name, shaderResources.storage_buffers[i].name.data(), name_size);
        ((char*)resource.name)[name_size] = 0;
        resource.mBinding =
            compiler->get_decoration(shaderResources.storage_buffers[i].id, spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.storage_buffers[i].id,
                                                 spv::DecorationDescriptorSet);

        // spirv_cross::Resource resource =
        // shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type =
            compiler->get_type(shaderResources.storage_buffers[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers =
            (resource.mMemberCount != 0)
                ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
                : nullptr;
        resource.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset =
                type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 ->
                // 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].name = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].name, compiler->get_member_name(type.self, m).data(),
                   member_name_size);
            ((char*)resource.mMembers[m].name)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.separate_images.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->resources[pShaderReflection->resource_count];
        resource.mIndex = pShaderReflection->resource_count++;
        u32 name_size = shaderResources.separate_images[i].name.length();
        resource.name = new char[name_size + 1];
        memcpy((char*)resource.name, shaderResources.separate_images[i].name.data(), name_size);
        ((char*)resource.name)[name_size] = 0;
        resource.mBinding =
            compiler->get_decoration(shaderResources.separate_images[i].id, spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.separate_images[i].id,
                                                 spv::DecorationDescriptorSet);

        // spirv_cross::Resource resource =
        // shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type =
            compiler->get_type(shaderResources.separate_images[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers =
            (resource.mMemberCount != 0)
                ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
                : nullptr;
        resource.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset =
                type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 ->
                // 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].name = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].name, compiler->get_member_name(type.self, m).data(),
                   member_name_size);
            ((char*)resource.mMembers[m].name)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.separate_samplers.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->resources[pShaderReflection->resource_count];
        resource.mIndex = pShaderReflection->resource_count++;
        u32 name_size = shaderResources.separate_samplers[i].name.length();
        resource.name = new char[name_size + 1];
        memcpy((char*)resource.name, shaderResources.separate_samplers[i].name.data(), name_size);
        ((char*)resource.name)[name_size] = 0;
        resource.mBinding = compiler->get_decoration(shaderResources.separate_samplers[i].id,
                                                     spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.separate_samplers[i].id,
                                                 spv::DecorationDescriptorSet);

        // spirv_cross::Resource resource =
        // shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type =
            compiler->get_type(shaderResources.separate_samplers[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers =
            (resource.mMemberCount != 0)
                ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
                : nullptr;
        resource.type = VK_DESCRIPTOR_TYPE_SAMPLER;
        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset =
                type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 ->
                // 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].name = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].name, compiler->get_member_name(type.self, m).data(),
                   member_name_size);
            ((char*)resource.mMembers[m].name)[member_name_size] = 0;
        }
    }

    // for (uint32_t i = 0; i < shaderResources.sampled_images.size(); ++i)
    //{
    //     ShaderResource& resource =
    //     pShaderReflection->resources[pShaderReflection->resource_count];
    //     resource.mIndex = pShaderReflection->resource_count++;
    //     u32 name_size = shaderResources.sampled_images[i].name.length();
    //     resource.name = new char[name_size + 1];
    //     memcpy((char*)resource.name,
    //     shaderResources.sampled_images[i].name.data(), name_size);
    //     ((char*)resource.name)[name_size] = 0;
    //     resource.mBinding =
    //     compiler->get_decoration(shaderResources.sampled_images[i].id,
    //     spv::DecorationBinding); resource.mSet =
    //     compiler->get_decoration(shaderResources.sampled_images[i].id,
    //     spv::DecorationDescriptorSet);

    //    //spirv_cross::Resource resource =
    //    shaderResources.storage_buffers[resourceCount++]; spirv_cross::SPIRType
    //    type =
    //    compiler->get_type(shaderResources.sampled_images[i].base_type_id);
    //    // bit width * vecsize = size
    //    resource.mMemberCount = type.member_types.size();
    //    resource.mMembers = (resource.mMemberCount != 0) ?
    //    (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
    //    : nullptr;

    //    resource.mIsStruct = false;
    //    resource.type = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    //    for (uint32_t m = 0; m < resource.mMemberCount; ++m)
    //    {
    //        size_t member_size = compiler->get_declared_struct_member_size(type,
    //        m); size_t offset = type.array.empty() > 1 ?
    //        compiler->type_struct_member_offset(type, m) : 0;

    //        resource.mMembers[m].mSize = member_size;
    //        resource.mMembers[m].mOffset = offset;
    //        resource.mSize += member_size;

    //        if (!type.array.empty())
    //        {
    //            size_t array_stride =
    //            compiler->type_struct_member_array_stride(type, m);
    //        }

    //        if (type.columns > 1)
    //        {
    //            // Get bytes stride between columns (if column major), for
    //            float4x4 -> 16 bytes. size_t matrix_stride =
    //            compiler->type_struct_member_matrix_stride(type, m);
    //        }

    //        u32 member_name_size = compiler->get_member_name(type.self,
    //        m).length(); resource.mMembers[m].name = new char[member_name_size +
    //        1]; memcpy((char*)resource.mMembers[m].name,
    //        compiler->get_member_name(type.self, m).data(), member_name_size);
    //        ((char*)resource.mMembers[m].name)[member_name_size] = 0;
    //    }
    //}

    // PushConstant
    for (uint32_t i = 0; i < shaderResources.push_constant_buffers.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->resources[pShaderReflection->resource_count];
        resource.mIndex = pShaderReflection->resource_count++;
        u32 name_size = shaderResources.push_constant_buffers[i].name.length();
        resource.name = new char[name_size + 1];
        memcpy((char*)resource.name, shaderResources.push_constant_buffers[i].name.data(),
               name_size);
        ((char*)resource.name)[name_size] = 0;
        resource.mBinding = -1;
        resource.mSet = -1;

        // spirv_cross::Resource resource =
        // shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type =
            compiler->get_type(shaderResources.push_constant_buffers[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers =
            (resource.mMemberCount != 0)
                ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
                : nullptr;

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset =
                type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 ->
                // 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].name = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].name, compiler->get_member_name(type.self, m).data(),
                   member_name_size);
            ((char*)resource.mMembers[m].name)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.gl_plain_uniforms.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->resources[pShaderReflection->resource_count];
        resource.mIndex = pShaderReflection->resource_count++;
        u32 name_size = shaderResources.gl_plain_uniforms[i].name.length();
        resource.name = new char[name_size + 1];
        memcpy((char*)resource.name, shaderResources.gl_plain_uniforms[i].name.data(), name_size);
        ((char*)resource.name)[name_size] = 0;
        resource.mBinding = compiler->get_decoration(shaderResources.gl_plain_uniforms[i].id,
                                                     spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.gl_plain_uniforms[i].id,
                                                 spv::DecorationDescriptorSet);

        // spirv_cross::Resource resource =
        // shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type =
            compiler->get_type(shaderResources.gl_plain_uniforms[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers =
            (resource.mMemberCount != 0)
                ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount)
                : nullptr;

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset =
                type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 ->
                // 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].name = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].name, compiler->get_member_name(type.self, m).data(),
                   member_name_size);
            ((char*)resource.mMembers[m].name)[member_name_size] = 0;
        }
    }

    if (compiler != nullptr)
        delete compiler;

    *ppOutShaderReflection = pShaderReflection;
}

void vulkan_shader_create(RenderContext* context, Shader** out_shader,
                          const ShaderLoadDesc* load_desc)
{
    Shader* shader = (Shader*)(calloc(1, sizeof(Shader)));
    ShaderReflection* shader_reflections[MAX_SHADER_STAGE_COUNT] = {};
    shader->vert_stage_index = (u32)(-1);
    shader->frag_stage_index = (u32)(-1);
    shader->comp_stage_index = (u32)(-1);

    u32 shaderCount = 0;

    for (u32 i = 0; i < MAX_SHADER_STAGE_COUNT; ++i)
    {
        if (!load_desc->names || load_desc->names[i] == 0)
            continue;

        const char* extension = get_file_extension(load_desc->names[i]);

        if (extension == "")
        {
            std::cout << "Add shader failed: " << load_desc->names[i] << " has no extension!"
                      << std::endl;
            return;
        }

        if (!strcmp(extension, "vert"))
        {
            if (shader->vert_stage_index != (u32)(-1))
            {
                std::cout << "Add shader failed: vertex stage already exists!" << std::endl;
                return;
            }

            shader->stage_flags |= VK_SHADER_STAGE_VERTEX_BIT;
            shader->vert_stage_index = i;
            shader->names[i] = load_desc->names[i];

            if (!vulkan_shader_module_create(context, &shader->shader_modules[i],
                                             load_desc->names[i]))
                return;

            vulkan_shader_reflect(&shader_reflections[i], shader->shader_modules[i]);
            shader_reflections[i]->stage_flag = VK_SHADER_STAGE_VERTEX_BIT;
            shaderCount++;
        }
        else if (!strcmp(extension, "frag"))
        {
            if (shader->frag_stage_index != (u32)(-1))
            {
                std::cout << "Add shader failed: fragment stage already exists!" << std::endl;
                return;
            }

            shader->stage_flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            shader->frag_stage_index = i;
            shader->names[i] = load_desc->names[i];

            if (!vulkan_shader_module_create(context, &shader->shader_modules[i],
                                             load_desc->names[i]))
                return;

            vulkan_shader_reflect(&shader_reflections[i], shader->shader_modules[i]);
            shader_reflections[i]->stage_flag = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderCount++;
        }
        else if (!strcmp(extension, "comp"))
        {
            if (shader->comp_stage_index != (u32)(-1))
            {
                std::cout << "Add shader failed: compute stage already exists!" << std::endl;
                return;
            }

            shader->stage_flags |= VK_SHADER_STAGE_COMPUTE_BIT;
            shader->comp_stage_index = i;
            shader->names[i] = load_desc->names[i];

            if (!vulkan_shader_module_create(context, &shader->shader_modules[i],
                                             load_desc->names[i]))
                return;

            vulkan_shader_reflect(&shader_reflections[i], shader->shader_modules[i]);
            shader_reflections[i]->stage_flag = VK_SHADER_STAGE_COMPUTE_BIT;
            shaderCount++;
        }
    }

    for (u32 shader_index = 0; shader_index < MAX_SHADER_STAGE_COUNT; ++shader_index)
    {
        ShaderReflection* shader_reflection = shader_reflections[shader_index];

        if (shader_reflection == NULL)
            continue;

        for (u32 resource_index = 0; resource_index < shader_reflection->resource_count;
             ++resource_index)
        {
            ShaderResource& resource = shader_reflection->resources[resource_index];
            resource.stage_flags |= shader_reflection->stage_flag;
            arrput(shader->shader_resources, resource);
        }
    }

    *out_shader = shader;
}

void vulkan_shader_destroy(RenderContext* context, Shader* pOutShader)
{
    for (u32 i = 0; i < MAX_SHADER_STAGE_COUNT; ++i)
    {
        if (!pOutShader->names[i] && pOutShader->shader_modules[i] != nullptr)
        {
            if (pOutShader->shader_modules[i]->module != VK_NULL_HANDLE)
                vkDestroyShaderModule(context->device_context.handle,
                                      pOutShader->shader_modules[i]->module, context->allocator);

            pOutShader->shader_modules[i]->module = VK_NULL_HANDLE;
            delete pOutShader->shader_modules[i];
            pOutShader->shader_modules[i];
        }
    }

    for (u32 r = 0; r < pOutShader->resource_count; ++r)
    {
        for (u32 m = 0; m < pOutShader->shader_resources[r].mMemberCount; ++m)
        {
            delete[] pOutShader->shader_resources[r].mMembers[m].name;
        }
        delete[] pOutShader->shader_resources[r].name;
        delete[] pOutShader->shader_resources[r].mMembers;
    }

    arrfree(pOutShader->shader_resources);

    free(pOutShader);
    pOutShader = NULL;
}
