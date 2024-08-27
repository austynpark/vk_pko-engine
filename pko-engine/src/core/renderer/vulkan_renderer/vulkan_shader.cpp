#include "vulkan_shader.h"
#include "vulkan_buffer.h"
#include "vulkan_pipeline.h"

#include "core/object.h"

#include "core/file_handle.h"

#include <mmgr/mmgr.h>
#include <SPIRV-Cross/spirv_cross.hpp>

#include <iostream>
#include <fstream>


const char* shaderPathName = "shader/";
const char* spvExtension = ".spv";

b8 vulkan_shader_module_create(VulkanContext* pContext, ShaderModule** ppOutModule, const char* fileName);

b8 vulkan_shader_module_create(VulkanContext* pContext, ShaderModule** ppOutModule, const char* fileName)
{
    assert(pContext);
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

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
        return false;
    }

    size_t fileSize = (size_t)file.tellg();
    pShaderModule->pCode.resize(fileSize / sizeof(u32));
    file.seekg(0);
    file.read((char*)pShaderModule->pCode.data(), fileSize);
    file.close();

    pShaderModule->codeSize = pShaderModule->pCode.size() * sizeof(u32);

    VkShaderModuleCreateInfo create_info{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    create_info.pCode = pShaderModule->pCode.data();
    create_info.codeSize = pShaderModule->codeSize;

    VK_CHECK(vkCreateShaderModule(pContext->device_context.handle, &create_info, pContext->allocator, &pShaderModule->mModule));

    free(fullPath);
    fullPath = nullptr;

    *ppOutModule = pShaderModule;

    return true;
}

