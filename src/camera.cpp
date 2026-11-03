
#include "camera.h"

Camera::Camera(int width, int height, glm::vec3 position)
{
    Camera::width = width;
    Camera::height = height;
    Position = position;
}

void Camera::Matrix(float FOVdeg, float nearPlane, float farPlane, Shader &shader, const char *uniform)
{
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 projection = glm::mat4(1.0f);

    view = glm::lookAt(Position, Position + Orientation, Up);

    projection = glm::perspective(
        glm::radians(FOVdeg),
        static_cast<float>(width) / static_cast<float>(height),
        nearPlane,
        farPlane);

    glUniformMatrix4fv(
        glGetUniformLocation(shader.ID, uniform),
        1,
        GL_FALSE,
        glm::value_ptr(projection * view));
}

void Camera::Inputs(Window& window, float deltaTime)
{
    // Movimento da câmera
    if (window.teclaSegurada(GLFW_KEY_LEFT_SHIFT))
        speed = 5.0f;
    else
        speed = 1.5f;

    if (window.teclaSegurada(GLFW_KEY_W))
        Position += speed * deltaTime * Orientation;

    if (window.teclaSegurada(GLFW_KEY_A))
        Position += speed * deltaTime * -glm::normalize(glm::cross(Orientation, Up));

    if (window.teclaSegurada(GLFW_KEY_S))
        Position += speed * deltaTime * -Orientation;

    if (window.teclaSegurada(GLFW_KEY_D))
        Position += speed * deltaTime * glm::normalize(glm::cross(Orientation, Up));

    if (window.teclaSegurada(GLFW_KEY_SPACE))
        Position += speed * deltaTime * Up;

    if (window.teclaSegurada(GLFW_KEY_LEFT_CONTROL))
        Position += speed * deltaTime * -Up;

    // Controle do mouse
    if (window.mouseDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        window.setCursorMode(Window::CursorMode::Hidden);

        if (firstClick)
        {
            window.setMousePosition(width / 2, height / 2);
            firstClick = false;
        }

        double mouseX = window.getMouseX();
        double mouseY = window.getMouseY();

        float rotX = sensitivity * static_cast<float>(mouseY - (height / 2)) / height;
        float rotY = sensitivity * static_cast<float>(mouseX - (width / 2)) / width;

        glm::vec3 newOrientation = glm::rotate(
            Orientation,
            glm::radians(-rotX),
            glm::normalize(glm::cross(Orientation, Up)));

        if (!(glm::angle(newOrientation, Up) <= glm::radians(5.0f) ||
              glm::angle(newOrientation, -Up) <= glm::radians(5.0f)))
        {
            Orientation = newOrientation;
        }

        Orientation = glm::rotate(
            Orientation,
            glm::radians(-rotY),
            Up);

        window.setMousePosition(width / 2, height / 2);
    }
    else
    {
        window.setCursorMode(Window::CursorMode::Normal);
        firstClick = true;
    }
}

// Atualiza o tamanho usado na projeção. Ignora tamanhos inválidos
// (janela minimizada tem altura 0), mantendo o último tamanho bom.
void Camera::setSize(int newWidth, int newHeight)
{
    if (newWidth <= 0 || newHeight <= 0)
        return;

    width = newWidth;
    height = newHeight;
}
