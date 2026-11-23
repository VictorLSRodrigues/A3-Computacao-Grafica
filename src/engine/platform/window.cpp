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

void Window::onCharacter(GLFWwindow *window, unsigned int codepoint)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (self)
        self->charactersTypedThisFrame.push_back(codepoint);
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

void Window::onWindowPosition(GLFWwindow *window, int x, int y)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self || self->changingMode || self->currentMode != WindowMode::Windowed ||
        glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE)
        return;

    self->windowedX = x;
    self->windowedY = y;
}

void Window::onWindowSize(GLFWwindow *window, int width, int height)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self || self->changingMode || self->currentMode != WindowMode::Windowed ||
        glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE)
        return;

    self->windowedWidth = std::max(1, width);
    self->windowedHeight = std::max(1, height);
}

void Window::onWindowMaximize(GLFWwindow *window, int maximized)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (!self || self->changingMode)
        return;

    if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED) == GLFW_TRUE)
        self->currentMode = WindowMode::Maximized;
    else if (self->currentMode == WindowMode::Maximized)
        self->currentMode = WindowMode::Windowed;

    (void)maximized;
}

bool Window::create(const WindowConfig &windowConfig)
{
    if (windowHandle)
        destroy();

    config = windowConfig;
    currentMode = WindowMode::Windowed;
    windowedWidth = std::max(1, config.width);
    windowedHeight = std::max(1, config.height);

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
    glfwWindowHint(GLFW_SAMPLES, std::max(0, config.samples));
    glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);

    // A janela nasce em modo convencional para preservar suas dimensões
    // antes de aplicar Maximized, Borderless ou Fullscreen.
    windowHandle = glfwCreateWindow(windowedWidth, windowedHeight,
                                    config.title.c_str(), nullptr, nullptr);
    if (!windowHandle)
    {
        std::cout << "Erro ao criar a janela!" << std::endl;
        destroy();
        return false;
    }

    glfwMakeContextCurrent(windowHandle);
    glfwSwapInterval(config.vsync ? 1 : 0);

    if (!gladLoadGL())
    {
        std::cout << "Erro ao inicializar o GLAD!" << std::endl;
        destroy();
        return false;
    }

    // GLFW_SRGB_CAPABLE é uma window hint, não um atributo consultável por
    // glfwGetWindowAttrib. Verifica a codificação real do buffer traseiro
    // diretamente no framebuffer padrão.
    GLint framebufferColorEncoding = GL_LINEAR;
    glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER,
        GL_BACK_LEFT,
        GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING,
        &framebufferColorEncoding);

    if (framebufferColorEncoding == GL_SRGB)
    {
        // O OpenGL converte as saídas lineares para sRGB e faz blending
        // corretamente no framebuffer com codificação sRGB.
        glEnable(GL_FRAMEBUFFER_SRGB);
        std::cout << "Framebuffer sRGB: habilitado (GL_SRGB)." << std::endl;
    }
    else if (framebufferColorEncoding == GL_LINEAR)
    {
        glDisable(GL_FRAMEBUFFER_SRGB);
        std::cout << "Aviso: framebuffer usa codificação linear; GL_FRAMEBUFFER_SRGB desabilitado."
                  << std::endl;
    }
    else
    {
        glDisable(GL_FRAMEBUFFER_SRGB);
        std::cout << "Aviso: não foi possível identificar a codificação de cor do framebuffer ("
                  << framebufferColorEncoding << ")." << std::endl;
    }

    // Guarda o estado convencional para permitir retornar de outros modos.
    saveWindowedBounds();

    // Permite que os callbacks encontrem esta instância e atualizem seus dados.
    glfwSetWindowUserPointer(windowHandle, this);
    glfwSetFramebufferSizeCallback(windowHandle, onFramebufferResize);
    glfwSetKeyCallback(windowHandle, onKey);
    glfwSetCharCallback(windowHandle, onCharacter);
    glfwSetCursorPosCallback(windowHandle, onMouseMove);
    glfwSetMouseButtonCallback(windowHandle, onMouseButton);
    glfwSetScrollCallback(windowHandle, onScroll);
    glfwSetWindowPosCallback(windowHandle, onWindowPosition);
    glfwSetWindowSizeCallback(windowHandle, onWindowSize);
    glfwSetWindowMaximizeCallback(windowHandle, onWindowMaximize);
    glfwGetCursorPos(windowHandle, &mouseX, &mouseY);

    if (config.minWidth > 0 || config.minHeight > 0)
    {
        glfwSetWindowSizeLimits(windowHandle,
                                config.minWidth > 0 ? config.minWidth : GLFW_DONT_CARE,
                                config.minHeight > 0 ? config.minHeight : GLFW_DONT_CARE,
                                GLFW_DONT_CARE, GLFW_DONT_CARE);
    }

    updateFramebufferSize();

    if (config.mode != WindowMode::Windowed && !setMode(config.mode))
    {
        destroy();
        return false;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    return true;
}

