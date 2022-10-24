#ifndef CAMERA_H
#define CAMERA_H

#include "defines.h"

#include "core/event.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <iostream>

enum class camera_movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;

struct camera {
	glm::vec3 pos;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 world_up;

	f32 yaw;
	f32 pitch;

	f32 movement_speed;
	f32 mouse_sensitivity;
	f32 zoom;

    void init(glm::vec3 position = glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));
    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 get_view_matrix();

	// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void process_keyboard(camera_movement direction, f32 deltaTime)
    {
        f32 velocity = movement_speed * deltaTime;
        if (direction == camera_movement::FORWARD)
            pos += front * velocity;
        if (direction == camera_movement::BACKWARD)
            pos -= front * velocity;
        if (direction == camera_movement::LEFT)
            pos -= right * velocity;
        if (direction == camera_movement::RIGHT)
            pos += right * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void process_mouse_movement(f32 xoffset, f32 yoffset, b8 constrainPitch = true)
    {
        xoffset *= mouse_sensitivity;
        yoffset *= mouse_sensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        update_camera_vectors();
    }

    void process_keyboard_rotate(f32 xoffset, f32 yoffset)
    {
        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        update_camera_vectors();
    }

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void process_mouse_scroll(float yoffset)
    {
        zoom -= (float)yoffset;
        if (zoom < 1.0f)
            zoom = 1.0f;
        if (zoom > 45.0f)
            zoom = 45.0f;
    }

    // 0 < |zoom_speed| < 45
    void process_zoom_inout(float zoom_speed)
    {
        zoom += zoom_speed;
        if (zoom > 45.0f)
            zoom = 45.0f;
        if (zoom < 1.0f)
            zoom = 1.0f;
    }
	
private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void update_camera_vectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->front = glm::normalize(front);

        // also re-calculate the Right and Up vector
        right = glm::normalize(glm::cross(this->front, world_up));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        up = glm::normalize(glm::cross(right, this->front));
    }
};

#endif // !CAMERA_H
