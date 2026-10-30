#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window
{
private:
    GLFWwindow* janela = nullptr;
    int largura = 0;
    int altura = 0;
    bool glfwIniciado = false;

    // Chamada pelo GLFW quando a janela muda de tamanho.
    static void aoRedimensionar(GLFWwindow* janela, int largura, int altura);

public:
    Window() = default;
    ~Window() { limpar(); }

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // telaCheia = true usa a resolução do monitor; senão usa largura x altura.
    bool criar(const char* titulo, bool telaCheia = true,
               int larguraJanela = 1280, int alturaJanela = 720);

    bool deveFechar() const;
    void fechar() const;          // pede pra janela fechar
    void trocarBuffers() const;
    void processarEventos() const;
    void limpar();

    GLFWwindow* ponteiro() const { return janela; }
    int getLargura() const { return largura; }
    int getAltura() const { return altura; }
    float aspecto() const { return altura > 0 ? static_cast<float>(largura) / altura : 1.0f; }
};