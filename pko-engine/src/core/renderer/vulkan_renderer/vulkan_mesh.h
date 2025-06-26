#ifndef VULKAN_MESH_H
#define VULKAN_MESH_H

#include "vulkan_types.inl"

#include "vulkan_image.h"

#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <memory>
#include <vector>
#include <string>

struct model_constant
{
    glm::mat4 model;
    glm::mat3 normal_matrix;
    glm::vec3 padding;
};

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    std::vector<Texture> textures;
    glm::mat4 transform_matrix;
};

class vulkan_render_object
{
   public:
    vulkan_render_object(VulkanContext* pContext, const char* path);
    void upload_mesh();
    void vulkan_render_object_destroy();
    ~vulkan_render_object();

    void load_model(std::string path);

    std::vector<Mesh> meshes;

    std::vector<Buffer> vertex_buffers;
    std::vector<Buffer> index_buffers;

    glm::mat4 get_transform_matrix() const;
    void rotate(float degree, glm::vec3 axis);
    void draw(VkCommandBuffer command_buffer);

    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation;

   private:
    void process_node(aiNode* node, const aiScene* scene);
    Mesh process_mesh(aiMesh* mesh, const aiScene* scene);
    std::vector<Texture> load_material_textures(aiMaterial* mat, aiTextureType type,
                                                std::string typeName);

    VulkanContext* pContext;
};

#endif  // !VULKAN_MESH_H