#include "UI.h"

#include <algorithm>
#include <iostream>
#include <cstdio>

#include "graphics/shapes.h"

bool UI::create()
{
    if (!rectShader.createFromFiles(ASSETS_DIR "shaders/ui.vert", ASSETS_DIR "shaders/ui.frag"))
    {
        std::cout << "Falha ao criar o shader da UI!" << std::endl;
        return false;
    }

    if (!textShader.createFromFiles(ASSETS_DIR "shaders/text.vert", ASSETS_DIR "shaders/text.frag"))
    {
        std::cout << "Falha ao criar o shader de texto!" << std::endl;
        return false;
    }

    if (!quad.createIndexed(shapes::rectangle(), shapes::rectangleIndices(), 2))
    {
        std::cout << "Falha ao criar o retangulo da UI!" << std::endl;
        return false;
    }

    return font.load(ASSETS_DIR "fonts/DejaVuSans.ttf");
}

// Liga o que a interface precisa: sem profundidade, com transparência, sem descartar faces.
void UI::begin(const Window &w)
{
    window = &w;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Restaura o estado do 3D.
void UI::end()
{
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    window = nullptr;
}

void UI::drawRect(float x, float y, float width, float height,
                  float r, float g, float b, float a)
{
    if (!window)
        return;

    rectShader.use();
    rectShader.setVec2("uTela", (float)window->getWidth(), (float)window->getHeight());
    rectShader.setVec2("uPos", x, y);
    rectShader.setVec2("uTamanho", width, height);
    rectShader.setVec4("uCor", r, g, b, a);
    quad.draw();
}

void UI::drawText(const std::string &text, float x, float y, float scale,
                  float r, float g, float b, float a)
{
    if (!window)
        return;

    textShader.use();
    textShader.setVec2("uScreen", (float)window->getWidth(), (float)window->getHeight());
    textShader.setVec4("uColor", r, g, b, a);
    font.getTexture().bind(0);

    float cursorX = x;
    for (unsigned int codepoint : Font::decodeUtf8(text))
    {
        const Font::Glyph &glyph = font.getGlyph(codepoint);

        textShader.setVec2("uPos", cursorX + glyph.x0 * scale, y + glyph.y0 * scale);
        textShader.setVec2("uSize", (glyph.x1 - glyph.x0) * scale, (glyph.y1 - glyph.y0) * scale);
        textShader.setVec2("uUVPos", glyph.s0, glyph.t0);
        textShader.setVec2("uUVSize", glyph.s1 - glyph.s0, glyph.t1 - glyph.t0);
        quad.draw();

        cursorX += glyph.advance * scale;
    }
}

void UI::drawTextCentered(const std::string &text, float x, float y, float width, float height,
                          float scale, float r, float g, float b, float a)
{
    float textWidth = font.measure(text, scale);
    float capHeight = font.getPixelHeight() * scale * 0.62f; // altura das maiúsculas

    drawText(text,
             x + (width - textWidth) / 2.0f,
             y + (height + capHeight) / 2.0f,
             scale, r, g, b, a);
}

bool UI::button(const std::string &label, float x, float y, float width, float height,
                float r, float g, float b, bool enabled)
{
    if (!window)
        return false;

    float mx = (float)window->getMouseX();
    float my = (float)window->getMouseY();
    bool hovered = enabled && mx >= x && mx <= x + width && my >= y && my <= y + height;

    if (!enabled)
    {
        r = 0.22f;
        g = 0.24f;
        b = 0.28f; // cinza apagado
    }
    else if (hovered)
    {
        r = std::min(r * 1.35f, 1.0f);
        g = std::min(g * 1.35f, 1.0f);
        b = std::min(b * 1.35f, 1.0f);
    }

    drawRect(x, y, width, height, r, g, b, 1.0f);
    drawTextCentered(label, x, y, width, height, 0.6f, 1.0f, 1.0f, 1.0f, enabled ? 1.0f : 0.4f);

    return hovered && window->mouseClicked(GLFW_MOUSE_BUTTON_LEFT);
}

bool UI::stepper(const std::string &label, float &value, float step, float minValue, float maxValue,
                 float x, float y, float width, float height, int decimals)
{
    if (!window)
        return false;

    const float buttonSize = height;
    const float valueW = 90.0f;
    const float controlsW = buttonSize + valueW + buttonSize;
    const float controlsX = x + width - controlsW;
    const float old = value;

    // Nome do ajuste, alinhado à esquerda
    const float labelScale = 0.5f;
    const float capHeight = font.getPixelHeight() * labelScale * 0.62f;
    drawText(label, x, y + (height + capHeight) / 2.0f, labelScale, 1.0f, 1.0f, 1.0f);

    // Botões primeiro: eles podem mudar o valor, e o número mostrado já tem que ser o novo
    if (button("-", controlsX, y, buttonSize, height, 0.25f, 0.30f, 0.40f))
        value -= step;
    if (button("+", controlsX + buttonSize + valueW, y, buttonSize, height, 0.25f, 0.30f, 0.40f))
        value += step;

    value = std::clamp(value, minValue, maxValue);

    // Valor no meio, numa caixa escura
    drawRect(controlsX + buttonSize, y, valueW, height, 0.04f, 0.05f, 0.08f, 1.0f);

    char text[32];
    std::snprintf(text, sizeof(text), "%.*f", decimals, value);
    drawTextCentered(text, controlsX + buttonSize, y, valueW, height, 0.5f, 1.0f, 1.0f, 1.0f);

    return value != old;
}