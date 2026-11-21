#pragma once

#include <algorithm>
#include <cstdio>
#include <string>

#include <glm/glm.hpp>

#include "engine/platform/window.h"
#include "engine/ui/UI.h"
#include "engine/ui/theme.h"

// Helpers de apresentação do Sandbox.
// A paleta vem do Theme compartilhado para manter coerência com o Editor e os controles da UI.
namespace sandbox_ui
{
    namespace palette
    {
        inline const glm::vec3 backdrop = theme::fieldBackground;
        inline const glm::vec3 panel = theme::panelBackground;
        inline const glm::vec3 surface = theme::buttonNeutral;
        inline const glm::vec3 surfaceRaised = theme::buttonPrimary;
        inline const glm::vec3 border = theme::panelBorder;
        inline const glm::vec3 accent = theme::accent;
        inline const glm::vec3 accentDim = theme::accentSoft;
        inline const glm::vec3 text = theme::text;
        inline const glm::vec3 muted = theme::textMuted;
        inline const glm::vec3 subtle{0.38f, 0.46f, 0.49f};
        inline const glm::vec3 danger = theme::buttonDanger;
        inline const glm::vec3 positive{0.25f, 0.48f, 0.39f};
    }

    enum class ButtonTone
    {
        Neutral,
        Primary,
        Danger,
        Selected
    };

    inline glm::vec3 buttonColor(ButtonTone tone)
    {
        switch (tone)
        {
        case ButtonTone::Primary:
            return theme::buttonPrimary;
        case ButtonTone::Danger:
            return theme::buttonDanger;
        case ButtonTone::Selected:
            return theme::accentSoft;
        case ButtonTone::Neutral:
        default:
            return theme::buttonNeutral;
        }
    }

    inline void drawText(UI &ui, const std::string &value, float x, float baseline,
                         float scale = 0.46f,
                         const glm::vec3 &color = palette::text,
                         float alpha = 1.0f)
    {
        ui.drawText(value, x, baseline, scale,
                    color.r, color.g, color.b, alpha);
    }

    inline void drawCenteredText(UI &ui, const std::string &value,
                                 float x, float y, float width, float height,
                                 float scale = 0.46f,
                                 const glm::vec3 &color = palette::text)
    {
        ui.drawTextCentered(value, x, y, width, height, scale,
                            color.r, color.g, color.b);
    }

    // Ajusta textos variáveis à largura disponível sem deixar caracteres escaparem do painel.
    inline std::string fitText(UI &ui, const std::string &value,
                               float maxWidth, float scale)
    {
        if (maxWidth <= 0.0f)
            return {};
        if (ui.measureText(value, scale) <= maxWidth)
            return value;

        std::string fitted = value;
        constexpr const char *ellipsis = "...";
        while (!fitted.empty())
        {
            std::size_t cut = fitted.size() - 1;
            while (cut > 0 &&
                   (static_cast<unsigned char>(fitted[cut]) & 0xC0u) == 0x80u)
                --cut;
            fitted.erase(cut);
            if (ui.measureText(fitted + ellipsis, scale) <= maxWidth)
                return fitted + ellipsis;
        }
        return ui.measureText(ellipsis, scale) <= maxWidth ? ellipsis : std::string{};
    }

    inline void drawTextFit(UI &ui, const std::string &value,
                            float x, float baseline, float maxWidth,
                            float scale = 0.46f,
                            const glm::vec3 &color = palette::text,
                            float alpha = 1.0f)
    {
        drawText(ui, fitText(ui, value, maxWidth, scale),
                 x, baseline, scale, color, alpha);
    }

    inline bool button(UI &ui, const std::string &label,
                       float x, float y, float width, float height,
                       ButtonTone tone = ButtonTone::Neutral,
                       bool enabled = true)
    {
        const glm::vec3 color = buttonColor(tone);
        return ui.button(label, x, y, width, height,
                         color.r, color.g, color.b, enabled);
    }

