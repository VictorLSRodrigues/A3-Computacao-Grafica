#include "UI.h"

#include <algorithm>
#include <iostream>
#include <cstdio>
#include <cmath>
#include <functional>

#include "engine/graphics/geometry/shapes.h"
#include "theme.h"

bool UI::create()
{
    if (!rectShader.createFromFiles(ASSETS_DIR "engine/shaders/ui.vert", ASSETS_DIR "engine/shaders/ui.frag"))
    {
        std::cout << "Falha ao criar o shader da UI!" << std::endl;
        return false;
    }

    if (!textShader.createFromFiles(ASSETS_DIR "engine/shaders/text.vert", ASSETS_DIR "engine/shaders/text.frag"))
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
            ASSETS_DIR "engine/shaders/text.vert",
            ASSETS_DIR "engine/shaders/icon.frag"))
    {
        std::cout << "Falha ao criar o shader dos icones!" << std::endl;
        return false;
    }

    return font.load(ASSETS_DIR "engine/fonts/DejaVuSans.ttf");
}

// Configura o OpenGL para desenhar a interface transparente sobre a cena.
void UI::begin(const Window &w)
{
    regions.clear();
    if (activeSliderId != 0 && !w.mouseDown(GLFW_MOUSE_BUTTON_LEFT))
        activeSliderId = 0;
    window = &w;
    framebufferSRGB = glIsEnabled(GL_FRAMEBUFFER_SRGB) == GL_TRUE;

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
    rectShader.setInt("uFramebufferSRGB", framebufferSRGB ? 1 : 0);
    rectShader.setVec2("uTela", (float)window->getWidth(), (float)window->getHeight());
    rectShader.setVec2("uPos", x, y);
    rectShader.setVec2("uTamanho", width, height);
    rectShader.setVec4("uCor", r, g, b, a);

    glUniform1f(
        glGetUniformLocation(rectShader.ID, "uAngle"),
        0.0f);
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
    rectShader.setInt("uFramebufferSRGB", framebufferSRGB ? 1 : 0);

    rectShader.setVec2(
        "uTela",
        static_cast<float>(window->getWidth()),
        static_cast<float>(window->getHeight()));

    rectShader.setVec2(
        "uPos",
        centerX - length * 0.5f,
        centerY - thickness * 0.5f);

    rectShader.setVec2("uTamanho", length, thickness);
    rectShader.setVec4("uCor", r, g, b, a);

    glUniform1f(
        glGetUniformLocation(rectShader.ID, "uAngle"),
        angle);

    quad.draw();
}

void UI::drawText(const std::string &text, float x, float y, float scale,
                  float r, float g, float b, float a)
{
    if (!window)
        return;

    textShader.use();
    textShader.setInt("uFramebufferSRGB", framebufferSRGB ? 1 : 0);
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
    // Evita que rótulos longos escapem de botões e campos em janelas estreitas.
    const float horizontalPadding = std::min(8.0f, std::max(2.0f, width * 0.08f));
    const float availableWidth = std::max(1.0f, width - horizontalPadding * 2.0f);
    const float textWidth = font.measure(text, scale);
    if (textWidth > availableWidth && textWidth > 0.0f)
        scale *= availableWidth / textWidth;

    // Também limita a altura para controles compactos.
    const float availableHeight = std::max(1.0f, height - 4.0f);
    const float glyphHeight = font.getPixelHeight() * scale;
    if (glyphHeight > availableHeight && glyphHeight > 0.0f)
        scale *= availableHeight / glyphHeight;

    const float fittedWidth = font.measure(text, scale);
    const float capHeight = font.getPixelHeight() * scale * theme::metrics::textCapHeightFactor;

    drawText(text,
             x + (width - fittedWidth) / 2.0f,
             y + (height + capHeight) / 2.0f,
             scale, r, g, b, a);
}

