#ifndef OBJECT_H
#define OBJECT_H

#include <glm/glm.hpp>

#include <memory>
#include <vector>

struct vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 uv;
};

struct mesh {
	std::vector<vertex> vertices;
	glm::mat4 transform_matrix;
};

class object {
	
public:
	object() = delete;
	virtual ~object() = 0 {};
	object(const char* object_name);

	std::unique_ptr<mesh> p_mesh;

	glm::vec3 position;
	glm::vec3 scale;
	// rotation (quat)
};

#endif // !OBJECT_H