    inline void drawPanel(UI &ui, const Window &window,
                          float x, float y, float width, float height,
                          bool dimBackground = false)
    {
        ui.addRegion(x, y, width, height);

        if (dimBackground)
        {
            ui.drawRect(0.0f, 0.0f,
                        static_cast<float>(window.getWidth()),
                        static_cast<float>(window.getHeight()),
                        0.015f, 0.025f, 0.035f, theme::dimOpacity);
        }

        ui.drawRect(x, y, width, height,
                    palette::panel.r, palette::panel.g, palette::panel.b,
                    theme::panelOpacity);
        ui.drawRect(x, y, width, 1.0f,
                    palette::border.r, palette::border.g, palette::border.b);
        ui.drawRect(x, y + height - 1.0f, width, 1.0f,
                    palette::border.r, palette::border.g, palette::border.b);
        ui.drawRect(x, y, 1.0f, height,
                    palette::border.r, palette::border.g, palette::border.b);
        ui.drawRect(x + width - 1.0f, y, 1.0f, height,
                    palette::border.r, palette::border.g, palette::border.b);
    }

    inline void drawHeader(UI &ui, float x, float y, float width,
                           const std::string &eyebrow,
                           const std::string &title,
                           const std::string &hint = {})
    {
        drawTextFit(ui, eyebrow, x, y + 12.0f, width, 0.34f, palette::muted);
        drawTextFit(ui, title, x, y + 43.0f, width, 0.72f, palette::text);
        if (!hint.empty())
            drawTextFit(ui, hint, x, y + 65.0f, width, 0.39f, palette::muted);

        ui.drawLine(x, y + 80.0f, x + width, y + 80.0f,
                    1.0f, palette::border.r, palette::border.g, palette::border.b);
    }

    inline void drawSection(UI &ui, float x, float y, float width,
                            const std::string &label)
    {
        const std::string visibleLabel = fitText(ui, label, std::max(0.0f, width - 14.0f), 0.39f);
        drawText(ui, visibleLabel, x, y + 13.0f, 0.39f, palette::muted);
        const float textEnd = x + ui.measureText(visibleLabel, 0.39f) + 10.0f;
        if (textEnd < x + width)
            ui.drawLine(textEnd, y + 9.0f, x + width, y + 9.0f,
                        1.0f, palette::border.r, palette::border.g, palette::border.b);
    }

    inline void drawDivider(UI &ui, float x, float y, float width, float alpha = 0.75f)
    {
        ui.drawLine(x, y, x + width, y, 1.0f,
                    palette::border.r, palette::border.g, palette::border.b, alpha);
    }

    inline bool stepper(UI &ui, const std::string &label, float &value,
                        float step, float minimum, float maximum, int decimals,
                        float x, float y, float width, float height)
    {
        constexpr float buttonWidth = 30.0f;
        constexpr float valueWidth = 76.0f;
        constexpr float gap = 4.0f;
        const float controlsWidth = buttonWidth * 2.0f + valueWidth + gap * 2.0f;
        const bool stacked = width - controlsWidth - 14.0f < ui.measureText(label, 0.43f);
        const float controlHeight = stacked ? std::max(26.0f, height - 20.0f) : height;
        const float controlY = stacked ? y + 19.0f : y;
        const float controlsX = x + std::max(0.0f, width - controlsWidth);
        const float oldValue = value;

        if (stacked)
            drawTextFit(ui, label, x, y + 14.0f, width, 0.38f, palette::text);
        else
            drawTextFit(ui, label, x, y + height * 0.63f,
                        std::max(0.0f, controlsX - x - 12.0f), 0.43f, palette::text);

        if (button(ui, "−", controlsX, controlY,
                   buttonWidth, controlHeight, ButtonTone::Neutral))
            value -= step;

        const float valueX = controlsX + buttonWidth + gap;
        ui.drawRect(valueX, controlY, valueWidth, controlHeight,
                    palette::backdrop.r, palette::backdrop.g, palette::backdrop.b);
        ui.drawRect(valueX, controlY, valueWidth, 1.0f,
                    palette::border.r, palette::border.g, palette::border.b);
        ui.drawRect(valueX, controlY + controlHeight - 1.0f, valueWidth, 1.0f,
                    palette::border.r, palette::border.g, palette::border.b);

        if (button(ui, "+", valueX + valueWidth + gap, controlY,
                   buttonWidth, controlHeight, ButtonTone::Neutral))
            value += step;

        value = std::clamp(value, minimum, maximum);
        char number[40];
        std::snprintf(number, sizeof(number), "%.*f", decimals, value);
        drawCenteredText(ui, number, valueX, controlY,
                         valueWidth, controlHeight, 0.43f, palette::text);
        return value != oldValue;
    }

