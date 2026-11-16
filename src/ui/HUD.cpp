#include "HUD.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
    constexpr float kTextScale = 0.4f;
    constexpr float kLineHeight = 22.0f;
    constexpr float kBaselineOffset = 16.0f; // da parte de cima da linha até a linha de base
    constexpr float kSectionGap = 10.0f;
    constexpr float kPadding = 12.0f;
    constexpr float kIndent = 10.0f;
    constexpr float kMargin = 16.0f;
    constexpr float kSwatchSize = 12.0f;
    constexpr float kMinPanelWidth = 260.0f;

    // Uma linha do painel completo: título de seção ou item.
    struct Row
    {
        std::string text;
        bool heading = false;
        bool hasSwatch = false; // mostra um quadrado de cor depois do texto
        glm::vec3 swatch{0.0f};
    };

    Row heading(const std::string &text)
    {
        Row row;
        row.text = text;
        row.heading = true;
        return row;
    }

    Row item(const std::string &text)
    {
        Row row;
        row.text = text;
        return row;
    }

    std::string vec3Text(const char *label, const glm::vec3 &v, int decimals)
    {
        char buffer[96];
        std::snprintf(buffer, sizeof(buffer), "%s %.*f  %.*f  %.*f",
                      label, decimals, v.x, decimals, v.y, decimals, v.z);
        return buffer;
    }

    std::string hexColor(const glm::vec3 &color)
    {
        const glm::vec3 c = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
        char buffer[16];
        std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X",
                      static_cast<int>(std::lround(c.r * 255.0f)),
                      static_cast<int>(std::lround(c.g * 255.0f)),
                      static_cast<int>(std::lround(c.b * 255.0f)));
        return buffer;
    }

    std::vector<Row> buildRows(const EnvironmentInfo &info, float fps, float frameTimeMs)
    {
        std::vector<Row> rows;
        char buffer[96];

        rows.push_back(heading("Desempenho"));
        std::snprintf(buffer, sizeof(buffer), "FPS: %.0f  (%.1f ms)", fps, frameTimeMs);
        rows.push_back(item(buffer));

        rows.push_back(heading("Câmera"));
        rows.push_back(item(vec3Text("Pos:", info.cameraPosition, 2)));

        rows.push_back(heading("Modo"));
        if (info.editing)
            rows.push_back(item("Edição (" + info.tool + ")"));
        else
            rows.push_back(item("Visualização"));
        rows.push_back(item("Exibindo: " + info.viewedModel));

        rows.push_back(heading("Luz"));
        rows.push_back(item("Tipo: " + info.lightType));
        rows.push_back(item(vec3Text("Pos:", info.lightPosition, 2)));
        Row color = item("Cor: " + hexColor(info.lightColor));
        color.hasSwatch = true;
        color.swatch = info.lightColor;
        rows.push_back(color);

        rows.push_back(heading("Cena"));
        std::snprintf(buffer, sizeof(buffer), "Objetos: %d / %d visíveis",
                      info.visibleObjectCount, info.objectCount);
        rows.push_back(item(buffer));
        std::snprintf(buffer, sizeof(buffer), "Faces: %zu", info.faceCount);
        rows.push_back(item(buffer));

        rows.push_back(heading("Seleção"));
        if (info.lightSelected)
        {
            rows.push_back(item("Luz"));
        }
        else if (info.hasSelection)
        {
            rows.push_back(item(info.selectedName.empty() ? "(sem nome)" : info.selectedName));
            rows.push_back(item(vec3Text("Pos:", info.selectedPosition, 2)));
            rows.push_back(item(vec3Text("Rot:", info.selectedRotation, 1)));
            rows.push_back(item(vec3Text("Esc:", info.selectedScale, 2)));
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

    // Atualiza o FPS a cada meio segundo para reduzir oscilações na leitura.
    timeAccumulator += deltaTime;
    ++frameCount;

    if (timeAccumulator >= 0.5f)
    {
        fps = frameCount / timeAccumulator;
        frameTimeMs = 1000.0f * timeAccumulator / frameCount;
        timeAccumulator = 0.0f;
        frameCount = 0;
    }
}

void HUD::draw(UI &ui, const EnvironmentInfo &info)
{
    if (visible)
        drawPanel(ui, info);
}

void HUD::drawPanel(UI &ui, const EnvironmentInfo &info) const
{
    const std::vector<Row> rows = buildRows(info, fps, frameTimeMs);

    // Primeira passada: mede o painel para que ele se ajuste ao conteúdo.
    float width = kMinPanelWidth;
    float height = 2.0f * kPadding;
    for (std::size_t i = 0; i < rows.size(); ++i)
    {
        const Row &row = rows[i];

        float rowWidth = ui.measureText(row.text, kTextScale) + 2.0f * kPadding;
        if (!row.heading)
            rowWidth += kIndent;
        if (row.hasSwatch)
            rowWidth += 8.0f + kSwatchSize;
        width = std::max(width, rowWidth);

        if (row.heading && i > 0)
            height += kSectionGap;
        height += kLineHeight;
    }

    ui.drawRect(kMargin, kMargin, width, height, 0.04f, 0.06f, 0.10f, 0.75f);
    ui.drawRect(kMargin, kMargin, width, 3.0f, 0.25f, 0.55f, 0.95f, 1.0f);

    // Segunda passada: desenha as linhas.
    float top = kMargin + kPadding;
    for (std::size_t i = 0; i < rows.size(); ++i)
    {
        const Row &row = rows[i];

        if (row.heading && i > 0)
            top += kSectionGap;

        const float baseline = top + kBaselineOffset;
        const float x = kMargin + kPadding + (row.heading ? 0.0f : kIndent);

        if (row.heading)
            ui.drawText(row.text, x, baseline, kTextScale, 0.45f, 0.70f, 1.0f);
        else
            ui.drawText(row.text, x, baseline, kTextScale, 1.0f, 1.0f, 1.0f);

        if (row.hasSwatch)
        {
            const float swatchX = x + ui.measureText(row.text, kTextScale) + 8.0f;
            const float swatchY = baseline - kSwatchSize + 1.0f;
            ui.drawRect(swatchX - 1.0f, swatchY - 1.0f, kSwatchSize + 2.0f, kSwatchSize + 2.0f,
                        0.85f, 0.85f, 0.90f, 1.0f);
            ui.drawRect(swatchX, swatchY, kSwatchSize, kSwatchSize,
                        row.swatch.r, row.swatch.g, row.swatch.b, 1.0f);
        }

        top += kLineHeight;
    }
}
