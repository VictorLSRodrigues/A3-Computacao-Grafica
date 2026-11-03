#include "window.h"
#include <algorithm>
#include <iterator>

#include <iostream>

void Window::aoRedimensionar(GLFWwindow *janela, int largura, int altura)
{
    glViewport(0, 0, largura, altura);

    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(janela));
    if (self)
    {
        self->largura = largura;
        self->altura = altura;
    }
}

void Window::aoTeclar(GLFWwindow *janela, int tecla, int /*scancode*/, int acao, int /*mods*/)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(janela));
    if (!self || tecla < 0 || tecla > GLFW_KEY_LAST)
        return;

    if (acao == GLFW_PRESS)
    {
        self->teclas[tecla] = true;
        self->teclasNoQuadro[tecla] = true;
    }
    else if (acao == GLFW_RELEASE)
    {
        self->teclas[tecla] = false;
    }
}

void Window::aoMoverMouse(GLFWwindow *janela, double x, double y)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(janela));
    if (self)
    {
        self->mouseX = x;
        self->mouseY = y;
    }
}

void Window::aoClicarMouse(GLFWwindow *janela, int botao, int acao, int /*mods*/)
{
    Window *self = static_cast<Window *>(glfwGetWindowUserPointer(janela));
    if (!self || botao < 0 || botao > GLFW_MOUSE_BUTTON_LAST)
        return;

    if (acao == GLFW_PRESS)
    {
        self->mouseButtons[botao] = true;
        self->cliquesNoQuadro[botao] = true;
    }
    else if (acao == GLFW_RELEASE)
    {
        self->mouseButtons[botao] = false;
    }
}

bool Window::criar(const char *titulo, bool telaCheia, int larguraJanela, int alturaJanela)
{
    glfwSetErrorCallback([](int codigo, const char *descricao)
                         { std::cout << "Erro do GLFW (" << codigo << "): " << descricao << std::endl; });

    if (!glfwInit())
    {
        std::cout << "Erro ao inicializar o GLFW!" << std::endl;
        return false;
    }
    glfwIniciado = true;

    // OpenGL 3.3 no perfil Core.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_SAMPLES, 4); // suaviza as bordas (MSAA 4x)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Em tela cheia, passa o monitor e usa a resolução dele.
    GLFWmonitor *monitor = nullptr;
    if (telaCheia)
    {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        larguraJanela = mode->width;
        alturaJanela = mode->height;
    }

    janela = glfwCreateWindow(larguraJanela, alturaJanela, titulo, monitor, nullptr);
    if (!janela)
    {
        std::cout << "Erro ao criar a janela!" << std::endl;
        limpar();
        return false;
    }

    glfwMakeContextCurrent(janela);

    glfwSwapInterval(1); // vsync

    if (!gladLoadGL())
    {
        std::cout << "Erro ao inicializar o GLAD!" << std::endl;
        limpar();
        return false;
    }

    glfwGetFramebufferSize(janela, &largura, &altura);

    // Guarda "this" na janela pra o callback conseguir atualizar largura/altura.
    glfwSetWindowUserPointer(janela, this);
    glfwSetFramebufferSizeCallback(janela, aoRedimensionar);

    glfwSetKeyCallback(janela, aoTeclar);
    glfwSetCursorPosCallback(janela, aoMoverMouse);
    glfwSetMouseButtonCallback(janela, aoClicarMouse);
    glfwGetCursorPos(janela, &mouseX, &mouseY);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    return true;
}

bool Window::deveFechar() const { return glfwWindowShouldClose(janela); }
void Window::fechar() const { glfwSetWindowShouldClose(janela, GLFW_TRUE); }
void Window::trocarBuffers() const { glfwSwapBuffers(janela); }

void Window::processarEventos()
{
    // Limpa o "aconteceu neste quadro" do quadro anterior, depois lê os eventos novos.
    std::fill(std::begin(teclasNoQuadro), std::end(teclasNoQuadro), false);
    std::fill(std::begin(cliquesNoQuadro), std::end(cliquesNoQuadro), false);
    glfwPollEvents();
}

void Window::limpar()
{
    if (janela)
    {
        glfwDestroyWindow(janela);
        janela = nullptr;
    }
    if (glfwIniciado)
    {
        glfwTerminate();
        glfwIniciado = false;
    }
}

void Window::setCursorMode(CursorMode mode)
{
    int value = GLFW_CURSOR_NORMAL;
    if (mode == CursorMode::Hidden)
        value = GLFW_CURSOR_HIDDEN;
    else if (mode == CursorMode::Disabled)
        value = GLFW_CURSOR_DISABLED;

    glfwSetInputMode(janela, GLFW_CURSOR, value);
}

void Window::setMousePosition(double x, double y)
{
    glfwSetCursorPos(janela, x, y);
    mouseX = x; // já guarda o valor novo, sem esperar o GLFW avisar
    mouseY = y;
}