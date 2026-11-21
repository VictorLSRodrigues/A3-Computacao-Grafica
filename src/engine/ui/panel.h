#pragma once

#include <functional>
#include <string>
#include <algorithm>

#include <glm/glm.hpp>

#include "UI.h"
#include "theme.h"
#include "engine/platform/window.h"

enum class ButtonStyle
{
    Neutral,
    Primary,
    Danger
};

enum class LayoutDirection
{
    Vertical,
    Horizontal
};

// Organiza componentes em coluna. Também permite abrir uma linha horizontal
// dentro da coluna, informando a largura de cada componente dessa linha.
// No modo de medição nada é desenhado nem reage ao mouse.
class Layout
{
public:
    Layout(UI &ui, float x, float y, float width, float rowHeight, float gap, bool measuring,
           LayoutDirection direction = LayoutDirection::Vertical)
        : ui(ui), x(x), top(y), cursor(direction == LayoutDirection::Vertical ? y : x),
          width(width), rowHeight(rowHeight), gap(gap), measuring(measuring), direction(direction)
    {
    }

    // Retorna true quando o botão é clicado neste quadro.
    // Em uma linha horizontal, informe itemWidth para cada componente.
    bool button(const std::string &label, ButtonStyle style = ButtonStyle::Neutral,
                float itemWidth = -1.0f)
    {
        const Slot slot = next(rowHeight, itemWidth);
        if (measuring)
            return false;

        const glm::vec3 c = buttonColor(style);
        return ui.button(label, slot.x, slot.y, slot.width, slot.height, c.r, c.g, c.b);
    }

    // Barra arrastável. IDs opcionais evitam colisões quando há sliders com rótulos iguais.
    bool slider(const std::string &label, float &value, float minValue, float maxValue,
                int decimals = 2, float displayScale = 1.0f,
                const std::string &id = {}, float itemWidth = -1.0f)
    {
        const Slot slot = next(rowHeight, itemWidth);
        if (measuring)
            return false;

        return ui.slider(label, value, minValue, maxValue,
                         slot.x, slot.y, slot.width, slot.height,
                         theme::accent.r, theme::accent.g, theme::accent.b,
                         decimals, displayScale, id);
    }

    // Retorna true se o valor mudar neste quadro.
    bool stepper(const std::string &label, float &value, float step,
                 float minValue, float maxValue, int decimals = 1,
                 float itemWidth = -1.0f)
    {
        const Slot slot = next(rowHeight, itemWidth);
        if (measuring)
            return false;

        return ui.stepper(label, value, step, minValue, maxValue,
                          slot.x, slot.y, slot.width, slot.height, decimals);
    }

    // Retorna true se a cor mudar neste quadro. Ocupa mais que uma linha.
    bool colorPicker(const std::string &label, glm::vec3 &rgb,
                     const std::string &id = {})
    {
        const Slot slot = next(UI::colorPickerHeight());
        if (measuring)
            return false;

        return ui.colorPicker(label, rgb, slot.x, slot.y, slot.width, id);
    }

    // Título de seção com divisor. Ocupa espaço no layout e pode ser reutilizado
    // por painéis de diferentes aplicações.
    void section(const std::string &label)
    {
        if (direction != LayoutDirection::Vertical)
            return;

        const Slot slot = next(theme::metrics::sectionHeight);
        if (measuring)
            return;

        const float scale = theme::metrics::sectionTextScale;
        const float textWidth = ui.measureText(label, scale);
        ui.drawText(label, slot.x,
                    slot.y + theme::metrics::sectionBaselineOffset,
                    scale, theme::accent.r, theme::accent.g, theme::accent.b);

        const float lineStart = slot.x + textWidth + theme::metrics::sectionLineGap;
        const float lineEnd = slot.x + slot.width;
        if (lineEnd > lineStart + 4.0f)
        {
            const float lineY = slot.y + slot.height - 5.0f;
            ui.drawLine(lineStart, lineY, lineEnd, lineY,
                        theme::metrics::sectionLineHeight,
                        theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b, 0.9f);
        }
    }

    // Abre uma linha horizontal dentro do layout vertical.
    // Use larguras explícitas nos componentes quando houver mais de um na linha.
    void horizontal(const std::function<void(Layout &)> &content, float height = -1.0f)
    {
        if (direction != LayoutDirection::Vertical)
        {
            content(*this);
            return;
        }

        const float lineHeight = height > 0.0f ? height : rowHeight;
        const Slot slot = next(lineHeight);
        Layout line(ui, slot.x, slot.y, slot.width, lineHeight, gap, measuring,
                    LayoutDirection::Horizontal);
        content(line);
    }

