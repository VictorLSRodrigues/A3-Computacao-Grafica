#pragma once

#include <string>

#include "font.h"
#include "graphics/mesh.h"
#include "graphics/shader.h"
#include "window.h"

// Desenha retângulos, texto e botões em coordenadas de pixel sobre a cena.
class UI
{
public:
    bool create();

    // begin() configura o estado de desenho 2D; end() restaura o estado da cena 3D.
    void begin(const Window &window);
    void end();

    void drawRect(float x, float y, float width, float height,
                  float r, float g, float b, float a = 1.0f);

    // (x, y) indica o início da linha de base do texto.
    void drawText(const std::string &text, float x, float y, float scale,
                  float r, float g, float b, float a = 1.0f);

    void drawTextCentered(const std::string &text, float x, float y, float width, float height,
                          float scale, float r, float g, float b, float a = 1.0f);

    // Retorna true quando o botão é clicado neste quadro.
    bool button(const std::string &label, float x, float y, float width, float height,
                float r, float g, float b, bool enabled = true);

    // Exibe "rótulo [-] valor [+]", limita o valor ao intervalo informado
    // e retorna true se ele mudar neste quadro.
    bool stepper(const std::string &label, float &value, float step, float minValue, float maxValue,
                 float x, float y, float width, float height, int decimals = 1);

private:
    const Window *window = nullptr; // Válido apenas entre begin() e end().
    Shader rectShader;
    Shader textShader;
    Mesh quad;
    Font font;
};