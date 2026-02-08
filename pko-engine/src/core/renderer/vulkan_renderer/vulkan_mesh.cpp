#include "vulkan_mesh.h"

#include "vulkan_buffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_image.h"
#include "stb_ds.h"
#include "stb_image.h"

#include <basis_universal/transcoder/basisu_transcoder.h>

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 4996)

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <string.h>
#include <iostream>

b8 load_raw_images_data_from_gltf(cgltf_data* data, const char* gltf_filename,
                                  RawImageData** out_images);
b8 load_meshes_from_gltf(cgltf_data* data, Mesh** out_meshes);

constexpr i32 CHANNELS_RGBA = 4;

static void create_device_local_buffer_from_data(RenderContext* context, const void* src_data,
                                                 u64 data_size, VkBufferUsageFlags usage,
                                                 Buffer* out_buffer)
{
    // Create staging buffer (host visible)
    Buffer staging;
    vulkan_buffer_create(
        context, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        &staging);

    // Upload to staging
    vulkan_buffer_upload(context, &staging, (void*)src_data, (u32)data_size);

    // Create device-local buffer (copy dest | usage)
    vulkan_buffer_create(context, data_size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, out_buffer);

    // Copy from staging to device-local
    vulkan_buffer_copy(context, &staging, out_buffer, data_size);

    // Cleanup staging
    vulkan_buffer_destroy(context, &staging);
}

void join_gltf_image_path(const char* gltf_filename, const char* image_uri, char* out_path,
                          size_t out_path_size)
{
    // gltf_filename?ì„œ ë§ˆì?ë§?'/' ?ëŠ” '\\' ?„ì¹˜ ì°¾ê¸°
    const char* last_slash = strrchr(gltf_filename, '/');
    const char* last_backslash = strrchr(gltf_filename, '\\');
    const char* dir_end = last_slash > last_backslash ? last_slash : last_backslash;

    size_t dir_len = dir_end ? (size_t)(dir_end - gltf_filename + 1) : 0;
    if (dir_len + strlen(image_uri) + 1 > out_path_size)
    {
        out_path[0] = 0;
        return;
    }

    memcpy(out_path, gltf_filename, dir_len);
    out_path[dir_len] = 0;
    strcat(out_path, image_uri);
}

cgltf_data* load_cgltf_data(const char* filename);

void gather_instances_from_node(cgltf_data* data, cgltf_node* node, const glm::mat4 parent_world,
                                const u32* mesh_first_range, const u32* mesh_range_count,
                                MeshInstance** out_instances)
{
    glm::mat4 local_transform(1.0f);

    if (node->has_matrix)
    {
        local_transform = glm::make_mat4(node->matrix);
    }
    else
    {
        glm::vec3 translation(0.0f);
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale(1.0f);

        if (node->has_translation)
            translation = glm::make_vec3(node->translation);
        if (node->has_rotation)
            rotation = glm::quat(node->rotation[3], node->rotation[0], node->rotation[1],
                                 node->rotation[2]);
        if (node->has_scale)
            scale = glm::make_vec3(node->scale);

        local_transform =
            glm::translate(translation) * glm::mat4_cast(rotation) * glm::scale(scale);
    }

    glm::mat4 world_transform = parent_world * local_transform;

    if (node->mesh)
    {
        u32 mesh_index = (u32)(node->mesh - data->meshes);
        const u32 first_range = mesh_first_range[mesh_index];
        const u32 range_count = mesh_range_count[mesh_index];
        for (u32 i = 0; i < range_count; ++i)
        {
            MeshInstance instance = {};
            instance.model_matrix = world_transform;
            instance.range_index = first_range + i;
            arrput(*out_instances, instance);
        }
    }

    for (cgltf_size i = 0; i < node->children_count; ++i)
    {
        gather_instances_from_node(data, node->children[i], world_transform, mesh_first_range,
                                   mesh_range_count, out_instances);
    }
}