void Window::saveWindowedBounds()
{
    if (!windowHandle)
        return;

    glfwGetWindowPos(windowHandle, &windowedX, &windowedY);
    glfwGetWindowSize(windowHandle, &windowedWidth, &windowedHeight);
    windowedWidth = std::max(1, windowedWidth);
    windowedHeight = std::max(1, windowedHeight);
}

void Window::updateFramebufferSize()
{
    if (!windowHandle)
        return;

    glfwGetFramebufferSize(windowHandle, &width, &height);
    glViewport(0, 0, width, height);
}

bool Window::setMode(WindowMode mode)
{
    if (!windowHandle)
        return false;

    if (mode == currentMode)
        return true;

    // Atualiza as dimensões restauráveis somente enquanto a janela está
    // no modo convencional. Maximizada, sem bordas ou tela cheia não devem
    // sobrescrever o último tamanho escolhido pelo usuário.
    if (currentMode == WindowMode::Windowed)
    {
        if (glfwGetWindowAttrib(windowHandle, GLFW_MAXIMIZED) == GLFW_FALSE)
            saveWindowedBounds();
        else
            currentMode = WindowMode::Maximized;
    }

    GLFWmonitor *primaryMonitor = nullptr;
    const GLFWvidmode *videoMode = nullptr;
    if (mode == WindowMode::Borderless || mode == WindowMode::Fullscreen)
    {
        primaryMonitor = glfwGetPrimaryMonitor();
        videoMode = primaryMonitor ? glfwGetVideoMode(primaryMonitor) : nullptr;
        if (!primaryMonitor || !videoMode)
        {
            std::cout << "Nao foi possivel obter o monitor para alterar o modo da janela!" << std::endl;
            return false;
        }
    }

    changingMode = true;
    switch (mode)
    {
    case WindowMode::Windowed:
        glfwSetWindowMonitor(windowHandle, nullptr,
                             windowedX, windowedY,
                             windowedWidth, windowedHeight,
                             GLFW_DONT_CARE);
        glfwSetWindowAttrib(windowHandle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowAttrib(windowHandle, GLFW_RESIZABLE,
                            config.resizable ? GLFW_TRUE : GLFW_FALSE);
        break;

    case WindowMode::Maximized:
        glfwSetWindowMonitor(windowHandle, nullptr,
                             windowedX, windowedY,
                             windowedWidth, windowedHeight,
                             GLFW_DONT_CARE);
        glfwSetWindowAttrib(windowHandle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowAttrib(windowHandle, GLFW_RESIZABLE,
                            config.resizable ? GLFW_TRUE : GLFW_FALSE);
        glfwMaximizeWindow(windowHandle);
        break;

    case WindowMode::Borderless:
    {
        // Primeiro retorna ao modo de janela; depois remove as decorações
        // e ocupa a área total do monitor sem trocar seu modo de vídeo.
        glfwSetWindowMonitor(windowHandle, nullptr,
                             windowedX, windowedY,
                             windowedWidth, windowedHeight,
                             GLFW_DONT_CARE);
        glfwSetWindowAttrib(windowHandle, GLFW_DECORATED, GLFW_FALSE);
        glfwSetWindowAttrib(windowHandle, GLFW_RESIZABLE, GLFW_FALSE);

        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(primaryMonitor, &monitorX, &monitorY);
        glfwSetWindowPos(windowHandle, monitorX, monitorY);
        glfwSetWindowSize(windowHandle, videoMode->width, videoMode->height);
        break;
    }

    case WindowMode::Fullscreen:
        glfwSetWindowAttrib(windowHandle, GLFW_DECORATED, GLFW_TRUE);
        glfwSetWindowMonitor(windowHandle, primaryMonitor, 0, 0,
                             videoMode->width, videoMode->height,
                             videoMode->refreshRate);
        break;
    }

    currentMode = mode;
    changingMode = false;
    updateFramebufferSize();
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
    charactersTypedThisFrame.clear();

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

    currentMode = WindowMode::Windowed;
    changingMode = false;
    width = 0;
    height = 0;
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