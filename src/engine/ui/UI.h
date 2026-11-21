#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "font.h"
#include "theme.h"
#include "engine/graphics/mesh/mesh.h"
#include "engine/graphics/resources/shader.h"
#include "engine/platform/window.h"
#include "engine/graphics/resources/texture.h"

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

    // Desenha uma linha entre dois pontos da tela.
    void drawLine(
        float x1, float y1,
        float x2, float y2,
        float thickness,
        float r, float g, float b,
        float a = 1.0f);

    // Largura que o texto ocupa, em pixels, na escala informada.
    float measureText(const std::string &text, float scale) const { return font.measure(text, scale); }

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

    // Barra arrastável: "rótulo [====|----] valor". Retorna true se o valor mudar neste quadro.
    // (r, g, b) tingem a parte preenchida; displayScale só afeta o número exibido
    // (por exemplo 255 para mostrar uma cor de 0 a 1 como 0 a 255).
    bool slider(const std::string &label, float &value, float minValue, float maxValue,
                float x, float y, float width, float height,
                float r = theme::accent.r, float g = theme::accent.g, float b = theme::accent.b,
                int decimals = 2, float displayScale = 1.0f,
                const std::string &id = {});

    // Editor de cor RGB (0 a 1): amostra com o código hexadecimal e uma barra por canal.
    // O ID opcional mantém a identidade dos controles independente da posição na tela.
    // Retorna true se a cor mudar neste quadro.
    bool colorPicker(const std::string &label, glm::vec3 &rgb, float x, float y, float width,
                     const std::string &id = {});

    // Altura que o colorPicker ocupa, para quem precisa reservar espaço no layout.
    static float colorPickerHeight();

    // Botão com ícone fornecido pela aplicação ou ferramenta que o utiliza.
    bool iconButton(
        const Texture &icon,
        const std::string &tooltip,
        float x, float y,
        float width, float height,
        bool selected = false);

    // Registra uma região da tela ocupada pela interface neste quadro.
    void addRegion(float x, float y, float width, float height);

    // True se o mouse está sobre a interface ou arrastando um slider.
    // Quem trata cliques na cena deve perguntar isso antes de selecionar objetos.
    bool wantsMouse(const Window &window) const;

private:
    struct Region { float x, y, w, h; };
    std::vector<Region> regions; // preenchida durante o quadro, zerada em begin()
    
    const Window *window = nullptr; // Válido apenas entre begin() e end().
    Shader rectShader;
    Shader textShader;
    Mesh quad;
    Font font;

    std::size_t activeSliderId = 0; // Slider sendo arrastado; 0 se nenhum.

    Shader iconShader;

    void drawIcon(
        const Texture &texture,
        float x, float y,
        float size,
        float r, float g, float b);
};