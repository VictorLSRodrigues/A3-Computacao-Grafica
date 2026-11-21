#pragma once

#include <glm/glm.hpp>

// Identidade visual compartilhada: azul-marinho discreto, com destaque azul.
// Cores em RGB, de 0 a 1. Medidas em pixels.
namespace theme
{
    inline const glm::vec3 accent{0.27f, 0.53f, 0.82f};
    inline const glm::vec3 accentSoft{0.13f, 0.27f, 0.43f};
    inline const glm::vec3 panelBackground{0.055f, 0.075f, 0.105f};
    inline const glm::vec3 panelBorder{0.17f, 0.23f, 0.32f};

    // Botões e estados.
    inline const glm::vec3 buttonNeutral{0.105f, 0.145f, 0.205f};
    inline const glm::vec3 buttonPrimary{0.14f, 0.29f, 0.46f};
    inline const glm::vec3 buttonDanger{0.39f, 0.17f, 0.22f};
    inline const glm::vec3 buttonDisabled{0.085f, 0.11f, 0.15f};
    inline const glm::vec3 buttonBorder{0.22f, 0.30f, 0.41f};
    inline const glm::vec3 iconButtonBackground{0.09f, 0.13f, 0.18f};

    // Elementos de entrada.
    inline const glm::vec3 fieldBackground{0.035f, 0.05f, 0.075f};
    inline const glm::vec3 sliderTrack{0.16f, 0.22f, 0.30f};
    inline const glm::vec3 sliderHandle{0.78f, 0.85f, 0.94f};
    inline const glm::vec3 previewBorder{0.72f, 0.79f, 0.88f};

    // Texto e sobreposições.
    inline const glm::vec3 text{0.90f, 0.93f, 0.97f};
    inline const glm::vec3 textMuted{0.61f, 0.69f, 0.79f};
    inline const glm::vec3 tooltipBackground{0.025f, 0.04f, 0.065f};

    constexpr float buttonHoverFactor = 1.22f;
    constexpr float buttonPressedFactor = 0.82f;
    constexpr float buttonBorderOpacity = 0.80f;
    constexpr float buttonHoverMarkHeight = 2.0f;
    constexpr float iconHoverFactor = 1.20f;
    constexpr float iconPressedFactor = 0.82f;
    constexpr float disabledTextOpacity = 0.42f;
    inline const glm::vec3 iconHoverOffset{0.025f, 0.035f, 0.055f};

    // Opacidades.
    constexpr float panelOpacity = 0.97f;
    constexpr float dimOpacity = 0.58f;
    constexpr float hudOpacity = 0.86f;

    // Medidas reutilizadas pelos componentes.
    namespace metrics
    {
        constexpr float accentBarHeight = 3.0f;
        constexpr float titleScale = 0.85f;
        constexpr float sectionHeight = 28.0f;
        constexpr float sectionTextScale = 0.43f;
        constexpr float sectionBaselineOffset = 18.0f;
        constexpr float sectionLineHeight = 1.0f;
        constexpr float sectionLineGap = 8.0f;
        constexpr float buttonTextScale = 0.60f;
        constexpr float labelTextScale = 0.50f;
        constexpr float tooltipTextScale = 0.45f;
        constexpr float textCapHeightFactor = 0.62f;
        constexpr float selectedIndicatorHeight = 3.0f;
        constexpr float sliderTrackHeightFactor = 0.35f;

        constexpr float stepperValueWidth = 90.0f;
        constexpr float stepperButtonWidth = 34.0f;
        constexpr float stepperControlHeight = 34.0f;
        constexpr float stepperControlGap = 4.0f;
        constexpr float sliderLabelWidth = 40.0f;
        constexpr float sliderValueWidth = 66.0f;
        constexpr float sliderValueInset = 4.0f;
        constexpr float sliderHandleWidth = 10.0f;
        constexpr float sliderHandleHeight = 18.0f;
        constexpr float sliderTrackHeight = 4.0f;

        constexpr float headerEyebrowScale = 0.36f;
        constexpr float headerEyebrowOffset = 13.0f;
        constexpr float headerTitleOffset = 43.0f;
        constexpr float headerMarkWidth = 12.0f;
        constexpr float headerMarkHeight = 2.0f;

        constexpr float iconMaxSize = 25.0f;
        constexpr float iconScale = 0.58f;
        constexpr float iconTooltipMinWidth = 54.0f;
        constexpr float iconTooltipPadding = 14.0f;
        constexpr float iconTooltipGap = 6.0f;
        constexpr float iconTooltipHeight = 26.0f;

        constexpr float colorPreviewHeight = 36.0f;
        constexpr float colorSliderHeight = 28.0f;
        constexpr float colorPickerGap = 8.0f;
    }
}