bool UI::button(const std::string &label, float x, float y, float width, float height,
                float r, float g, float b, bool enabled)
{
    if (!window)
        return false;

    addRegion(x, y, width, height);

    float mx = (float)window->getMouseX();
    float my = (float)window->getMouseY();
    bool hovered = enabled && mx >= x && mx <= x + width && my >= y && my <= y + height;

    const bool pressed = hovered && window->mouseDown(GLFW_MOUSE_BUTTON_LEFT);
    if (!enabled)
    {
        r = theme::buttonDisabled.r;
        g = theme::buttonDisabled.g;
        b = theme::buttonDisabled.b;
    }
    else if (pressed)
    {
        r *= theme::buttonPressedFactor;
        g *= theme::buttonPressedFactor;
        b *= theme::buttonPressedFactor;
    }
    else if (hovered)
    {
        r = std::min(r * theme::buttonHoverFactor, 1.0f);
        g = std::min(g * theme::buttonHoverFactor, 1.0f);
        b = std::min(b * theme::buttonHoverFactor, 1.0f);
    }

    drawRect(x, y, width, height, r, g, b, 1.0f);

    // Borda discreta para separar o controle do fundo sem criar excesso de linhas.
    const glm::vec3 &border = theme::buttonBorder;
    drawRect(x, y, width, 1.0f, border.r, border.g, border.b,
             enabled ? theme::buttonBorderOpacity : 0.45f);
    drawRect(x, y + height - 1.0f, width, 1.0f, border.r, border.g, border.b,
             enabled ? theme::buttonBorderOpacity : 0.45f);

    if (enabled && hovered)
    {
        const glm::vec3 &mark = pressed ? theme::accentSoft : theme::accent;
        drawRect(x, y, width, theme::buttonHoverMarkHeight,
                 mark.r, mark.g, mark.b, 1.0f);
    }

    drawTextCentered(label, x, y, width, height, theme::metrics::buttonTextScale,
                     theme::text.r, theme::text.g, theme::text.b,
                     enabled ? 1.0f : theme::disabledTextOpacity);

    return hovered && window->mouseClicked(GLFW_MOUSE_BUTTON_LEFT);
}

bool UI::stepper(const std::string &label, float &value, float step, float minValue, float maxValue,
                 float x, float y, float width, float height, int decimals)
{
    if (!window)
        return false;

    addRegion(x, y, width, height);

    const float buttonSize = theme::metrics::stepperButtonWidth;
    const float valueW = theme::metrics::stepperValueWidth;
    const float controlHeight = std::min(height, theme::metrics::stepperControlHeight);
    const float controlY = y + (height - controlHeight) * 0.5f;
    const float controlGap = theme::metrics::stepperControlGap;
    const float controlsW = buttonSize + valueW + buttonSize + 2.0f * controlGap;
    const float controlsX = x + width - controlsW;
    const float old = value;

    // Mostra o nome do ajuste à esquerda dos controles.
    const float labelScale = theme::metrics::labelTextScale;
    const float capHeight = font.getPixelHeight() * labelScale * theme::metrics::textCapHeightFactor;
    drawText(label, x, y + (height + capHeight) / 2.0f, labelScale,
             theme::text.r, theme::text.g, theme::text.b);

    // Atualiza o valor antes de desenhá-lo para exibir a alteração no mesmo quadro.
    if (button("-", controlsX, controlY, buttonSize, controlHeight,
               theme::buttonNeutral.r, theme::buttonNeutral.g, theme::buttonNeutral.b))
        value -= step;
    if (button("+", controlsX + buttonSize + valueW + 2.0f * controlGap, controlY,
               buttonSize, controlHeight,
               theme::buttonNeutral.r, theme::buttonNeutral.g, theme::buttonNeutral.b))
        value += step;

    value = std::clamp(value, minValue, maxValue);

    // Destaca o valor entre os botões de decremento e incremento.
    const float valueX = controlsX + buttonSize + controlGap;
    drawRect(valueX, controlY, valueW, controlHeight,
             theme::fieldBackground.r, theme::fieldBackground.g, theme::fieldBackground.b, 1.0f);
    drawRect(valueX, controlY, 2.0f, controlHeight,
             theme::accentSoft.r, theme::accentSoft.g, theme::accentSoft.b, 1.0f);
    drawRect(valueX, controlY, valueW, 1.0f,
             theme::buttonBorder.r, theme::buttonBorder.g, theme::buttonBorder.b, 0.75f);
    drawRect(valueX, controlY + controlHeight - 1.0f, valueW, 1.0f,
             theme::buttonBorder.r, theme::buttonBorder.g, theme::buttonBorder.b, 0.75f);

    char text[32];
    std::snprintf(text, sizeof(text), "%.*f", decimals, value);
    drawTextCentered(text, valueX, controlY, valueW, controlHeight,
                     theme::metrics::labelTextScale,
                     theme::text.r, theme::text.g, theme::text.b);

    return value != old;
}

