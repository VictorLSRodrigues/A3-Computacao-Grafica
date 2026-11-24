#ifndef CAMERA_CLASS_H
#define CAMERA_CLASS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "engine/graphics/resources/shader.h"
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

    // Campo de visão vertical, em graus.
    float fov = kDefaultFov;

    float speed = 1.0f;
    float walkSpeed = kDefaultWalkSpeed;
    float sprintSpeed = kDefaultSprintSpeed;
    float sensitivity = kDefaultSensitivity;
    float zoomSpeed = 0.5f;

    // Distâncias mínima e máxima visíveis pela câmera.
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    Camera(int width, int height, glm::vec3 position);

    // Matriz de visão: posiciona o mundo em relação à câmera.
    glm::mat4 viewMatrix() const;

    // Matriz de projeção em perspectiva; usa o campo de visão, o tamanho do viewport e os planos near e far da câmera.
    glm::mat4 projectionMatrix() const;

    // Projeção vezes visão: leva um ponto do mundo para o espaço de recorte.
    glm::mat4 viewProjectionMatrix() const;

    // Envia ao shader as matrizes de visão e projeção da câmera.
    // Com ignoreTranslation, a posição da câmera é ignorada (usado no céu).
    void setViewProjection(Shader &shader, const char *uniformName,
                           bool ignoreTranslation = false);

    // Atualiza posição e orientação a partir do teclado e do mouse.
    // allowPlanarMovement=false reserva WASD para controlar um corpo da simulação.
    void processInput(
        Window &window,
        float deltaTime,
        int rotateButton = GLFW_MOUSE_BUTTON_RIGHT,
        bool allowPlanarMovement = true);

    // Liga a câmera a um alvo e permite orbitá-lo em terceira pessoa.
    // O alvo é atualizado continuamente por processFollowInput().
    void beginFollow(const glm::vec3 &target);
    void processFollowInput(
        Window &window,
        const glm::vec3 &target,
        float deltaTime,
        int rotateButton = GLFW_MOUSE_BUTTON_RIGHT);
    void processFollowInput(
        Window &window,
        const glm::vec3 &target,
        const glm::vec3 &targetVelocity,
        float deltaTime,
        int rotateButton = GLFW_MOUSE_BUTTON_RIGHT);
    void endFollow();
    bool isFollowing() const { return followActive_; }

    // Atualiza a área usada para calcular a projeção.
    void setViewportSize(int width, int height);

    // Volta o campo de visão, as velocidades e a sensibilidade aos valores padrão.
    void resetSettings();

private:
    bool followActive_ = false;
    float followYaw_ = 0.0f;
    float followPitch_ = glm::radians(28.0f);
    float followDistance_ = 12.0f;
    float followTargetDistance_ = 12.0f;
    float followDistanceVelocity_ = 0.0f;
    glm::vec3 followFocus_{0.0f};
    glm::vec3 followFocusVelocity_{0.0f};
    glm::vec3 followPositionVelocity_{0.0f};

    static constexpr float kDefaultFov = 45.0f;
    static constexpr float kDefaultWalkSpeed = 1.5f;
    static constexpr float kDefaultSprintSpeed = 5.0f;
    static constexpr float kDefaultSensitivity = 100.0f;
};
#endif