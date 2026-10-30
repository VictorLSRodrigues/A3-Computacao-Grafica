#pragma once

#include "shader.h"
#include "mesh.h"
#include "window.h"

class Menu
{
private:
    Shader shaderUI;
    Mesh retangulo;
    bool aberto = false;

    void desenharRetangulo(const Window& janela, float x, float y, float largura, float altura,
                           float r, float g, float b, float a);

    // Desenha um botão e devolve true se ele foi clicado neste quadro.
    bool botao(const Window& janela, float x, float y, float largura, float altura);

public:
    bool criar();
    void atualizar(const Window& janela); // ESC abre e fecha
    void desenhar(const Window& janela);  // desenha o menu e trata os cliques
    bool estaAberto() const { return aberto; }
};