void UI::drawIcon(
    const Texture &texture,
    float x, float y,
    float size,
    float r, float g, float b)
{
    if (!window)
        return;

    iconShader.use();
    iconShader.setInt("uFramebufferSRGB", framebufferSRGB ? 1 : 0);

    iconShader.setVec2(
        "uScreen",
        static_cast<float>(window->getWidth()),
        static_cast<float>(window->getHeight()));

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
    const Texture &icon,
    const std::string &tooltip,
    float x, float y,
    float width, float height,
    bool selected)
{
    if (!window)
        return false;

    addRegion(x, y, width, height);

    const float mx = static_cast<float>(window->getMouseX());
    const float my = static_cast<float>(window->getMouseY());

    const bool hovered =
        mx >= x && mx <= x + width &&
        my >= y && my <= y + height;

    glm::vec3 color = selected ? theme::buttonPrimary : theme::iconButtonBackground;

    if (hovered && window->mouseDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        color *= theme::iconPressedFactor;
    }
    else if (hovered)
    {
        color = glm::min(color * theme::iconHoverFactor + theme::iconHoverOffset, glm::vec3(1.0f));
    }

    drawRect(x, y, width, height, color.r, color.g, color.b);
    if (selected)
        drawRect(x, y + height - theme::metrics::selectedIndicatorHeight,
                 width, theme::metrics::selectedIndicatorHeight,
                 theme::accent.r, theme::accent.g, theme::accent.b);

    const float iconSize = std::min(theme::metrics::iconMaxSize,
                                    std::min(width, height) * theme::metrics::iconScale);

    drawIcon(
        icon,
        x + (width - iconSize) / 2.0f,
        y + (height - iconSize) / 2.0f,
        iconSize,
        theme::text.r, theme::text.g, theme::text.b);

    // Mostra o nome da ferramenta ao passar o mouse.
    if (hovered)
    {
        const float tooltipWidth = std::max(
            theme::metrics::iconTooltipMinWidth,
            font.measure(tooltip, theme::metrics::tooltipTextScale) +
                theme::metrics::iconTooltipPadding);

        const float tooltipX = x + (width - tooltipWidth) / 2.0f;
        const float tooltipY = y + height + theme::metrics::iconTooltipGap;

        drawRect(
            tooltipX, tooltipY,
            tooltipWidth, theme::metrics::iconTooltipHeight,
            theme::tooltipBackground.r, theme::tooltipBackground.g,
            theme::tooltipBackground.b, theme::panelOpacity);

        drawTextCentered(
            tooltip,
            tooltipX, tooltipY,
            tooltipWidth, theme::metrics::iconTooltipHeight,
            theme::metrics::tooltipTextScale,
            theme::text.r, theme::text.g, theme::text.b);
    }

    return hovered && window->mouseClicked(GLFW_MOUSE_BUTTON_LEFT);
}

