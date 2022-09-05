#ifndef MATERIAL_H
#define MATERIAL_H

#include <glm/glm.hpp>

struct material {
	bool is_light;
	glm::vec3 diffuse;
	glm::vec3 specular;
	//glm::vec3 
	//TODO: PBR textures
};




#endif // MATERIAL_H
