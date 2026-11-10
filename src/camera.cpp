
#include "camera.h"
#include <algorithm>

Camera::Camera(int width, int height, glm::vec3 position)
{
    viewportWidth = width;
    viewportHeight = height;
    this->position = position;
}

void Camera::setViewProjection(float fovDegrees, float nearPlane, float farPlane, Shader &shader, const char *uniformName, bool ignoreTranslation)
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    view = glm::lookAt(position, position + orientation, up);

        // Sem a translação, só a rotação da câmera vale: o céu parece infinitamente distante.
    if (ignoreTranslation)
        view = glm::mat4(glm::mat3(view));

    projection = glm::perspective(
        glm::radians(fovDegrees),
        static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight),
        nearPlane,
        farPlane);

    glUniformMatrix4fv(
        glGetUniformLocation(shader.ID, uniformName),
        1,
        GL_FALSE,
        glm::value_ptr(projection * view));
}

void Camera::processInput(Window &window, float deltaTime, int rotateButton)
{
    // WASD move no plano horizontal; Espaço e Ctrl movem no eixo vertical.
    if (window.keyDown(GLFW_KEY_LEFT_SHIFT))
        speed = sprintSpeed;
    else
        speed = walkSpeed;

    if (window.keyDown(GLFW_KEY_W))
        position += speed * deltaTime * orientation;

    if (window.keyDown(GLFW_KEY_A))
        position += speed * deltaTime * -glm::normalize(glm::cross(orientation, up));

    if (window.keyDown(GLFW_KEY_S))
        position += speed * deltaTime * -orientation;

    if (window.keyDown(GLFW_KEY_D))
        position += speed * deltaTime * glm::normalize(glm::cross(orientation, up));

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

    // Enquanto o botão esquerdo estiver pressionado, o mouse controla a orientação.
    if (window.mouseDown(GLFW_MOUSE_BUTTON_RIGHT))
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

// Mantém as dimensões usadas na projeção; ignora valores inválidos durante a minimização.
void Camera::setViewportSize(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    viewportWidth = width;
    viewportHeight = height;
}
