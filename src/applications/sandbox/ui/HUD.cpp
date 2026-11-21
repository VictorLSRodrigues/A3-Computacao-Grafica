#include "HUD.h"

#include <algorithm>
#include <cstdio>
#include <vector>

#include "applications/sandbox/ui/sandbox_ui_style.h"

namespace
{
    constexpr float kMargin = 16.0f;
    constexpr float kPadding = 12.0f;
    constexpr float kTextScale = 0.38f;
    constexpr float kLineHeight = 19.0f;
    constexpr float kSectionGap = 6.0f;
    constexpr float kIndent = 10.0f;

    struct Row
    {
        std::string text;
        bool heading = false;
    };

    Row heading(const std::string &text)
    {
        return {text, true};
    }

    Row item(const std::string &text)
    {
        return {text, false};
    }

    std::string vectorText(const char *label, const glm::vec3 &value, int decimals)
    {
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "%s %.*f  %.*f  %.*f",
                      label, decimals, value.x, decimals, value.y, decimals, value.z);
        return buffer;
    }

    std::string colorText(const glm::vec3 &color)
    {
        const glm::vec3 clamped = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X",
                      static_cast<int>(clamped.r * 255.0f),
                      static_cast<int>(clamped.g * 255.0f),
                      static_cast<int>(clamped.b * 255.0f));
        return buffer;
    }

    std::vector<Row> buildRows(const EnvironmentInfo &info, float fps, float frameTimeMs)
    {
        std::vector<Row> rows;
        char buffer[128];

        rows.push_back(heading("Desempenho"));
        std::snprintf(buffer, sizeof(buffer), "%.0f FPS   ·   %.1f ms", fps, frameTimeMs);
        rows.push_back(item(buffer));

        rows.push_back(heading("Sessão"));
        rows.push_back(item(info.editing ? "Modo: edição / " + info.tool : "Modo: visualização"));
        rows.push_back(item("Exibindo: " + info.viewedModel));

        rows.push_back(heading("Câmera"));
        rows.push_back(item(vectorText("Posição:", info.cameraPosition, 2)));

        rows.push_back(heading("Luz"));
        rows.push_back(item("Tipo: " + info.lightType));
        rows.push_back(item(vectorText("Posição:", info.lightPosition, 2)));
        rows.push_back(item("Cor: " + colorText(info.lightColor)));

        rows.push_back(heading("Cena"));
        std::snprintf(buffer, sizeof(buffer), "Objetos: %d   ·   Visíveis: %d",
                      info.objectCount, info.visibleObjectCount);
        rows.push_back(item(buffer));
        std::snprintf(buffer, sizeof(buffer), "Faces: %zu", info.faceCount);
        rows.push_back(item(buffer));

        rows.push_back(heading("Seleção"));
        if (info.lightSelected)
        {
            rows.push_back(item("Luz selecionada"));
        }
        else if (info.hasSelection)
        {
            rows.push_back(item(info.selectedName.empty() ? "Objeto sem nome" : info.selectedName));
            rows.push_back(item(vectorText("Posição:", info.selectedPosition, 2)));
            rows.push_back(item(vectorText("Rotação:", info.selectedRotation, 1)));
            rows.push_back(item(vectorText("Escala:", info.selectedScale, 2)));
            if (info.hasFace)
            {
                std::snprintf(buffer, sizeof(buffer), "Face: %u", info.selectedFace);
                rows.push_back(item(buffer));
            }
        }
        else
        {
            rows.push_back(item("Nada selecionado"));
        }

        return rows;
    }
}

void HUD::update(const Window &window, float deltaTime)
{
    if (window.keyPressed(GLFW_KEY_F3))
        visible = !visible;

    timeAccumulator += deltaTime;
    ++frameCount;

    if (timeAccumulator >= 0.5f && frameCount > 0)
    {
        fps = frameCount / timeAccumulator;
        frameTimeMs = 1000.0f * timeAccumulator / frameCount;
        timeAccumulator = 0.0f;
        frameCount = 0;
    }
}

void HUD::draw(UI &ui, const Window &window, const EnvironmentInfo &info)
{
    if (visible)
        drawPanel(ui, window, info);
}

void HUD::drawPanel(UI &ui, const Window &window, const EnvironmentInfo &info) const
{
    const std::vector<Row> rows = buildRows(info, fps, frameTimeMs);
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());
    const float maxWidth = std::max(160.0f, screenW - 2.0f * kMargin);
    const float maxHeight = std::max(140.0f, screenH - 2.0f * kMargin);

    float textScale = kTextScale;
    float lineHeight = kLineHeight;
    float sectionGap = kSectionGap;
    float padding = kPadding;

    // Largura estável: nomes longos são truncados, em vez de fazer o HUD mudar de tamanho.
    const float panelW = std::min(360.0f, maxWidth);

    const auto measureHeight = [&](float rowHeight, float gap, float panelPadding)
    {
        float result = 2.0f * panelPadding + 8.0f;
        for (std::size_t i = 0; i < rows.size(); ++i)
        {
            if (rows[i].heading && i > 0)
                result += gap;
            result += rowHeight;
        }
        return result;
    };

    float panelH = measureHeight(lineHeight, sectionGap, padding);
    if (panelH > maxHeight)
    {
        const float ratio = std::clamp(maxHeight / panelH, 0.55f, 1.0f);
        textScale *= ratio;
        lineHeight = std::max(12.0f, lineHeight * ratio);
        sectionGap = std::max(3.0f, sectionGap * ratio);
        padding = std::max(8.0f, padding * ratio);
        panelH = measureHeight(lineHeight, sectionGap, padding);
    }

    const float x = kMargin;
    const float y = kMargin;
    ui.addRegion(x, y, panelW, panelH);
    ui.drawRect(x, y, panelW, panelH,
                sandbox_ui::palette::panel.r,
                sandbox_ui::palette::panel.g,
                sandbox_ui::palette::panel.b,
                0.90f);
    ui.drawRect(x, y, panelW, 1.0f,
                sandbox_ui::palette::border.r,
                sandbox_ui::palette::border.g,
                sandbox_ui::palette::border.b);
    ui.drawRect(x, y + panelH - 1.0f, panelW, 1.0f,
                sandbox_ui::palette::border.r,
                sandbox_ui::palette::border.g,
                sandbox_ui::palette::border.b);
    ui.drawRect(x, y, 2.0f, panelH,
                sandbox_ui::palette::accent.r,
                sandbox_ui::palette::accent.g,
                sandbox_ui::palette::accent.b);

    const float textWidth = std::max(0.0f, panelW - 2.0f * padding - kIndent);
    float top = y + padding + 4.0f;
    for (std::size_t i = 0; i < rows.size(); ++i)
    {
        const Row &row = rows[i];
        if (row.heading && i > 0)
            top += sectionGap;

        const float indent = row.heading ? 0.0f : kIndent;
        const float baseline = top + lineHeight * 0.74f;
        const float textX = x + padding + indent;
        const glm::vec3 &color = row.heading
            ? sandbox_ui::palette::accent
            : sandbox_ui::palette::muted;
        sandbox_ui::drawTextFit(ui, row.text, textX, baseline,
                                std::max(0.0f, textWidth - (row.heading ? 0.0f : indent)),
                                textScale, color);
        top += lineHeight;
    }
}