bool UI::slider(const std::string &label, float &value, float minValue, float maxValue,
                float x, float y, float width, float height,
                float r, float g, float b, int decimals, float displayScale,
                const std::string &widgetId)
{
    if (!window)
        return false;

    addRegion(x, y, width, height);

    const float labelW = theme::metrics::sliderLabelWidth;
    const float valueW = theme::metrics::sliderValueWidth;
    const float trackX = x + labelW;
    const float valueX = x + width - valueW;
    const float trackW = std::max(valueX - theme::metrics::sliderValueInset - trackX, 1.0f);
    const float handleW = theme::metrics::sliderHandleWidth;
    const float old = value;

    // A identidade não depende da posição: redimensionar ou reposicionar o painel
    // não interrompe o arraste. Controles com rótulos repetidos devem informar um ID.
    const std::string stableId = widgetId.empty() ? label : widgetId;
    std::size_t id = std::hash<std::string>{}(stableId);
    if (id == 0)
        id = 1;

    const float mx = static_cast<float>(window->getMouseX());
    const float my = static_cast<float>(window->getMouseY());
    const bool hovered = mx >= trackX && mx <= trackX + trackW && my >= y && my <= y + height;

    // O clique ativa o slider; ele continua ativo, mesmo com o mouse fora da barra, até soltar o botão.
    const bool clicked = hovered && window->mouseClicked(GLFW_MOUSE_BUTTON_LEFT);
    if (clicked)
        activeSliderId = id;
    else if (activeSliderId == id && !window->mouseDown(GLFW_MOUSE_BUTTON_LEFT))
        activeSliderId = 0;

    const bool active = activeSliderId == id;
    if (clicked || active)
    {
        const float t = std::clamp((mx - trackX) / trackW, 0.0f, 1.0f);
        value = minValue + t * (maxValue - minValue);
    }
    value = std::clamp(value, minValue, maxValue);

    const float range = maxValue - minValue;
    const float t = range > 0.0f ? (value - minValue) / range : 0.0f;

    // Nome do canal à esquerda.
    const float labelScale = theme::metrics::labelTextScale;
    const float capHeight = font.getPixelHeight() * labelScale * theme::metrics::textCapHeightFactor;
    drawText(label, x, y + (height + capHeight) / 2.0f, labelScale,
             theme::text.r, theme::text.g, theme::text.b);

    // Trilho, parte preenchida e marcador.
    const float trackH = std::min(theme::metrics::sliderTrackHeight,
                                  height * theme::metrics::sliderTrackHeightFactor);
    const float trackY = y + (height - trackH) / 2.0f;
    drawRect(trackX, trackY, trackW, trackH,
             theme::sliderTrack.r, theme::sliderTrack.g, theme::sliderTrack.b, 1.0f);
    drawRect(trackX, trackY, trackW * t, trackH, r, g, b, 1.0f);

    const float handleShade = (hovered || active) ? 1.0f : theme::sliderHandle.r;
    const float handleHeight = std::max(1.0f,
                                        std::min(height - 4.0f, theme::metrics::sliderHandleHeight));
    const float handleY = y + (height - handleHeight) * 0.5f;
    drawRect(trackX + trackW * t - handleW / 2.0f, handleY, handleW, handleHeight,
             handleShade, handleShade, handleShade, 1.0f);
    drawRect(trackX + trackW * t - 1.0f, handleY + 4.0f, 2.0f,
             std::max(0.0f, handleHeight - 8.0f), r, g, b, 1.0f);

    // Valor à direita.
    char text[32];
    std::snprintf(text, sizeof(text), "%.*f", decimals, value * displayScale);
    const float valueY = y + 3.0f;
    const float valueHeight = std::max(1.0f, height - 6.0f);
    drawRect(valueX, valueY, valueW, valueHeight,
             theme::fieldBackground.r, theme::fieldBackground.g, theme::fieldBackground.b, 1.0f);
    drawRect(valueX, valueY, 2.0f, valueHeight,
             theme::accentSoft.r, theme::accentSoft.g, theme::accentSoft.b, 1.0f);
    drawRect(valueX, valueY, valueW, 1.0f,
             theme::buttonBorder.r, theme::buttonBorder.g, theme::buttonBorder.b, 0.7f);
    drawRect(valueX, valueY + valueHeight - 1.0f, valueW, 1.0f,
             theme::buttonBorder.r, theme::buttonBorder.g, theme::buttonBorder.b, 0.7f);
    drawTextCentered(text, valueX, y, valueW, height,
                     theme::metrics::labelTextScale,
                     theme::text.r, theme::text.g, theme::text.b);

    return value != old;
}

