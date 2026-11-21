#pragma once

#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum class WindowMode
{
    Windowed,   // Janela convencional com bordas.
    Maximized,  // Janela convencional maximizada.
    Borderless, // Janela sem bordas ocupando o monitor, sem trocar o modo de vídeo.
    Fullscreen  // Janela associada ao monitor, utilizando seu modo de vídeo.
};

struct WindowConfig
{
    std::string title = "Rotor";
    WindowMode mode = WindowMode::Windowed;

    int width = 1280;
    int height = 720;
    int minWidth = 0;
    int minHeight = 0;
    int samples = 4;

    bool resizable = true;
    bool vsync = true;
};

class Window
{
private:
    GLFWwindow *windowHandle = nullptr;
    int width = 0;
    int height = 0;
    bool glfwInitialized = false;
    bool changingMode = false;
    WindowConfig config;
    WindowMode currentMode = WindowMode::Windowed;

    int windowedX = 100;
    int windowedY = 100;
    int windowedWidth = 1280;
    int windowedHeight = 720;

    // Estados contínuos e eventos de pressionamento de teclado e mouse.
    bool keyStates[GLFW_KEY_LAST + 1] = {};             // true enquanto a tecla estiver pressionada
    bool keysPressedThisFrame[GLFW_KEY_LAST + 1] = {};  // true no quadro em que a tecla foi pressionada
    bool mouseClicksThisFrame[GLFW_MOUSE_BUTTON_LAST + 1] = {};
    bool mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1] = {}; // true enquanto o botão estiver pressionado
    double mouseX = 0.0;
    double mouseY = 0.0;
    double scrollY = 0.0;

    static void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void onMouseMove(GLFWwindow *window, double x, double y);
    static void onMouseButton(GLFWwindow *window, int button, int action, int mods);
    static void onScroll(GLFWwindow *window, double xOffset, double yOffset);
    static void onWindowPosition(GLFWwindow *window, int x, int y);
    static void onWindowSize(GLFWwindow *window, int width, int height);
    static void onWindowMaximize(GLFWwindow *window, int maximized);

    // Atualiza o viewport e as dimensões ao redimensionar a janela.
    static void onFramebufferResize(GLFWwindow *window, int width, int height);

    void saveWindowedBounds();
    void updateFramebufferSize();

public:
    Window() = default;
    ~Window() { destroy(); }

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    bool create(const WindowConfig &config);

    // Altera o modo da janela sem recriar o contexto OpenGL.
    bool setMode(WindowMode mode);
    WindowMode getMode() const { return currentMode; }

    bool shouldClose() const;
    void close() const; // Solicita o fechamento da janela.
    void swapBuffers() const;
    void pollEvents();

    // keyPressed detecta o evento de pressionar; keyDown indica que a tecla continua pressionada.
    bool keyPressed(int key) const { return key >= 0 && key <= GLFW_KEY_LAST && keysPressedThisFrame[key]; }
    bool keyDown(int key) const { return key >= 0 && key <= GLFW_KEY_LAST && keyStates[key]; }
    bool mouseClicked(int button) const { return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST && mouseClicksThisFrame[button]; }
    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }
    double getScrollY() const { return scrollY; }

    void destroy();

    GLFWwindow *getHandle() const { return windowHandle; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    float aspectRatio() const { return height > 0 ? static_cast<float>(width) / height : 1.0f; }
    
    enum class CursorMode { Normal, Hidden, Disabled };

    bool mouseDown(int button) const { return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST && mouseButtons[button]; }
    void setCursorMode(CursorMode mode);
    void setMousePosition(double x, double y);
};