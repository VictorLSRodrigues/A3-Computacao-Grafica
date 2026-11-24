
#include "camera.h"
#include <algorithm>
#include <cmath>

namespace
{
void smoothDamp(
    glm::vec3& current,
    glm::vec3& currentVelocity,
    const glm::vec3& target,
    float smoothTime,
    float deltaTime)
{
    if (deltaTime <= 0.0f)
        return;

    const float omega = 2.0f / std::max(0.001f, smoothTime);
    const float x = omega * deltaTime;
    const float decay = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    const glm::vec3 change = current - target;
    const glm::vec3 temporary = (currentVelocity + omega * change) * deltaTime;
    currentVelocity = (currentVelocity - omega * temporary) * decay;
    current = target + (change + temporary) * decay;
}

void smoothDamp(
    float& current,
    float& currentVelocity,
    float target,
    float smoothTime,
    float deltaTime)
{
    if (deltaTime <= 0.0f)
        return;

    const float omega = 2.0f / std::max(0.001f, smoothTime);
    const float x = omega * deltaTime;
    const float decay = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    const float change = current - target;
    const float temporary = (currentVelocity + omega * change) * deltaTime;
    currentVelocity = (currentVelocity - omega * temporary) * decay;
    current = target + (change + temporary) * decay;
}
}

Camera::Camera(int width, int height, glm::vec3 position)
{
    viewportWidth = width;
    viewportHeight = height;
    this->position = position;
}

void Camera::resetSettings()
{
    fov = kDefaultFov;
    walkSpeed = kDefaultWalkSpeed;
    sprintSpeed = kDefaultSprintSpeed;
    sensitivity = kDefaultSensitivity;
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(position, position + orientation, up);
}

glm::mat4 Camera::projectionMatrix() const
{
    return glm::perspective(
        glm::radians(fov),
        static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
        nearPlane,
        farPlane);
}

glm::mat4 Camera::viewProjectionMatrix() const
{
    return projectionMatrix() * viewMatrix();
}

void Camera::setViewProjection(Shader &shader, const char *uniformName, bool ignoreTranslation)
{
    glm::mat4 view = viewMatrix();

    // Sem a translação, só a rotação da câmera vale: o céu parece infinitamente distante.
    if (ignoreTranslation)
        view = glm::mat4(glm::mat3(view));

    const glm::mat4 projection = projectionMatrix();

    glUniformMatrix4fv(
        glGetUniformLocation(shader.ID, uniformName),
        1,
        GL_FALSE,
        glm::value_ptr(projection * view));
}

void Camera::processInput(
    Window &window,
    float deltaTime,
    int rotateButton,
    bool allowPlanarMovement)
{
    // WASD move no plano horizontal; podem ser reservadas para um corpo selecionado.
    // Espaço e Ctrl continuam movendo a câmera no eixo vertical.
    if (window.keyDown(GLFW_KEY_LEFT_SHIFT))
        speed = sprintSpeed;
    else
        speed = walkSpeed;

    if (allowPlanarMovement)
    {
        if (window.keyDown(GLFW_KEY_W))
            position += speed * deltaTime * orientation;

        if (window.keyDown(GLFW_KEY_A))
            position += speed * deltaTime * -glm::normalize(glm::cross(orientation, up));

        if (window.keyDown(GLFW_KEY_S))
            position += speed * deltaTime * -orientation;

        if (window.keyDown(GLFW_KEY_D))
            position += speed * deltaTime * glm::normalize(glm::cross(orientation, up));
    }

    if (window.keyDown(GLFW_KEY_SPACE))
        position += speed * deltaTime * up;

    if (window.keyDown(GLFW_KEY_LEFT_CONTROL))
        position += speed * deltaTime * -up;
    
       // Scroll aproxima ou afasta a câmera na direção em que ela está olhando.
    const float scrollOffset = static_cast<float>(window.getScrollY());

    if (scrollOffset != 0.0f)
    {
        position += glm::normalize(orientation) * scrollOffset * zoomSpeed;
    }

    // Enquanto o botão de rotação estiver pressionado, o mouse controla a orientação.
    if (window.mouseDown(rotateButton))
    {
        window.setCursorMode(Window::CursorMode::Hidden);

        if (firstMouseClick)
        {
            window.setMousePosition(viewportWidth / 2, viewportHeight / 2);
            firstMouseClick = false;
        }

        double mouseX = window.getMouseX();
        double mouseY = window.getMouseY();

        float rotationX = sensitivity * static_cast<float>(mouseY - (viewportHeight / 2)) / viewportHeight;
        float rotationY = sensitivity * static_cast<float>(mouseX - (viewportWidth / 2)) / viewportWidth;

        glm::vec3 newOrientation = glm::rotate(
            orientation,
            glm::radians(-rotationX),
            glm::normalize(glm::cross(orientation, up)));

        if (!(glm::angle(newOrientation, up) <= glm::radians(5.0f) ||
              glm::angle(newOrientation, -up) <= glm::radians(5.0f)))
        {
            orientation = newOrientation;
        }

        orientation = glm::rotate(
            orientation,
            glm::radians(-rotationY),
            up);

        window.setMousePosition(viewportWidth / 2, viewportHeight / 2);
    }
    else
    {
        window.setCursorMode(Window::CursorMode::Normal);
        firstMouseClick = true;
    }
    
}

