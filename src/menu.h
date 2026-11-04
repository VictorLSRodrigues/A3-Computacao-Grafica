#pragma once

#include "shader.h"
#include "mesh.h"
#include "window.h"
#include "font.h"
#include <string>

class Menu
{
private:
    Shader shaderUI;
    Mesh retangulo;
    Font font;
    bool aberto = false;

    void desenharRetangulo(const Window &janela, float x, float y, float largura, float altura,
                           float r, float g, float b, float a);

    Shader textShader;

    void drawText(const Window &window, const std::string &text, float x, float y, float scale,
                  float red, float green, float blue, float alpha);

    void drawTextCentered(const Window &window, const std::string &text,
                          float x, float y, float width, float height, float scale,
                          float red, float green, float blue, float alpha);

    bool botao(const Window &janela, float x, float y, float largura, float altura,
               const std::string &label, float r, float g, float b, bool enabled = true);

public:
    bool criar();
    void atualizar(const Window &janela); // ESC abre e fecha
    void desenhar(const Window &janela);  // desenha o menu e trata os cliques
    bool estaAberto() const { return aberto; }
    bool criarEbo();
};