    float availableWidth() const { return width; }
    float itemSpacing() const { return gap; }
    float itemHeight() const { return rowHeight; }

    // Altura ocupada até agora, sem o espaço que sobra depois do último componente.
    float height() const
    {
        if (!hasContent)
            return 0.0f;

        if (direction == LayoutDirection::Horizontal)
            return rowHeight;

        return cursor - top - gap;
    }

private:
    struct Slot
    {
        float x;
        float y;
        float width;
        float height;
    };

    UI &ui;
    float x;
    float top;
    float cursor;
    float width;
    float rowHeight;
    float gap;
    bool measuring;
    LayoutDirection direction;
    bool hasContent = false;

    // Reserva espaço no eixo principal e retorna a área do componente.
    Slot next(float height, float itemWidth = -1.0f)
    {
        hasContent = true;

        if (direction == LayoutDirection::Horizontal)
        {
            const float remaining = std::max(0.0f, x + width - cursor);
            const float slotWidth = itemWidth > 0.0f
                ? std::min(itemWidth, remaining)
                : remaining;

            const Slot slot{cursor, top, slotWidth, height};
            cursor += slotWidth + gap;
            return slot;
        }

        const float slotWidth = itemWidth > 0.0f ? std::min(itemWidth, width) : width;
        const Slot slot{x, cursor, slotWidth, height};
        cursor += height + gap;
        return slot;
    }

    static glm::vec3 buttonColor(ButtonStyle style)
    {
        switch (style)
        {
        case ButtonStyle::Primary:
            return theme::buttonPrimary;
        case ButtonStyle::Danger:
            return theme::buttonDanger;
        case ButtonStyle::Neutral:
        default:
            return theme::buttonNeutral;
        }
    }
};

struct PanelStyle
{
    float width = 560.0f;
    float margin = 30.0f;
    float titleHeight = 70.0f;
    float rowHeight = 48.0f;
    float gap = 14.0f;
    bool dimBackground = true;
    // Cabeçalho opcional para painéis que precisam de uma identificação própria.
    // Sem eyebrow, o título mantém o alinhamento central tradicional.
    std::string eyebrow;
    bool titleLeftAligned = false;
};

enum class PanelPlacement
{
    Center,
    Right
};