b8 load_meshes_from_gltf(cgltf_data* data, Mesh** out_meshes)
{
    if (!data || !out_meshes)
        return false;

    Mesh mesh = {};
    MeshRange* ranges = NULL;
    f32* positions = NULL;
    f32* normals = NULL;
    f32* uvs = NULL;
    u32* indices = NULL;

    u32 vertex_offset = 0;
    u32 index_offset = 0;

    u32* mesh_first_range = (u32*)malloc(sizeof(u32) * data->meshes_count);
    u32* mesh_range_count = (u32*)malloc(sizeof(u32) * data->meshes_count);

    for (cgltf_size mesh_idx = 0; mesh_idx < data->meshes_count; ++mesh_idx)
    {
        const u32 range_start = (u32)arrlen(ranges);
        const cgltf_mesh* cgltfMesh = &data->meshes[mesh_idx];
        for (cgltf_size prim_idx = 0; prim_idx < cgltfMesh->primitives_count; ++prim_idx)
        {
            const cgltf_primitive* prim = &cgltfMesh->primitives[prim_idx];

            const cgltf_accessor* pos_accessor = NULL;
            const cgltf_accessor* norm_accessor = NULL;
            const cgltf_accessor* uv_accessor = NULL;

            for (cgltf_size attr_idx = 0; attr_idx < prim->attributes_count; ++attr_idx)
            {
                const cgltf_attribute* attr = &prim->attributes[attr_idx];
                if (attr->type == cgltf_attribute_type_position)
                    pos_accessor = attr->data;
                if (attr->type == cgltf_attribute_type_normal)
                    norm_accessor = attr->data;
                if (attr->type == cgltf_attribute_type_texcoord)
                    uv_accessor = attr->data;
            }

            if (!pos_accessor)
                continue;

            cgltf_size vertex_count = pos_accessor->count;
            cgltf_size index_count = prim->indices ? prim->indices->count : 0;

            // Positions
            f32* temp_positions = (f32*)malloc(sizeof(f32) * vertex_count * 3);
            cgltf_accessor_unpack_floats(pos_accessor, temp_positions, vertex_count * 3);
            for (cgltf_size v = 0; v < vertex_count * 3; ++v) arrput(positions, temp_positions[v]);
            free(temp_positions);

            // Normals
            if (norm_accessor)
            {
                f32* temp_normals = (f32*)malloc(sizeof(f32) * vertex_count * 3);
                cgltf_accessor_unpack_floats(norm_accessor, temp_normals, vertex_count * 3);
                for (cgltf_size v = 0; v < vertex_count * 3; ++v) arrput(normals, temp_normals[v]);
                free(temp_normals);
            }
            else
            {
                for (cgltf_size v = 0; v < vertex_count; ++v)
                {
                    arrput(normals, 0.0f);
                    arrput(normals, 1.0f);
                    arrput(normals, 0.0f);
                }
            }

            // UVs
            if (uv_accessor)
            {
                f32* temp_uvs = (f32*)malloc(sizeof(f32) * vertex_count * 2);
                cgltf_accessor_unpack_floats(uv_accessor, temp_uvs, vertex_count * 2);
                for (cgltf_size v = 0; v < vertex_count * 2; ++v) arrput(uvs, temp_uvs[v]);
                free(temp_uvs);
            }
            else
            {
                for (cgltf_size v = 0; v < vertex_count; ++v)
                {
                    arrput(uvs, 0.0f);
                    arrput(uvs, 0.0f);
                }
            }

            // Indices
            if (prim->indices)
            {
                u32* temp_indices = (u32*)malloc(sizeof(u32) * index_count);
                cgltf_accessor_unpack_indices(prim->indices, temp_indices, sizeof(u32),
                                              index_count);
                for (cgltf_size i = 0; i < index_count; ++i)
                    arrput(indices, temp_indices[i]);  // ?¤í”„???ìš©
                free(temp_indices);
            }

            // MeshRange ê¸°ë¡
            MeshRange range = {};
            range.vertex_offset = vertex_offset;
            range.vertex_count = (u32)vertex_count;
            range.index_offset = index_offset;
            range.index_count = (u32)index_count;
            range.material_index = prim->material ? (u32)(prim->material - data->materials) : 0;
            arrput(ranges, range);

            vertex_offset += (u32)vertex_count;
            index_offset += (u32)index_count;
        }
        mesh_first_range[mesh_idx] = range_start;
        mesh_range_count[mesh_idx] = (u32)arrlen(ranges) - range_start;
    }

    MeshInstance* instances = NULL;
    if (data->scene && data->scene->nodes_count > 0)
    {
        for (cgltf_size i = 0; i < data->scene->nodes_count; ++i)
        {
            gather_instances_from_node(data, data->scene->nodes[i], glm::mat4(1.0f),
                                       mesh_first_range, mesh_range_count, &instances);
        }
    }
    else
    {
        for (cgltf_size i = 0; i < data->nodes_count; ++i)
        {
            gather_instances_from_node(data, &data->nodes[i], glm::mat4(1.0f),
                                       mesh_first_range, mesh_range_count, &instances);
        }
    }

    MaterialInfo* materials = NULL;
    u32 material_count = (u32)data->materials_count;
    if (material_count > 0)
    {
        materials = (MaterialInfo*)malloc(sizeof(MaterialInfo) * material_count);
        for (u32 i = 0; i < material_count; ++i)
        {
            const u32 invalid_tex_index = 255;
            u32 base_color = invalid_tex_index;
            u32 normal = invalid_tex_index;
            u32 metallic_roughness = invalid_tex_index;
            u32 occlusion = invalid_tex_index;

            const cgltf_material* material = &data->materials[i];
            if (material->has_pbr_metallic_roughness)
            {
                const cgltf_texture* tex =
                    material->pbr_metallic_roughness.base_color_texture.texture;
                if (tex && tex->image)
                {
                    base_color = (u32)(tex->image - data->images);
                }

                tex = material->pbr_metallic_roughness.metallic_roughness_texture.texture;
                if (tex && tex->image)
                {
                    metallic_roughness = (u32)(tex->image - data->images);
                }
            }

            if (material->normal_texture.texture && material->normal_texture.texture->image)
            {
                normal = (u32)(material->normal_texture.texture->image - data->images);
            }

            if (material->occlusion_texture.texture && material->occlusion_texture.texture->image)
            {
                occlusion = (u32)(material->occlusion_texture.texture->image - data->images);
            }

            materials[i].base_color_texture_index = base_color;
            materials[i].normal_texture_index = normal;
            materials[i].metallic_roughness_texture_index = metallic_roughness;
            materials[i].occlusion_texture_index = occlusion;
        }
    }
    else
    {
        material_count = 1;
        materials = (MaterialInfo*)malloc(sizeof(MaterialInfo) * material_count);
        materials[0].base_color_texture_index = 255;
        materials[0].normal_texture_index = 255;
        materials[0].metallic_roughness_texture_index = 255;
        materials[0].occlusion_texture_index = 255;
    }

    free(mesh_first_range);
    free(mesh_range_count);

    mesh.positions = positions;
    mesh.normals = normals;
    mesh.uvs = uvs;
    mesh.indices = indices;
    mesh.ranges = ranges;
    mesh.instances = instances;
    mesh.materials = materials;
    mesh.material_count = material_count;

    Mesh* meshes = NULL;
    arrput(meshes, mesh);
    *out_meshes = meshes;

    return true;
}

