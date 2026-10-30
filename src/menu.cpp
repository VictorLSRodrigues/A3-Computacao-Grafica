#include "menu.h"
#include "formas.h"

#include <iostream>

bool Menu::criar()
{
    if (!shaderUI.criarDeArquivos(ASSETS_DIR "shaders/ui.vert",
                                  ASSETS_DIR "shaders/ui.frag"))
    {
        std::cout << "Falha ao criar o shader da UI!" << std::endl;
        return false;
    }

    if (!retangulo.criar(formas::retangulo(), 2)) // 2 componentes (x, y)
    {
        std::cout << "Falha ao criar o retangulo!" << std::endl;
        return false;
    }

    // Permite cores com transparência (o quarto número da cor).
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void Menu::atualizar(const Window& janela)
{
    if (janela.teclaApertou(GLFW_KEY_ESCAPE))
        aberto = !aberto;
}

void Menu::desenharRetangulo(const Window& janela, float x, float y, float largura, float altura,
                             float r, float g, float b, float a)
{
    shaderUI.usar();
    shaderUI.setVec2("uTela", (float)janela.getLargura(), (float)janela.getAltura());
    shaderUI.setVec2("uPos", x, y);
    shaderUI.setVec2("uTamanho", largura, altura);
    shaderUI.setVec4("uCor", r, g, b, a);
    retangulo.desenhar();
}

bool Menu::botao(const Window& janela, float x, float y, float largura, float altura)
{
    float mx = (float)janela.getMouseX();
    float my = (float)janela.getMouseY();
    bool sobre = mx >= x && mx <= x + largura && my >= y && my <= y + altura;

    // Cor fixa por enquanto (o texto do botão vem depois).
    if (sobre)
        desenharRetangulo(janela, x, y, largura, altura, 0.9f, 0.2f, 0.2f, 1.0f);
    else
        desenharRetangulo(janela, x, y, largura, altura, 0.6f, 0.1f, 0.1f, 1.0f);

    return sobre && janela.mouseClicou(GLFW_MOUSE_BUTTON_LEFT);
}

void Menu::desenhar(const Window& janela)
{
    if (!aberto)
        return;

    float tela_l = (float)janela.getLargura();
    float tela_a = (float)janela.getAltura();

    // Escurece a cena toda
    desenharRetangulo(janela, 0.0f, 0.0f, tela_l, tela_a, 0.0f, 0.0f, 0.0f, 0.5f);

    // Painel no centro da tela
    float painelL = 400.0f;
    float painelA = 300.0f;
    float px = (tela_l - painelL) / 2.0f;
    float py = (tela_a - painelA) / 2.0f;
    desenharRetangulo(janela, px, py, painelL, painelA, 0.1f, 0.12f, 0.18f, 0.95f);

    // Botão Sair, na parte de baixo do painel
    if (botao(janela, px + 50.0f, py + painelA - 120.0f, 300.0f, 70.0f))
        janela.fechar();
}