void Camera::beginFollow(const glm::vec3& target)
{
    if (followActive_)
        return;

    followFocus_ = target + glm::vec3(0.0f, 0.25f, 0.0f);
    followFocusVelocity_ = glm::vec3(0.0f);
    followPositionVelocity_ = glm::vec3(0.0f);

    glm::vec3 offset = position - followFocus_;
    float distance = glm::length(offset);
    if (distance <= 1e-4f)
    {
        offset = glm::vec3(0.0f, 5.0f, 10.0f);
        distance = glm::length(offset);
    }

    followDistance_ = std::clamp(distance, 6.0f, 16.0f);
    followTargetDistance_ = followDistance_;
    followDistanceVelocity_ = 0.0f;
    followYaw_ = std::atan2(offset.x, offset.z);
    followPitch_ = std::clamp(
        std::asin(std::clamp(offset.y / distance, -1.0f, 1.0f)),
        glm::radians(10.0f),
        glm::radians(65.0f));
    followActive_ = true;
    firstMouseClick = true;
}

void Camera::processFollowInput(
    Window& window,
    const glm::vec3& target,
    float deltaTime,
    int rotateButton)
{
    processFollowInput(
        window, target, glm::vec3(0.0f), deltaTime, rotateButton);
}

void Camera::processFollowInput(
    Window& window,
    const glm::vec3& target,
    const glm::vec3& targetVelocity,
    float deltaTime,
    int rotateButton)
{
    if (!followActive_)
        beginFollow(target);

    if (window.mouseDown(rotateButton))
    {
        window.setCursorMode(Window::CursorMode::Hidden);

        if (firstMouseClick)
        {
            window.setMousePosition(viewportWidth / 2, viewportHeight / 2);
            firstMouseClick = false;
        }
        else
        {
            const double mouseX = window.getMouseX();
            const double mouseY = window.getMouseY();
            const float rotationX = sensitivity *
                static_cast<float>(mouseY - viewportHeight / 2.0) /
                static_cast<float>(viewportHeight);
            const float rotationY = sensitivity *
                static_cast<float>(mouseX - viewportWidth / 2.0) /
                static_cast<float>(viewportWidth);

            followPitch_ = std::clamp(
                followPitch_ - glm::radians(rotationX),
                glm::radians(10.0f),
                glm::radians(65.0f));
            followYaw_ -= glm::radians(rotationY);
            window.setMousePosition(viewportWidth / 2, viewportHeight / 2);
        }
    }
    else
    {
        window.setCursorMode(Window::CursorMode::Normal);
        firstMouseClick = true;
    }

    const float scrollOffset = static_cast<float>(window.getScrollY());
    if (scrollOffset != 0.0f)
        followTargetDistance_ = std::clamp(
            followTargetDistance_ - scrollOffset * zoomSpeed * 2.0f,
            4.5f,
            24.0f);

    const float safeDelta = std::clamp(deltaTime, 0.0f, 0.1f);

    // A câmera olha um pouco à frente do movimento, evitando manter o alvo
    // sempre no centro quando ele está cruzando a bacia em velocidade.
    glm::vec3 horizontalVelocity(targetVelocity.x, 0.0f, targetVelocity.z);
    const float targetSpeed = glm::length(horizontalVelocity);
    glm::vec3 lookAhead(0.0f);
    if (targetSpeed > 0.15f)
    {
        const float lookAheadDistance = std::clamp(targetSpeed * 0.16f, 0.0f, 2.0f);
        lookAhead = horizontalVelocity * (lookAheadDistance / targetSpeed);
    }

    const glm::vec3 desiredFocus = target + lookAhead + glm::vec3(0.0f, 0.25f, 0.0f);
    smoothDamp(followFocus_, followFocusVelocity_, desiredFocus, 0.18f, safeDelta);
    smoothDamp(followDistance_, followDistanceVelocity_,
               followTargetDistance_, 0.16f, safeDelta);

    const float cosPitch = std::cos(followPitch_);
    const glm::vec3 orbitDirection(
        std::sin(followYaw_) * cosPitch,
        std::sin(followPitch_),
        std::cos(followYaw_) * cosPitch);
    const glm::vec3 desiredPosition = followFocus_ + orbitDirection * followDistance_;

    // Amortecimento crítico: acompanha acelerações e mudanças de direção sem
    // dar trancos, sem oscilar e sem alterar a simulação física.
    smoothDamp(position, followPositionVelocity_, desiredPosition, 0.22f, safeDelta);
    const glm::vec3 viewDirection = followFocus_ - position;
    if (glm::length(viewDirection) > 1e-5f)
        orientation = glm::normalize(viewDirection);
    up = glm::vec3(0.0f, 1.0f, 0.0f);
}

void Camera::endFollow()
{
    followActive_ = false;
    firstMouseClick = true;
}

// Mantém as dimensões usadas na projeção; ignora valores inválidos durante a minimização.
void Camera::setViewportSize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    viewportWidth = width;
    viewportHeight = height;
}