b8 load_raw_images_data_from_gltf(cgltf_data* data, const char* gltf_filename,
                                  RawImageData** out_images)
{
    if (!data || !out_images)
        return false;

    RawImageData* img_data_arr = NULL;

    for (cgltf_size i = 0; i < data->images_count; ++i)
    {
        const cgltf_image* image = &data->images[i];
        RawImageData img_data = {};

        // Load image data from buffer_view or uri
        if (image->buffer_view)
        {
            const cgltf_buffer_view* bv = image->buffer_view;
            img_data.size = (u32)bv->size;
            img_data.pixels = malloc(img_data.size);
            memcpy(img_data.pixels, (u8*)bv->buffer->data + bv->offset, img_data.size);
            stbi_info_from_memory((stbi_uc*)img_data.pixels, img_data.size, (int*)&img_data.width,
                                  (int*)&img_data.height, (int*)&img_data.channels);
        }
        else if (image->uri)
        {
            char image_path[512];
            printf("Loading image file %s\n", image->uri);
            join_gltf_image_path(gltf_filename, image->uri, image_path, sizeof(image_path));
            img_data.pixels = stbi_load(image_path, (int*)&img_data.width, (int*)&img_data.height,
                                        (int*)&img_data.channels, STBI_rgb_alpha);
            img_data.channels = CHANNELS_RGBA;
            if (!img_data.pixels)
            {
                printf("stbi_load failed: %s\n", stbi_failure_reason());
            }

            img_data.size = img_data.width * img_data.height * CHANNELS_RGBA;
        }
        else
        {
            continue;
        }

        arrput(img_data_arr, img_data);
    }

    *out_images = img_data_arr;
    return true;
}

