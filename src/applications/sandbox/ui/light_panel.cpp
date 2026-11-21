#include "light_panel.h"

#include <algorithm>

#include "applications/sandbox/ui/sandbox_ui_style.h"

namespace
{
    using sandbox_ui::ButtonTone;

    void drawVectorControls(UI &ui, const std::string &label,
                            glm::vec3 &value, float x, float y, float width,
                            float scale)
    {
        sandbox_ui::drawText(ui, label, x, y + 10.0f * scale,
                             0.35f, sandbox_ui::palette::muted);

        const float gap = 5.0f;
        const float axisWidth = (width - gap * 2.0f) / 3.0f;
        const float controlsY = y + 15.0f * scale;
        const float controlHeight = 27.0f * scale;
        sandbox_ui::axisStepper(ui, 'X', value.x, 0.25f, -10.0f, 10.0f,
                                x, controlsY, axisWidth, controlHeight);
        sandbox_ui::axisStepper(ui, 'Y', value.y, 0.25f, -10.0f, 10.0f,
                                x + axisWidth + gap, controlsY, axisWidth, controlHeight);
        sandbox_ui::axisStepper(ui, 'Z', value.z, 0.25f, -10.0f, 10.0f,
                                x + (axisWidth + gap) * 2.0f, controlsY,
                                axisWidth, controlHeight);
    }
}

void LightPanel::draw(const Window &window, UI &ui, Light &light)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());
    const float panelW = std::min(420.0f, std::max(1.0f, screenW - 32.0f));
    const float panelH = std::min(590.0f, std::max(1.0f, screenH - 32.0f));
    const float x = screenW - panelW - 16.0f;
    const float y = (screenH - panelH) * 0.5f;
    const float innerX = x + 22.0f;
    const float innerW = std::max(1.0f, panelW - 44.0f);

    // Mantém o cabeçalho legível e comprime o conteúdo vertical quando a janela é baixa.
    constexpr float contentStart = 80.0f;
    constexpr float contentEnd = 492.0f;
    const float availableContent = std::max(1.0f, panelH - 64.0f - contentStart);
    const float layoutScale = std::clamp(availableContent / (contentEnd - contentStart),
                                         0.48f, 1.0f);
    const auto offset = [layoutScale](float value)
    {
        return value <= contentStart
            ? value
            : contentStart + (value - contentStart) * layoutScale;
    };

    sandbox_ui::drawPanel(ui, window, x, y, panelW, panelH, false);
    sandbox_ui::drawHeader(ui, innerX, y + 16.0f, innerW,
                           "EDITOR / PROPRIEDADES", "Luz",
                           "Fonte de iluminação da cena");

    sandbox_ui::drawSection(ui, innerX, y + offset(106.0f), innerW, "TIPO DE LUZ");
    {
        const float gap = 6.0f;
        const float buttonW = (innerW - gap * 2.0f) / 3.0f;
        const float buttonsY = y + offset(128.0f);

        if (sandbox_ui::button(ui, "Ponto", innerX, buttonsY,
                               buttonW, 32.0f * layoutScale,
                               light.type == LightType::Point ? ButtonTone::Selected : ButtonTone::Neutral))
            light.type = LightType::Point;
        if (sandbox_ui::button(ui, "Direcional", innerX + buttonW + gap, buttonsY,
                               buttonW, 32.0f * layoutScale,
                               light.type == LightType::Directional ? ButtonTone::Selected : ButtonTone::Neutral))
            light.type = LightType::Directional;
        if (sandbox_ui::button(ui, "Holofote", innerX + (buttonW + gap) * 2.0f, buttonsY,
                               buttonW, 32.0f * layoutScale,
                               light.type == LightType::Spot ? ButtonTone::Selected : ButtonTone::Neutral))
            light.type = LightType::Spot;
    }

    sandbox_ui::drawSection(ui, innerX, y + offset(178.0f), innerW, "TRANSFORMAÇÃO");
    drawVectorControls(ui, "POSIÇÃO", light.position,
                       innerX, y + offset(196.0f), innerW, layoutScale);
    drawVectorControls(ui, "ALVO", light.target,
                       innerX, y + offset(246.0f), innerW, layoutScale);

    sandbox_ui::drawSection(ui, innerX, y + offset(306.0f), innerW, "APARÊNCIA");
    sandbox_ui::drawText(ui, "Cor da luz", innerX, y + offset(339.0f),
                         0.42f, sandbox_ui::palette::text);
    {
        const float swatchW = 72.0f;
        const float swatchH = 22.0f;
        const float swatchX = x + panelW - 22.0f - swatchW;
        const float swatchY = y + offset(322.0f);
        ui.drawRect(swatchX - 1.0f, swatchY - 1.0f,
                    swatchW + 2.0f, swatchH + 2.0f,
                    sandbox_ui::palette::border.r,
                    sandbox_ui::palette::border.g,
                    sandbox_ui::palette::border.b);
        ui.drawRect(swatchX, swatchY, swatchW, swatchH,
                    light.color.r, light.color.g, light.color.b);
    }

    float colorSliderY = y + offset(350.0f);
    ui.slider("R", light.color.r, 0.0f, 1.0f,
              innerX, colorSliderY, innerW, 24.0f * layoutScale,
              0.82f, 0.30f, 0.26f, 0, 255.0f, "sandbox/light/red");
    colorSliderY += 26.0f * layoutScale;
    ui.slider("G", light.color.g, 0.0f, 1.0f,
              innerX, colorSliderY, innerW, 24.0f * layoutScale,
              0.34f, 0.67f, 0.36f, 0, 255.0f, "sandbox/light/green");
    colorSliderY += 26.0f * layoutScale;
    ui.slider("B", light.color.b, 0.0f, 1.0f,
              innerX, colorSliderY, innerW, 24.0f * layoutScale,
              0.31f, 0.49f, 0.81f, 0, 255.0f, "sandbox/light/blue");

    sandbox_ui::drawSection(ui, innerX, y + offset(438.0f), innerW, "INTENSIDADE");
    sandbox_ui::stepper(ui, "Valor", light.intensity, 0.25f,
                        0.0f, 10.0f, 2,
                        innerX, y + offset(458.0f), innerW, 34.0f * layoutScale);

    const float footerY = y + panelH - 44.0f;
    sandbox_ui::drawDivider(ui, innerX, footerY - 10.0f, innerW);
    if (sandbox_ui::button(ui, "Restaurar propriedades", innerX,
                           footerY, innerW, 30.0f, ButtonTone::Neutral))
    {
        const LightType type = light.type;
        light.resetProperties();
        light.type = type;
    }
}
