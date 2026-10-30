#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window
{
private:
    GLFWwindow *janela = nullptr;
    int largura = 0;
    int altura = 0;
    bool glfwIniciado = false;

    // Teclado e mouse
    bool teclas[GLFW_KEY_LAST + 1] = {};         // true enquanto a tecla está segurada
    bool teclasNoQuadro[GLFW_KEY_LAST + 1] = {}; // true só no quadro em que apertou
    bool cliquesNoQuadro[GLFW_MOUSE_BUTTON_LAST + 1] = {};
    double mouseX = 0.0;
    double mouseY = 0.0;

    static void aoTeclar(GLFWwindow *janela, int tecla, int scancode, int acao, int mods);
    static void aoMoverMouse(GLFWwindow *janela, double x, double y);
    static void aoClicarMouse(GLFWwindow *janela, int botao, int acao, int mods);

    // Chamada pelo GLFW quando a janela muda de tamanho.
    static void aoRedimensionar(GLFWwindow *janela, int largura, int altura);

public:
    Window() = default;
    ~Window() { limpar(); }

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    // telaCheia = true usa a resolução do monitor; senão usa largura x altura.
    bool criar(const char *titulo, bool telaCheia = true,
               int larguraJanela = 1280, int alturaJanela = 720);

    bool deveFechar() const;
    void fechar() const; // pede pra janela fechar
    void trocarBuffers() const;
    void processarEventos();

    bool teclaApertou(int tecla) const { return tecla >= 0 && tecla <= GLFW_KEY_LAST && teclasNoQuadro[tecla]; }
    bool teclaSegurada(int tecla) const { return tecla >= 0 && tecla <= GLFW_KEY_LAST && teclas[tecla]; }
    bool mouseClicou(int botao) const { return botao >= 0 && botao <= GLFW_MOUSE_BUTTON_LAST && cliquesNoQuadro[botao]; }
    double getMouseX() const { return mouseX; }
    double getMouseY() const { return mouseY; }
    
    void limpar();

    GLFWwindow *ponteiro() const { return janela; }
    int getLargura() const { return largura; }
    int getAltura() const { return altura; }
    float aspecto() const { return altura > 0 ? static_cast<float>(largura) / altura : 1.0f; }
};