cgltf_data* load_cgltf_data(const char* filename)
{
    cgltf_options options = {};
    cgltf_data* data = NULL;
    if (cgltf_parse_file(&options, filename, &data) != cgltf_result_success)
        return NULL;
    if (cgltf_load_buffers(&options, data, filename) != cgltf_result_success)
    {
        cgltf_free(data);
        return NULL;
    }
    return data;
}

b8 load_gltf_from_file(RenderContext* context, const char* filename, Mesh** out_meshes,
                       Texture** out_textures)
{
    assert(context && filename && out_meshes && out_textures);

    cgltf_data* data = load_cgltf_data(filename);
    if (!data)
    {
        std::cerr << "Failed to load glTF file: " << filename << std::endl;
        return false;
    }

    Mesh* Meshes = NULL;

    if (!load_meshes_from_gltf(data, &Meshes))
    {
        std::cerr << "Failed to load meshes from glTF data." << std::endl;
        cgltf_free(data);
        return false;
    }

    *out_meshes = Meshes;

    RawImageData* raw_images = NULL;

    if (!load_raw_images_data_from_gltf(data, filename, &raw_images))
    {
        std::cerr << "Failed to load images from glTF data." << std::endl;
        cgltf_free(data);
        return false;
    }

    // Debug: report missing textures by material
    if (data->materials_count > 0)
    {
        auto get_image_label = [](const cgltf_image* image) -> const char*
        {
            if (!image)
                return "<null image>";
            if (image->uri)
                return image->uri;
            if (image->buffer_view)
                return "<buffer_view>";
            return "<no uri>";
        };

        u32 missing_any = 0;
        for (u32 i = 0; i < (u32)data->materials_count; ++i)
        {
            const cgltf_material* material = &data->materials[i];
            const char* mat_name = material->name ? material->name : "<unnamed>";

            const cgltf_texture* base_color_tex = nullptr;
            const cgltf_texture* normal_tex = nullptr;
            const cgltf_texture* metal_rough_tex = nullptr;
            const cgltf_texture* occlusion_tex = nullptr;

            if (material->has_pbr_metallic_roughness)
            {
                base_color_tex =
                    material->pbr_metallic_roughness.base_color_texture.texture;
                metal_rough_tex =
                    material->pbr_metallic_roughness.metallic_roughness_texture.texture;
            }
            if (material->normal_texture.texture)
                normal_tex = material->normal_texture.texture;
            if (material->occlusion_texture.texture)
                occlusion_tex = material->occlusion_texture.texture;

            const cgltf_image* base_color_img =
                base_color_tex ? base_color_tex->image : nullptr;
            const cgltf_image* normal_img = normal_tex ? normal_tex->image : nullptr;
            const cgltf_image* metal_rough_img =
                metal_rough_tex ? metal_rough_tex->image : nullptr;
            const cgltf_image* occlusion_img =
                occlusion_tex ? occlusion_tex->image : nullptr;

            const bool missing_base = (base_color_tex == nullptr || base_color_img == nullptr);
            const bool missing_normal = (normal_tex == nullptr || normal_img == nullptr);
            const bool missing_metal_rough =
                (metal_rough_tex == nullptr || metal_rough_img == nullptr);
            const bool missing_occlusion =
                (occlusion_tex == nullptr || occlusion_img == nullptr);

            if (missing_base || missing_normal || missing_metal_rough || missing_occlusion)
            {
                ++missing_any;
                std::cout << "[gltf] material[" << i << "] " << mat_name << " missing:";
                if (missing_base)
                    std::cout << " baseColor";
                if (missing_normal)
                    std::cout << " normal";
                if (missing_metal_rough)
                    std::cout << " metalRough";
                if (missing_occlusion)
                    std::cout << " occlusion";
                std::cout << std::endl;
            }
            else
            {
                // Uncomment for verbose listing of all resolved textures.
                // std::cout << "[gltf] material[" << i << "] " << mat_name
                //           << " baseColor=" << get_image_label(base_color_img)
                //           << " normal=" << get_image_label(normal_img)
                //           << " metalRough=" << get_image_label(metal_rough_img)
                //           << " occlusion=" << get_image_label(occlusion_img) << std::endl;
            }
        }

        std::cout << "[gltf] materials with missing textures: " << missing_any << "/"
                  << data->materials_count << std::endl;
    }

    Texture* textures = NULL;
    u32 raw_image_length = arrlen(raw_images);
    for (u32 i = 0; i < raw_image_length; ++i)
    {
        Texture* texture = NULL;
        TextureDesc desc = {};
        desc.width = raw_images[i].width;
        desc.height = raw_images[i].height;
        desc.mip_levels = 1;
        desc.sample_count = 1;
        desc.vulkan_format = VK_FORMAT_R8G8B8A8_SRGB;
        desc.clear_value = {};
        desc.start_state = RESOURCE_STATE_SHADER_RESOURCE;
        desc.type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        desc.native_handle = NULL;

        vulkan_texture_create(context, &desc, &texture);

        Buffer staging_buffer;
        u32 staging_buffer_size = raw_images[i].width * raw_images[i].height * CHANNELS_RGBA;
        vulkan_buffer_create(context, staging_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                             VMA_MEMORY_USAGE_AUTO,
                             VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
                             &staging_buffer);
        vulkan_buffer_upload(context, &staging_buffer, raw_images[i].pixels, staging_buffer_size);

        Command one_time_submit;
        vulkan_command_pool_create(context, &one_time_submit, QUEUE_TYPE_TRANSFER);
        vulkan_command_buffer_allocate(context, &one_time_submit, true);
        vulkan_command_buffer_begin(&one_time_submit, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        TextureBarrier textureBarrier = {};
        textureBarrier.texture = texture;
        textureBarrier.current_state = RESOURCE_STATE_UNDEFINED;
        textureBarrier.new_state = RESOURCE_STATE_COPY_DEST;
        u32 textureBarrierCount = 1;

        vulkan_command_resource_barrier(&one_time_submit, NULL, 0, &textureBarrier,
                                        textureBarrierCount, NULL, 0);

        VkBufferImageCopy copy_region = {};
        copy_region.bufferOffset = 0;
        copy_region.bufferRowLength = raw_images[i].width;
        copy_region.bufferImageHeight = raw_images[i].height;
        copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.imageSubresource.mipLevel = 0;
        copy_region.imageSubresource.baseArrayLayer = 0;
        copy_region.imageSubresource.layerCount = 1;
        copy_region.imageExtent = {desc.width, desc.height, 1};

        vkCmdCopyBufferToImage(one_time_submit.buffer, staging_buffer.handle, texture->image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);

        textureBarrier.current_state = RESOURCE_STATE_COPY_DEST;
        textureBarrier.new_state = RESOURCE_STATE_SHADER_RESOURCE;
        vulkan_command_resource_barrier(&one_time_submit, NULL, 0, &textureBarrier,
                                        textureBarrierCount, NULL, 0);

        vulkan_command_buffer_end(&one_time_submit);

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &one_time_submit.buffer;

        VK_CHECK(
            vkQueueSubmit(context->device_context.transfer_queue, 1, &submit_info, VK_NULL_HANDLE));
        vkQueueWaitIdle(context->device_context.transfer_queue);
        vulkan_command_pool_destroy(context, &one_time_submit);

        vulkan_buffer_destroy(context, &staging_buffer);

        // stb_ds ë°°ì—´??ì¶”ê?
        arrput(textures, *texture);

        free(raw_images[i].pixels);  // Free raw pixel data after creating texture
    }

    *out_textures = textures;
    arrfree(raw_images);
}

b8 create_deinterleaved_mesh_buffers(RenderContext* context, const Mesh* mesh,
                                     Buffer* out_positions, Buffer* out_normals, Buffer* out_uvs,
                                     Buffer* out_indices, u32* out_vertex_count,
                                     u32* out_index_count)
{
    if (!context || !mesh || !out_positions || !out_normals || !out_uvs || !out_indices)
        return false;

    // Compute counts from stb_ds arrays
    const u32 pos_floats = mesh->positions ? (u32)arrlen(mesh->positions) : 0;
    const u32 nrm_floats = mesh->normals ? (u32)arrlen(mesh->normals) : 0;
    const u32 uv_floats = mesh->uvs ? (u32)arrlen(mesh->uvs) : 0;
    const u32 idx_count = mesh->indices ? (u32)arrlen(mesh->indices) : 0;

    // Positions are 3 floats per vertex
    if (pos_floats == 0 || pos_floats % 3 != 0)
        return false;
    const u32 vertex_count = pos_floats / 3;

    // Optional sanity checks (normals/uvs arrays should match if present)
    if (nrm_floats && (nrm_floats / 3) != vertex_count)
        return false;
    if (uv_floats && (uv_floats / 2) != vertex_count)
        return false;

    // Upload de-interleaved buffers
    const u64 positions_size = (u64)pos_floats * sizeof(f32);
    create_device_local_buffer_from_data(context, mesh->positions, positions_size,
                                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, out_positions);

    const u64 normals_size = (u64)nrm_floats * sizeof(f32);
    if (nrm_floats)
    {
        create_device_local_buffer_from_data(context, mesh->normals, normals_size,
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, out_normals);
    }
    else
    {
        out_normals->handle = VK_NULL_HANDLE;
        out_normals->allocation = VK_NULL_HANDLE;
    }

    const u64 uvs_size = (u64)uv_floats * sizeof(f32);
    if (uv_floats)
    {
        create_device_local_buffer_from_data(context, mesh->uvs, uvs_size,
                                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, out_uvs);
    }
    else
    {
        out_uvs->handle = VK_NULL_HANDLE;
        out_uvs->allocation = VK_NULL_HANDLE;
    }

    const u64 indices_size = (u64)idx_count * sizeof(u32);
    if (idx_count)
    {
        create_device_local_buffer_from_data(context, mesh->indices, indices_size,
                                             VK_BUFFER_USAGE_INDEX_BUFFER_BIT, out_indices);
    }
    else
    {
        out_indices->handle = VK_NULL_HANDLE;
        out_indices->allocation = VK_NULL_HANDLE;
    }

    if (out_vertex_count)
        *out_vertex_count = vertex_count;
    if (out_index_count)
        *out_index_count = idx_count;
    return true;
}

void free_mesh(Mesh* mesh, u32 mesh_count)
{
    if (!mesh)
        return;

    for (u32 i = 0; i < mesh_count; ++i)
    {
        arrfree(mesh[i].positions);
        arrfree(mesh[i].normals);
        arrfree(mesh[i].uvs);
        arrfree(mesh[i].indices);
        arrfree(mesh[i].ranges);
        free(mesh[i].materials);
    }
    arrfree(mesh);
}

