#pragma once

#include <string>

#include "font.h"
#include "mesh.h"
#include "shader.h"
#include "window.h"

// Desenha elementos 2D em pixels (retângulos, texto, botões) por cima da cena.
class UI
{
public:
    bool create();

    // Chamar antes e depois de desenhar a interface do quadro.
    void begin(const Window& window);
    void end();

    void drawRect(float x, float y, float width, float height,
                  float r, float g, float b, float a = 1.0f);

    // (x, y) é o começo da linha de base do texto.
    void drawText(const std::string& text, float x, float y, float scale,
                  float r, float g, float b, float a = 1.0f);

    void drawTextCentered(const std::string& text, float x, float y, float width, float height,
                          float scale, float r, float g, float b, float a = 1.0f);

    // Devolve true se foi clicado neste quadro.
    bool button(const std::string& label, float x, float y, float width, float height,
                float r, float g, float b, bool enabled = true);

private:
    const Window* window = nullptr; // só vale entre begin() e end()
    Shader rectShader;
    Shader textShader;
    Mesh quad;
    Font font;
};