void vulkan_shader_reflect(ShaderReflection** ppOutShaderReflection, ShaderModule* pShaderModule)
{
    ShaderReflection* pShaderReflection = (ShaderReflection*)malloc(sizeof(ShaderReflection));
    memset(pShaderReflection, 0, sizeof(ShaderReflection));
    //pOut_ShaderReflection->mStageFlag = get_file_extension(shaderModule-)
    spirv_cross::Compiler* compiler = new spirv_cross::Compiler(pShaderModule->pCode.data(), pShaderModule->codeSize / sizeof(u32));
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
    //shaderResources.gl_plain_uniforms
    allResourceCount += shaderResources.gl_plain_uniforms.size();

    pShaderReflection->mResourceCount = 0;
    pShaderReflection->pResources = (ShaderResource*)malloc(sizeof(ShaderResource) * allResourceCount);
    memset(pShaderReflection->pResources, 0, sizeof(ShaderResource) * allResourceCount);
    u32 resourceCount = 0;

    //stage input/output
    for (uint32_t i = 0; i < shaderResources.stage_inputs.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
        resource.mIndex = pShaderReflection->mResourceCount++;
        u32 name_size = shaderResources.stage_inputs[i].name.length();
        resource.pName = new char[name_size + 1];
        memcpy((char*)resource.pName, shaderResources.stage_inputs[i].name.data(), name_size);
        ((char*)resource.pName)[name_size] = 0;
        resource.mBinding = -1;
        resource.mSet = -1;

        spirv_cross::SPIRType type = compiler->get_type(shaderResources.stage_inputs[i].base_type_id);
        // bit width * vecsize = size
        resource.mSize = (type.width / 8) * type.vecsize;
        resource.mMemberCount = type.member_types.size();
        resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;

        if (resource.mMemberCount != 0)
            memset(resource.mMembers, 0, sizeof(ShaderVariable) * resource.mMemberCount);

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            resource.pName = new char[name_size + 1];
            memcpy((char*)resource.pName, shaderResources.stage_inputs[i].name.data(), name_size);
            ((char*)resource.pName)[name_size] = 0;

            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset = type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].pName = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
            ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
        }

    }

    for (uint32_t i = 0; i < shaderResources.stage_outputs.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
        resource.mIndex = pShaderReflection->mResourceCount++;
        u32 name_size = shaderResources.stage_outputs[i].name.length();
        resource.pName = new char[name_size + 1];
        memcpy((char*)resource.pName, shaderResources.stage_outputs[i].name.data(), name_size);
        ((char*)resource.pName)[name_size] = 0;
        resource.mBinding = -1;
        resource.mSet = -1;

        //spirv_cross::Resource resource = shaderResources.stage_inputs[resourceCount++];
        spirv_cross::SPIRType type = compiler->get_type(shaderResources.stage_outputs[i].base_type_id);

        // bit width * vecsize = size
        resource.mSize = (type.width / 8) * type.vecsize;
        resource.mMemberCount = type.member_types.size();
        resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;

        if (resource.mMemberCount != 0)
            memset(resource.mMembers, 0, sizeof(ShaderVariable) * resource.mMemberCount);

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            spirv_cross::SPIRType member_type = compiler->get_type(shaderResources.stage_outputs[i].base_type_id);
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset = member_type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!member_type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (member_type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }


            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].pName = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
            ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
        }
    }


    for (uint32_t i = 0; i < shaderResources.uniform_buffers.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
        resource.mIndex = pShaderReflection->mResourceCount++;
        u32 name_size = shaderResources.uniform_buffers[i].name.length();
        resource.pName = new char[name_size + 1];
        memcpy((char*)resource.pName, shaderResources.uniform_buffers[i].name.data(), name_size);
        ((char*)resource.pName)[name_size] = 0;
        resource.mBinding = compiler->get_decoration(shaderResources.uniform_buffers[i].id, spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.uniform_buffers[i].id, spv::DecorationDescriptorSet);

        spirv_cross::SPIRType type = compiler->get_type(shaderResources.uniform_buffers[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;
        resource.mType = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        if (resource.mMemberCount != 0)
            memset(resource.mMembers, 0, sizeof(ShaderVariable) * resource.mMemberCount);

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            spirv_cross::SPIRType member_type = compiler->get_type(shaderResources.uniform_buffers[i].base_type_id);
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset = member_type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!member_type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (member_type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].pName = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
            ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.storage_buffers.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
        resource.mIndex = pShaderReflection->mResourceCount++;
        u32 name_size = shaderResources.storage_buffers[i].name.length();
        resource.pName = new char[name_size + 1];
        memcpy((char*)resource.pName, shaderResources.storage_buffers[i].name.data(), name_size);
        ((char*)resource.pName)[name_size] = 0;
        resource.mBinding = compiler->get_decoration(shaderResources.storage_buffers[i].id, spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.storage_buffers[i].id, spv::DecorationDescriptorSet);

        //spirv_cross::Resource resource = shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type = compiler->get_type(shaderResources.storage_buffers[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;
        resource.mType = DESCRIPTOR_TYPE_STORAGE_BUFFER;
        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset = type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].pName = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
            ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.separate_images.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
        resource.mIndex = pShaderReflection->mResourceCount++;
        u32 name_size = shaderResources.separate_images[i].name.length();
        resource.pName = new char[name_size + 1];
        memcpy((char*)resource.pName, shaderResources.separate_images[i].name.data(), name_size);
        ((char*)resource.pName)[name_size] = 0;
        resource.mBinding = compiler->get_decoration(shaderResources.separate_images[i].id, spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.separate_images[i].id, spv::DecorationDescriptorSet);

        //spirv_cross::Resource resource = shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type = compiler->get_type(shaderResources.separate_images[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;
        resource.mType = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset = type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].pName = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
            ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.separate_samplers.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
        resource.mIndex = pShaderReflection->mResourceCount++;
        u32 name_size = shaderResources.separate_samplers[i].name.length();
        resource.pName = new char[name_size + 1];
        memcpy((char*)resource.pName, shaderResources.separate_samplers[i].name.data(), name_size);
        ((char*)resource.pName)[name_size] = 0;
        resource.mBinding = compiler->get_decoration(shaderResources.separate_samplers[i].id, spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.separate_samplers[i].id, spv::DecorationDescriptorSet);

        //spirv_cross::Resource resource = shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type = compiler->get_type(shaderResources.separate_samplers[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;
        resource.mType = DESCRIPTOR_TYPE_SAMPLER;
        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset = type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].pName = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
            ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
        }
    }

    //for (uint32_t i = 0; i < shaderResources.sampled_images.size(); ++i)
    //{
    //    ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
    //    resource.mIndex = pShaderReflection->mResourceCount++;
    //    u32 name_size = shaderResources.sampled_images[i].name.length();
    //    resource.pName = new char[name_size + 1];
    //    memcpy((char*)resource.pName, shaderResources.sampled_images[i].name.data(), name_size);
    //    ((char*)resource.pName)[name_size] = 0;
    //    resource.mBinding = compiler->get_decoration(shaderResources.sampled_images[i].id, spv::DecorationBinding);
    //    resource.mSet = compiler->get_decoration(shaderResources.sampled_images[i].id, spv::DecorationDescriptorSet);

    //    //spirv_cross::Resource resource = shaderResources.storage_buffers[resourceCount++];
    //    spirv_cross::SPIRType type = compiler->get_type(shaderResources.sampled_images[i].base_type_id);
    //    // bit width * vecsize = size
    //    resource.mMemberCount = type.member_types.size();
    //    resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;

    //    resource.mIsStruct = false;
    //    resource.mType = DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

    //    for (uint32_t m = 0; m < resource.mMemberCount; ++m)
    //    {
    //        size_t member_size = compiler->get_declared_struct_member_size(type, m);
    //        size_t offset = type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

    //        resource.mMembers[m].mSize = member_size;
    //        resource.mMembers[m].mOffset = offset;
    //        resource.mSize += member_size;

    //        if (!type.array.empty())
    //        {
    //            size_t array_stride = compiler->type_struct_member_array_stride(type, m);
    //        }

    //        if (type.columns > 1)
    //        {
    //            // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
    //            size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
    //        }

    //        u32 member_name_size = compiler->get_member_name(type.self, m).length();
    //        resource.mMembers[m].pName = new char[member_name_size + 1];
    //        memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
    //        ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
    //    }
    //}

    for (uint32_t i = 0; i < shaderResources.push_constant_buffers.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
        resource.mIndex = pShaderReflection->mResourceCount++;
        u32 name_size = shaderResources.push_constant_buffers[i].name.length();
        resource.pName = new char[name_size + 1];
        memcpy((char*)resource.pName, shaderResources.push_constant_buffers[i].name.data(), name_size);
        ((char*)resource.pName)[name_size] = 0;
        resource.mBinding = -1;
        resource.mSet = -1;
        resource.mType = DESCRIPTOR_TYPE_PUSH_CONSTANT;

        //spirv_cross::Resource resource = shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type = compiler->get_type(shaderResources.push_constant_buffers[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset = type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].pName = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
            ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
        }
    }

    for (uint32_t i = 0; i < shaderResources.gl_plain_uniforms.size(); ++i)
    {
        ShaderResource& resource = pShaderReflection->pResources[pShaderReflection->mResourceCount];
        resource.mIndex = pShaderReflection->mResourceCount++;
        u32 name_size = shaderResources.gl_plain_uniforms[i].name.length();
        resource.pName = new char[name_size + 1];
        memcpy((char*)resource.pName, shaderResources.gl_plain_uniforms[i].name.data(), name_size);
        ((char*)resource.pName)[name_size] = 0;
        resource.mBinding = compiler->get_decoration(shaderResources.gl_plain_uniforms[i].id, spv::DecorationBinding);
        resource.mSet = compiler->get_decoration(shaderResources.gl_plain_uniforms[i].id, spv::DecorationDescriptorSet);

        //spirv_cross::Resource resource = shaderResources.storage_buffers[resourceCount++];
        spirv_cross::SPIRType type = compiler->get_type(shaderResources.gl_plain_uniforms[i].base_type_id);
        // bit width * vecsize = size
        resource.mMemberCount = type.member_types.size();
        resource.mMembers = (resource.mMemberCount != 0) ? (ShaderVariable*)malloc(sizeof(ShaderVariable) * resource.mMemberCount) : nullptr;

        for (uint32_t m = 0; m < resource.mMemberCount; ++m)
        {
            size_t member_size = compiler->get_declared_struct_member_size(type, m);
            size_t offset = type.array.empty() > 1 ? compiler->type_struct_member_offset(type, m) : 0;

            resource.mMembers[m].mSize = member_size;
            resource.mMembers[m].mOffset = offset;
            resource.mSize += member_size;

            if (!type.array.empty())
            {
                size_t array_stride = compiler->type_struct_member_array_stride(type, m);
            }

            if (type.columns > 1)
            {
                // Get bytes stride between columns (if column major), for float4x4 -> 16 bytes.
                size_t matrix_stride = compiler->type_struct_member_matrix_stride(type, m);
            }

            u32 member_name_size = compiler->get_member_name(type.self, m).length();
            resource.mMembers[m].pName = new char[member_name_size + 1];
            memcpy((char*)resource.mMembers[m].pName, compiler->get_member_name(type.self, m).data(), member_name_size);
            ((char*)resource.mMembers[m].pName)[member_name_size] = 0;
        }
    }

    if (compiler != nullptr)
        delete compiler;

    *ppOutShaderReflection = pShaderReflection;

}