float UI::colorPickerHeight()
{
    return theme::metrics::colorPreviewHeight + theme::metrics::colorPickerGap +
           3.0f * theme::metrics::colorSliderHeight +
           2.0f * theme::metrics::colorPickerGap;
}

bool UI::colorPicker(const std::string &label, glm::vec3 &rgb, float x, float y, float width,
                     const std::string &widgetId)
{
    if (!window)
        return false;

    addRegion(x, y, width, colorPickerHeight());

    bool changed = false;
    rgb = glm::clamp(rgb, glm::vec3(0.0f), glm::vec3(1.0f));

    // Linha de cima: nome à esquerda e amostra com o código hexadecimal à direita.
    const float labelScale = theme::metrics::labelTextScale;
    const float capHeight = font.getPixelHeight() * labelScale * theme::metrics::textCapHeightFactor;
    drawText(label, x,
             y + (theme::metrics::colorPreviewHeight + capHeight) / 2.0f,
             labelScale, theme::text.r, theme::text.g, theme::text.b);

    const float swatchW = width * 0.5f;
    const float swatchX = x + width - swatchW;
    drawRect(swatchX - 2.0f, y - 2.0f, swatchW + 4.0f,
             theme::metrics::colorPreviewHeight + 4.0f,
             theme::previewBorder.r, theme::previewBorder.g, theme::previewBorder.b, 1.0f);
    drawRect(swatchX, y, swatchW, theme::metrics::colorPreviewHeight,
             rgb.r, rgb.g, rgb.b, 1.0f);

    char hex[16];
    std::snprintf(hex, sizeof(hex), "#%02X%02X%02X",
                  static_cast<int>(std::lround(rgb.r * 255.0f)),
                  static_cast<int>(std::lround(rgb.g * 255.0f)),
                  static_cast<int>(std::lround(rgb.b * 255.0f)));

    // Texto escuro sobre cores claras e claro sobre cores escuras.
    const float luminance = 0.299f * rgb.r + 0.587f * rgb.g + 0.114f * rgb.b;
    const float textShade = luminance > 0.5f ? 0.05f : 1.0f;
    drawTextCentered(hex, swatchX, y, swatchW, theme::metrics::colorPreviewHeight,
                     theme::metrics::labelTextScale, textShade, textShade, textShade);

    // Uma barra por canal, exibida de 0 a 255. Cada canal recebe um ID próprio.
    const std::string idPrefix = widgetId.empty() ? label : widgetId;
    float sliderY = y + theme::metrics::colorPreviewHeight + theme::metrics::colorPickerGap;
    if (slider("R", rgb.r, 0.0f, 1.0f, x, sliderY, width,
               theme::metrics::colorSliderHeight, 0.90f, 0.25f, 0.25f,
               0, 255.0f, idPrefix + "/R"))
        changed = true;
    sliderY += theme::metrics::colorSliderHeight + theme::metrics::colorPickerGap;
    if (slider("G", rgb.g, 0.0f, 1.0f, x, sliderY, width,
               theme::metrics::colorSliderHeight, 0.25f, 0.80f, 0.35f,
               0, 255.0f, idPrefix + "/G"))
        changed = true;
    sliderY += theme::metrics::colorSliderHeight + theme::metrics::colorPickerGap;
    if (slider("B", rgb.b, 0.0f, 1.0f, x, sliderY, width,
               theme::metrics::colorSliderHeight, 0.30f, 0.45f, 0.95f,
               0, 255.0f, idPrefix + "/B"))
        changed = true;

    return changed;
}

void UI::addRegion(float x, float y, float width, float height)
{
    regions.push_back({x, y, width, height});
}

bool UI::wantsMouse(const Window &window) const
{
    if (activeSliderId != 0)
        return true; // arrastando um slider: o mouse é da interface, mesmo fora dele

    const float mx = (float)window.getMouseX();
    const float my = (float)window.getMouseY();

    for (const Region &r : regions)
    {
        if (mx >= r.x && mx <= r.x + r.w && my >= r.y && my <= r.y + r.h)
            return true;
    }
    return false;
}