#include "camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

void camera::init(glm::vec3 position, glm::vec3 up)
{
    front = glm::vec3(0.0f, 0.0f, -1.0f);
    movement_speed = SPEED;
    mouse_sensitivity = SENSITIVITY;
    zoom = ZOOM;
    pos = position;
    world_up = up;
    yaw = YAW;
    pitch = PITCH;
    update_camera_vectors();
}

glm::mat4 camera::get_view_matrix()
{
	return glm::lookAt(pos, pos + front, up);
}