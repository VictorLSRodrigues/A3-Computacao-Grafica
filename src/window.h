#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window
{
private:
    GLFWwindow *windowHandle = nullptr;
    int width = 0;
    int height = 0;
    bool glfwInitialized = false;

    // Estados contínuos e eventos de pressionamento de teclado e mouse.
    bool keyStates[GLFW_KEY_LAST + 1] = {};             // true enquanto a tecla estiver pressionada
    bool keysPressedThisFrame[GLFW_KEY_LAST + 1] = {};  // true no quadro em que a tecla foi pressionada
    bool mouseClicksThisFrame[GLFW_MOUSE_BUTTON_LAST + 1] = {};
    bool mouseButtons[GLFW_MOUSE_BUTTON_LAST + 1] = {}; // true enquanto o botão estiver pressionado
    double mouseX = 0.0;
    double mouseY = 0.0;

    static void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void onMouseMove(GLFWwindow *window, double x, double y);
    static void onMouseButton(GLFWwindow *window, int button, int action, int mods);

    // Atualiza o viewport e as dimensões ao redimensionar a janela.
    static void onFramebufferResize(GLFWwindow *window, int width, int height);

public:
    Window() = default;
    ~Window() { destroy(); }

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    // Em tela cheia usa a resolução do monitor; caso contrário, usa as dimensões informadas.
    bool create(const char *title, bool fullscreen = true,
                int windowWidth = 1280, int windowHeight = 720);

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