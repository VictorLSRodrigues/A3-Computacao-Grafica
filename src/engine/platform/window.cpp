#include "window.h"
#include <algorithm>
#include <iterator>

#include <iostream>

void Window::onFramebufferResize(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);

    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (self)
    {
        self->width = width;
        self->height = height;
    }
}

void Window::onKey(GLFWwindow *window, int key, int /*scancode*/, int action, int /*mods*/)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self || key < 0 || key > GLFW_KEY_LAST)
        return;

    if (action == GLFW_PRESS)
    {
        self->keyStates[key] = true;
        self->keysPressedThisFrame[key] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        self->keyStates[key] = false;
    }
}

void Window::onMouseMove(GLFWwindow *window, double x, double y)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (self)
    {
        self->mouseX = x;
        self->mouseY = y;
    }
}

void Window::onScroll(GLFWwindow *window, double /*xOffset*/, double yOffset)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));

    if (self)
        self->scrollY += yOffset;
}

void Window::onMouseButton(GLFWwindow *window, int button, int action, int /*mods*/)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self || button < 0 || button > GLFW_MOUSE_BUTTON_LAST)
        return;

    if (action == GLFW_PRESS)
    {
        self->mouseButtons[button] = true;
        self->mouseClicksThisFrame[button] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        self->mouseButtons[button] = false;
    }
}

bool Window::create(const char *title, bool fullscreen, int windowWidth, int windowHeight)
{
    glfwSetErrorCallback([](int errorCode, const char *description)
                         { std::cout << "Erro do GLFW (" << errorCode << "): " << description << std::endl; });

    if (!glfwInit())
    {
        std::cout << "Erro ao inicializar o GLFW!" << std::endl;
        return false;
    }
    glfwInitialized = true;

    // Solicita OpenGL 3.3 no perfil Core.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4); // Ativa antisserrilhamento com quatro amostras.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Em tela cheia, associa a janela ao monitor principal.
    GLFWmonitor *monitor = nullptr;
    if (fullscreen)
    {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        windowWidth = mode->width;
        windowHeight = mode->height;
    }

    windowHandle = glfwCreateWindow(windowWidth, windowHeight, title, monitor, nullptr);
    if (!windowHandle)
    {
        std::cout << "Erro ao criar a janela!" << std::endl;
        destroy();
        return false;
    }

    glfwMakeContextCurrent(windowHandle);

    glfwSwapInterval(1); // Sincroniza a troca de quadros com a tela.

    if (!gladLoadGL())
    {
        std::cout << "Erro ao inicializar o GLAD!" << std::endl;
        destroy();
        return false;
    }

    glfwGetFramebufferSize(windowHandle, &width, &height);

    // Permite que os callbacks encontrem esta instância e atualizem seus dados.
    glfwSetWindowUserPointer(windowHandle, this);
    glfwSetFramebufferSizeCallback(windowHandle, onFramebufferResize);

    glfwSetKeyCallback(windowHandle, onKey);
    glfwSetCursorPosCallback(windowHandle, onMouseMove);
    glfwSetMouseButtonCallback(windowHandle, onMouseButton);
    glfwSetScrollCallback(windowHandle, onScroll);
    glfwGetCursorPos(windowHandle, &mouseX, &mouseY);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    return true;
}

bool Window::shouldClose() const { return glfwWindowShouldClose(windowHandle); }
void Window::close() const { glfwSetWindowShouldClose(windowHandle, GLFW_TRUE); }
void Window::swapBuffers() const { glfwSwapBuffers(windowHandle); }

void Window::pollEvents()
{
    // Limpa os eventos de um quadro e registra os novos recebidos do GLFW.
    std::fill(std::begin(keysPressedThisFrame), std::end(keysPressedThisFrame), false);
    std::fill(std::begin(mouseClicksThisFrame), std::end(mouseClicksThisFrame), false);

    scrollY = 0.0;

    glfwPollEvents();
}

void Window::destroy()
{
    if (windowHandle)
    {
        glfwDestroyWindow(windowHandle);
        windowHandle = nullptr;
    }
    if (glfwInitialized)
    {
        glfwTerminate();
        glfwInitialized = false;
    }
}

void Window::setCursorMode(CursorMode mode)
{
    int value = GLFW_CURSOR_NORMAL;
    if (mode == CursorMode::Hidden)
        value = GLFW_CURSOR_HIDDEN;
    else if (mode == CursorMode::Disabled)
        value = GLFW_CURSOR_DISABLED;

    glfwSetInputMode(windowHandle, GLFW_CURSOR, value);
}

void Window::setMousePosition(double x, double y)
{
    glfwSetCursorPos(windowHandle, x, y);
    mouseX = x; // Atualiza o estado local sem aguardar outro callback.
    mouseY = y;
}