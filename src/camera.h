#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "graphics/shader.h"
#include "window.h"

class Camera
{

public:
    glm::vec3 position;
    glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    bool firstMouseClick = true;

    int viewportWidth;
    int viewportHeight;

    float speed = 1.0f;
    float walkSpeed = 1.5f;
    float sprintSpeed = 5.0f;
    float sensitivity = 100.0f;

    Camera(int width, int height, glm::vec3 position);

    void setViewProjection(float fovDegrees, float nearPlane, float farPlane, Shader &shader, const char *uniformName);
    void processInput(Window &window, float deltaTime);
    void setViewportSize(int width, int height);
};
#endif