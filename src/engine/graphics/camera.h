#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "shader.h"
#include "engine/platform/window.h"

class Camera
{

public:
    // Posição, direção para onde a câmera olha e eixo vertical da câmera.
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
    float zoomSpeed = 0.5f;

    // Distâncias mínima e máxima visíveis pela câmera.
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    Camera(int width, int height, glm::vec3 position);

    // Matriz de visão: posiciona o mundo em relação à câmera.
    glm::mat4 viewMatrix() const;

    // Matriz de projeção em perspectiva; usa o tamanho do viewport e os planos near e far da câmera.
    glm::mat4 projectionMatrix(float fovDegrees) const;

    // Projeção vezes visão: leva um ponto do mundo para o espaço de recorte.
    glm::mat4 viewProjectionMatrix(float fovDegrees) const;

    // Envia ao shader as matrizes de visão e projeção da câmera.
    // Com ignoreTranslation, a posição da câmera é ignorada (usado no céu).
    void setViewProjection(float fovDegrees, Shader &shader, const char *uniformName,
                           bool ignoreTranslation = false);

    // Atualiza posição e orientação a partir do teclado e do mouse.
    void processInput(Window &window, float deltaTime, int rotateButton = GLFW_MOUSE_BUTTON_RIGHT);

    // Atualiza a área usada para calcular a projeção.
    void setViewportSize(int width, int height);
};
#endif