void vulkan_shader_create(VulkanContext* pContext, Shader** ppOutShader, const ShaderLoadDesc* pLoadDesc)
{
    Shader* pShader = (Shader*)(calloc(1, sizeof(Shader)));
    pShader->mVertStageIndex = (u32)(-1);
    pShader->mFragStageIndex = (u32)(-1);
    pShader->mCompStageIndex = (u32)(-1);

    u32 shaderCount = 0;

    for (u32 i = 0; i < MAX_SHADER_STAGE_COUNT; ++i)
    {
        if (!loadDesc->mNames || loadDesc->mNames[i] == 0)
            continue;

        const char* extension = get_file_extension(loadDesc->mNames[i]);

        if (extension == "") {
            std::cout << "Add shader failed: " << loadDesc->mNames[i] << " has no extension!" << std::endl;
            return;
        }

        if (!strcmp(extension, "vert"))
        {
            if (pShader->mVertStageIndex != (u32)(-1)) {
                std::cout << "Add shader failed: vertex stage already exists!" << std::endl;
                return;
            }

            pShader->stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
            pShader->mVertStageIndex = i;
            pShader->mNames[i] = loadDesc->mNames[i];


            if (!vulkan_shader_module_create(context, &pShader->pShaderModules[i], loadDesc->mNames[i]))
                return;

            vulkan_shader_reflect(&pShader->pShaderReflections[i], pShader->pShaderModules[i]);
            pShader->pShaderReflections[i]->mStageFlag = VK_SHADER_STAGE_VERTEX_BIT;
            shaderCount++;
        }
        else if (!strcmp(extension, "frag"))
        {
            if (pShader->mFragStageIndex != (u32)(-1)) {
                std::cout << "Add shader failed: fragment stage already exists!" << std::endl;
                return;
            }

            pShader->stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
            pShader->mFragStageIndex = i;
            pShader->mNames[i] = loadDesc->mNames[i];

            if (!vulkan_shader_module_create(context, &pShader->pShaderModules[i], loadDesc->mNames[i]))
                return;

            vulkan_shader_reflect(&pShader->pShaderReflections[i], pShader->pShaderModules[i]);
            pShader->pShaderReflections[i]->mStageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
            shaderCount++;
        }
        else if (!strcmp(extension, "comp"))
        {
            if (pShader->mCompStageIndex != (u32)(-1)) {
                std::cout << "Add shader failed: compute stage already exists!" << std::endl;
                return;
            }

            pShader->stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;
            pShader->mCompStageIndex = i;
            pShader->mNames[i] = loadDesc->mNames[i];

            if (!vulkan_shader_module_create(context, &pShader->pShaderModules[i], loadDesc->mNames[i]))
                return;


            vulkan_shader_reflect(&pShader->pShaderReflections[i], pShader->pShaderModules[i]);
            pShader->pShaderReflections[i]->mStageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
            shaderCount++;
        }
    }

    *ppOut_shader = pShader;
}

