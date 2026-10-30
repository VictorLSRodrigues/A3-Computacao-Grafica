#include "window.h"

#include <iostream>

void Window::aoRedimensionar(GLFWwindow* janela, int largura, int altura)
{
    glViewport(0, 0, largura, altura);

    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(janela));
    if (self)
    {
        self->largura = largura;
        self->altura = altura;
    }
}

bool Window::criar(const char* titulo, bool telaCheia, int larguraJanela, int alturaJanela)
{
    if (!glfwInit())
    {
        std::cout << "Erro ao inicializar o GLFW!" << std::endl;
        return false;
    }
    glfwIniciado = true;

    // OpenGL 3.3 no perfil Core.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Em tela cheia, passa o monitor e usa a resolução dele.
    GLFWmonitor* monitor = nullptr;
    if (telaCheia)
    {
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
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

    if (!gladLoadGL())
    {
        std::cout << "Erro ao inicializar o GLAD!" << std::endl;
        limpar();
        return false;
    }

    largura = larguraJanela;
    altura = alturaJanela;

    // Guarda "this" na janela pra o callback conseguir atualizar largura/altura.
    glfwSetWindowUserPointer(janela, this);
    glfwSetFramebufferSizeCallback(janela, aoRedimensionar);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    return true;
}

bool Window::deveFechar() const { return glfwWindowShouldClose(janela); }
void Window::fechar() const { glfwSetWindowShouldClose(janela, GLFW_TRUE); }
void Window::trocarBuffers() const { glfwSwapBuffers(janela); }
void Window::processarEventos() const { glfwPollEvents(); }

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