    inline bool axisStepper(UI &ui, char axis, float &value,
                            float step, float minimum, float maximum,
                            float x, float y, float width, float height)
    {
        constexpr float axisWidth = 12.0f;
        constexpr float buttonWidth = 22.0f;
        constexpr float valueWidth = 50.0f;
        constexpr float gap = 2.0f;
        const float controlWidth = axisWidth + buttonWidth * 2.0f + valueWidth + gap * 2.0f;
        const float left = x + std::max(0.0f, (width - controlWidth) * 0.5f);
        const float oldValue = value;

        const char axisLabel[2] = {axis, '\0'};
        drawText(ui, axisLabel, left, y + height * 0.67f,
                 0.37f, palette::muted);

        const float minusX = left + axisWidth + 2.0f;
        if (button(ui, "−", minusX, y, buttonWidth, height, ButtonTone::Neutral))
            value -= step;

        const float valueX = minusX + buttonWidth + gap;
        ui.drawRect(valueX, y, valueWidth, height,
                    palette::backdrop.r, palette::backdrop.g, palette::backdrop.b);
        ui.drawRect(valueX, y, valueWidth, 1.0f,
                    palette::border.r, palette::border.g, palette::border.b);
        ui.drawRect(valueX, y + height - 1.0f, valueWidth, 1.0f,
                    palette::border.r, palette::border.g, palette::border.b);

        if (button(ui, "+", valueX + valueWidth + gap, y,
                   buttonWidth, height, ButtonTone::Neutral))
            value += step;

        value = std::clamp(value, minimum, maximum);
        char number[24];
        std::snprintf(number, sizeof(number), "%.2f", value);
        drawCenteredText(ui, number, valueX, y, valueWidth, height,
                         0.30f, palette::text);
        return value != oldValue;
    }

    inline bool toggleRow(UI &ui, float x, float y, float width, float height,
                          const std::string &title, const std::string &description,
                          bool enabled)
    {
        const float buttonWidth = std::min(76.0f, std::max(58.0f, width * 0.22f));
        const float buttonHeight = 28.0f;
        const float buttonX = x + width - buttonWidth;
        const float buttonY = y + (height - buttonHeight) * 0.5f;
        const float textWidth = std::max(0.0f, buttonX - x - 14.0f);

        drawTextFit(ui, title, x, y + 23.0f, textWidth, 0.46f, palette::text);
        drawTextFit(ui, description, x, y + 45.0f, textWidth, 0.35f, palette::muted);

        const bool clicked = button(ui, enabled ? "Ligado" : "Desligado",
                                    buttonX, buttonY, buttonWidth, buttonHeight,
                                    enabled ? ButtonTone::Primary : ButtonTone::Neutral);
        drawDivider(ui, x, y + height - 1.0f, width, 0.45f);
        return clicked;
    }

    inline bool slider(UI &ui, const std::string &label, float &value,
                       float minimum, float maximum, float x, float y,
                       float width, float height, const std::string &id,
                       int decimals = 2, float displayScale = 1.0f)
    {
        return ui.slider(label, value, minimum, maximum,
                         x, y, width, height,
                         palette::accent.r, palette::accent.g, palette::accent.b,
                         decimals, displayScale, id);
    }

    inline std::string vectorText(const glm::vec3 &value, int decimals = 2)
    {
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "%.*f   %.*f   %.*f",
                      decimals, value.x, decimals, value.y, decimals, value.z);
        return buffer;
    }
}
