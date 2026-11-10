#include "UI.h"

#include <algorithm>
#include <iostream>
#include <cstdio>
#include <cmath>

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

    if (!iconShader.createFromFiles(
            ASSETS_DIR "shaders/text.vert",
            ASSETS_DIR "shaders/icon.frag"))
    {
        std::cout << "Falha ao criar o shader dos icones!" << std::endl;
        return false;
    }

    if (!moveIcon.load(ASSETS_DIR "icons/move.png", false) ||
    !rotateIcon.load(ASSETS_DIR "icons/rotate-3d.png", false) ||
    !scaleIcon.load(ASSETS_DIR "icons/scaling.png", false))
{
    std::cout << "Falha ao carregar os icones da UI!" << std::endl;
    return false;
}

    return font.load(ASSETS_DIR "fonts/DejaVuSans.ttf");
}

// Configura o OpenGL para desenhar a interface transparente sobre a cena.
void UI::begin(const Window &w)
{
    window = &w;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Restaura o estado usado para desenhar a cena 3D.
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

void UI::drawLine(
    float x1, float y1,
    float x2, float y2,
    float thickness,
    float r, float g, float b,
    float a)
{
    if (!window)
        return;

    const float dx = x2 - x1;
    const float dy = y2 - y1;
    const float length = std::sqrt(dx * dx + dy * dy);

    if (length <= 0.001f)
        return;

    const float angle = std::atan2(dy, dx);

    const float centerX = (x1 + x2) * 0.5f;
    const float centerY = (y1 + y2) * 0.5f;

    rectShader.use();

    rectShader.setVec2(
        "uTela",
        static_cast<float>(window->getWidth()),
        static_cast<float>(window->getHeight())
    );

    rectShader.setVec2(
        "uPos",
        centerX - length * 0.5f,
        centerY - thickness * 0.5f
    );

    rectShader.setVec2("uTamanho", length, thickness);
    rectShader.setVec4("uCor", r, g, b, a);

    glUniform1f(
        glGetUniformLocation(rectShader.ID, "uAngle"),
        angle
    );

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
    float capHeight = font.getPixelHeight() * scale * 0.62f; // Aproxima a altura das maiúsculas.

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
        b = 0.28f; // Mantém a cor do botão desativado em tons de cinza.
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

    // Mostra o nome do ajuste à esquerda dos controles.
    const float labelScale = 0.5f;
    const float capHeight = font.getPixelHeight() * labelScale * 0.62f;
    drawText(label, x, y + (height + capHeight) / 2.0f, labelScale, 1.0f, 1.0f, 1.0f);

    // Atualiza o valor antes de desenhá-lo para exibir a alteração no mesmo quadro.
    if (button("-", controlsX, y, buttonSize, height, 0.25f, 0.30f, 0.40f))
        value -= step;
    if (button("+", controlsX + buttonSize + valueW, y, buttonSize, height, 0.25f, 0.30f, 0.40f))
        value += step;

    value = std::clamp(value, minValue, maxValue);

    // Destaca o valor entre os botões de decremento e incremento.
    drawRect(controlsX + buttonSize, y, valueW, height, 0.04f, 0.05f, 0.08f, 1.0f);

    char text[32];
    std::snprintf(text, sizeof(text), "%.*f", decimals, value);
    drawTextCentered(text, controlsX + buttonSize, y, valueW, height, 0.5f, 1.0f, 1.0f, 1.0f);

    return value != old;
}

void UI::drawIcon(
    const Texture& texture,
    float x, float y,
    float size,
    float r, float g, float b)
{
    if (!window)
        return;

    iconShader.use();

    iconShader.setVec2(
        "uScreen",
        static_cast<float>(window->getWidth()),
        static_cast<float>(window->getHeight())
    );

    iconShader.setVec2("uPos", x, y);
    iconShader.setVec2("uSize", size, size);
    iconShader.setVec2("uUVPos", 0.0f, 0.0f);
    iconShader.setVec2("uUVSize", 1.0f, 1.0f);

    iconShader.setVec4("uColor", r, g, b, 1.0f);
    iconShader.setInt("uIcon", 0);

    texture.bind(0);
    quad.draw();
}

bool UI::iconButton(
    Icon icon,
    const std::string& tooltip,
    float x, float y,
    float width, float height,
    bool selected)
{
    if (!window)
        return false;

    const float mx = static_cast<float>(window->getMouseX());
    const float my = static_cast<float>(window->getMouseY());

    const bool hovered =
        mx >= x && mx <= x + width &&
        my >= y && my <= y + height;

    const Texture* texture = nullptr;

    switch (icon)
    {
        case Icon::Move:
            texture = &moveIcon;
            break;

        case Icon::Rotate:
            texture = &rotateIcon;
            break;

        case Icon::Scale:
            texture = &scaleIcon;
            break;
    }

    float r = selected ? 0.12f : 0.18f;
    float g = selected ? 0.38f : 0.20f;
    float b = selected ? 0.75f : 0.25f;

    if (hovered)
    {
        r = std::min(r * 1.3f, 1.0f);
        g = std::min(g * 1.3f, 1.0f);
        b = std::min(b * 1.3f, 1.0f);
    }

    drawRect(x, y, width, height, r, g, b);

    const float iconSize = 25.0f;

    drawIcon(
        *texture,
        x + (width - iconSize) / 2.0f,
        y + (height - iconSize) / 2.0f,
        iconSize,
        1.0f, 1.0f, 1.0f
    );

    // Mostra o nome da ferramenta ao passar o mouse.
    if (hovered)
    {
        const float tooltipWidth = std::max(
            54.0f,
            font.measure(tooltip, 0.45f) + 14.0f
        );

        const float tooltipX = x + (width - tooltipWidth) / 2.0f;
        const float tooltipY = y + height + 6.0f;

        drawRect(
            tooltipX, tooltipY,
            tooltipWidth, 26.0f,
            0.04f, 0.05f, 0.08f, 0.96f
        );

        drawTextCentered(
            tooltip,
            tooltipX, tooltipY,
            tooltipWidth, 26.0f,
            0.45f,
            1.0f, 1.0f, 1.0f
        );
    }

    return hovered && window->mouseClicked(GLFW_MOUSE_BUTTON_LEFT);
}