void vulkan_shader_destroy(VulkanContext* pContext, Shader* pOutShader)
{
    for (u32 i = 0; i < MAX_SHADER_STAGE_COUNT; ++i)
    {
        if (!pOutShader->mNames[i] && pOutShader->pShaderReflections[i] != nullptr && pOutShader->pShaderModules[i] != nullptr)
        {
            for (u32 r = 0; r < pOutShader->pShaderReflections[i]->mResourceCount; ++r)
            {
                for (u32 m = 0; m < pOutShader->pShaderReflections[i]->pResources[r].mMemberCount; ++m)
                {
                    delete[] pOutShader->pShaderReflections[i]->pResources[r].mMembers[m].pName;
                }
                delete[] pOutShader->pShaderReflections[i]->pResources[r].pName;
                delete[] pOutShader->pShaderReflections[i]->pResources[r].mMembers;
            }

            delete[] pOutShader->pShaderReflections[i]->pResources;

            delete pOutShader->pShaderReflections[i];
            pOutShader->pShaderReflections[i] = nullptr;

            if (pOutShader->pShaderModules[i]->mModule != VK_NULL_HANDLE)
                vkDestroyShaderModule(context->mDevice.pHandle, pOutShader->pShaderModules[i]->mModule, context->allocator);

            pOutShader->pShaderModules[i]->mModule = VK_NULL_HANDLE;
            delete pOutShader->pShaderModules[i];
            pOutShader->pShaderModules[i];
        }
    }

    free(pOutShader);
    pOutShader = NULL;
}