// Desenha um painel com título. A altura dele se ajusta ao conteúdo.
// O conteúdo é medido antes do desenho; ações só são executadas na passagem final.
inline void drawPanel(const Window &window, UI &ui, const std::string &title,
                      const PanelStyle &style, PanelPlacement placement,
                      const std::function<void(Layout &)> &content)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());
    const float panelWidth = std::min(style.width, std::max(1.0f, screenW - 24.0f));
    const float verticalReserve = placement == PanelPlacement::Right ? 96.0f : 24.0f;
    const float maxPanelHeight = std::max(1.0f, screenH - verticalReserve);

    float scale = 1.0f;
    float panelHeight = 0.0f;
    float panelMargin = style.margin;
    float titleHeight = style.titleHeight;
    float rowHeight = style.rowHeight;
    float gap = style.gap;

    const auto measurePanel = [&](float density)
    {
        const float margin = std::max(12.0f, style.margin * density);
        const float title = std::max(44.0f, style.titleHeight * density);
        const float row = std::max(28.0f, style.rowHeight * density);
        const float rowGap = std::max(4.0f, style.gap * density);
        const float contentWidth = panelWidth - 2.0f * margin;

        Layout measure(ui, 0.0f, 0.0f, contentWidth, row, rowGap, true);
        content(measure);

        return margin * 2.0f + title + measure.height();
    };

    panelHeight = measurePanel(scale);
    if (panelHeight > maxPanelHeight)
    {
        float low = 0.35f;
        float high = 1.0f;
        for (int i = 0; i < 12; ++i)
        {
            const float middle = (low + high) * 0.5f;
            if (measurePanel(middle) <= maxPanelHeight)
                low = middle;
            else
                high = middle;
        }
        scale = low;
    }

    panelMargin = std::max(12.0f, style.margin * scale);
    titleHeight = std::max(44.0f, style.titleHeight * scale);
    rowHeight = std::max(28.0f, style.rowHeight * scale);
    gap = std::max(4.0f, style.gap * scale);
    const float contentWidth = panelWidth - 2.0f * panelMargin;
    Layout measure(ui, 0.0f, 0.0f, contentWidth, rowHeight, gap, true);
    content(measure);
    panelHeight = panelMargin * 2.0f + titleHeight + measure.height();

    const float px = placement == PanelPlacement::Center
        ? (screenW - panelWidth) / 2.0f
        : screenW - panelWidth - 16.0f;
    const float py = placement == PanelPlacement::Center
        ? std::max(12.0f, (screenH - panelHeight) / 2.0f)
        : std::min(80.0f, std::max(12.0f, screenH - panelHeight - 12.0f));
    ui.addRegion(px, py, panelWidth, panelHeight);

    if (style.dimBackground)
        ui.drawRect(0.0f, 0.0f, screenW, screenH, 0.0f, 0.0f, 0.0f, theme::dimOpacity);

    const glm::vec3 &bg = theme::panelBackground;
    const glm::vec3 &accent = theme::accent;
    ui.drawRect(px, py, panelWidth, panelHeight, bg.r, bg.g, bg.b, theme::panelOpacity);
    ui.drawRect(px, py, 1.0f, panelHeight, theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);
    ui.drawRect(px + panelWidth - 1.0f, py, 1.0f, panelHeight,
                theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);
    ui.drawRect(px, py + panelHeight - 1.0f, panelWidth, 1.0f,
                theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);
    if (!style.eyebrow.empty())
    {
        // Cabeçalho identificado: uma pequena marca geométrica e uma linha de contexto.
        // Mantém o restante do sistema de painéis inalterado quando o recurso não é usado.
        const float headerX = px + panelMargin;
        const float headerY = py + panelMargin;
        const float markX = headerX;
        const float markY = headerY + theme::metrics::headerEyebrowOffset - 5.0f;

        ui.drawRect(px, py, panelWidth, 1.0f,
                    theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);
        ui.drawLine(markX, markY + 4.0f,
                    markX + theme::metrics::headerMarkWidth, markY,
                    theme::metrics::headerMarkHeight,
                    accent.r, accent.g, accent.b);
        ui.drawLine(markX + 2.0f, markY + 8.0f,
                    markX + theme::metrics::headerMarkWidth - 2.0f, markY + 4.0f,
                    theme::metrics::headerMarkHeight,
                    accent.r, accent.g, accent.b, 0.65f);

        ui.drawText(style.eyebrow,
                    headerX + theme::metrics::headerMarkWidth + 8.0f,
                    headerY + theme::metrics::headerEyebrowOffset,
                    theme::metrics::headerEyebrowScale,
                    theme::textMuted.r, theme::textMuted.g, theme::textMuted.b);

        if (style.titleLeftAligned)
        {
            ui.drawText(title, headerX,
                        headerY + theme::metrics::headerTitleOffset,
                        theme::metrics::titleScale,
                        theme::text.r, theme::text.g, theme::text.b);
        }
        else
        {
            ui.drawTextCentered(title, px, py + panelMargin, panelWidth, titleHeight,
                                theme::metrics::titleScale,
                                theme::text.r, theme::text.g, theme::text.b);
        }

        const float separatorY = py + panelMargin + titleHeight - 1.0f;
        ui.drawLine(px + panelMargin, separatorY,
                    px + panelWidth - panelMargin, separatorY,
                    1.0f, theme::panelBorder.r, theme::panelBorder.g, theme::panelBorder.b);
    }
    else
    {
        ui.drawRect(px, py, panelWidth, theme::metrics::accentBarHeight,
                    accent.r, accent.g, accent.b);
        ui.drawTextCentered(title, px, py + panelMargin, panelWidth, titleHeight,
                            theme::metrics::titleScale,
                            theme::text.r, theme::text.g, theme::text.b);
    }

    Layout layout(ui, px + panelMargin, py + panelMargin + titleHeight,
                  contentWidth, rowHeight, gap, false);
    content(layout);
}

inline void drawCenteredPanel(const Window &window, UI &ui, const std::string &title,
                              const PanelStyle &style,
                              const std::function<void(Layout &)> &content)
{
    drawPanel(window, ui, title, style, PanelPlacement::Center, content);
}

inline void drawRightPanel(const Window &window, UI &ui, const std::string &title,
                           const PanelStyle &style,
                           const std::function<void(Layout &)> &content)
{
    drawPanel(window, ui, title, style, PanelPlacement::Right, content);
}
