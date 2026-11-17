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

// Posiciona os componentes em coluna, de cima para baixo, sem que quem usa precise
// calcular coordenadas: cada chamada ocupa uma linha e o próximo componente vem abaixo.
//
// No modo de medição nada é desenhado nem reage ao mouse; só se soma a altura ocupada.
class Layout
{
public:
    Layout(UI &ui, float x, float y, float width, float rowHeight, float gap, bool measuring)
        : ui(ui), x(x), top(y), cursor(y), width(width),
          rowHeight(rowHeight), gap(gap), measuring(measuring)
    {
    }

    // Retorna true quando o botão é clicado neste quadro.
    bool button(const std::string &label, ButtonStyle style = ButtonStyle::Neutral)
    {
        const float y = next(rowHeight);
        if (measuring)
            return false;

        const glm::vec3 c = buttonColor(style);
        return ui.button(label, x, y, width, rowHeight, c.r, c.g, c.b);
    }

    // Retorna true se o valor mudar neste quadro.
    bool stepper(const std::string &label, float &value, float step,
                 float minValue, float maxValue, int decimals = 1)
    {
        const float y = next(rowHeight);
        if (measuring)
            return false;

        return ui.stepper(label, value, step, minValue, maxValue, x, y, width, rowHeight, decimals);
    }

    // Retorna true se a cor mudar neste quadro. Ocupa mais que uma linha.
    bool colorPicker(const std::string &label, glm::vec3 &rgb)
    {
        const float y = next(UI::colorPickerHeight());
        if (measuring)
            return false;

        return ui.colorPicker(label, rgb, x, y, width);
    }

    // Altura ocupada até agora, sem o espaço que sobra depois do último componente.
    float height() const { return cursor > top ? cursor - top - gap : 0.0f; }

private:
    UI &ui;
    float x;
    float top;
    float cursor;
    float width;
    float rowHeight;
    float gap;
    bool measuring;

    // Reserva uma linha de altura h e retorna o y onde ela começa.
    float next(float h)
    {
        const float y = cursor;
        cursor += h + gap;
        return y;
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
    float margin = 30.0f;      // da borda do painel até o conteúdo
    float titleHeight = 70.0f; // altura reservada ao título
    float rowHeight = 48.0f;
    float gap = 14.0f; // entre as linhas
    bool dimBackground = true; // escurece a tela atrás (painel modal, como o menu)
};

enum class PanelPlacement
{
    Center,
    Right
};

// Desenha um painel com título. A altura dele se ajusta ao conteúdo.
//
// `content` pode ser medido mais de uma vez para ajustar o painel à janela;
// as passagens de medição não desenham nem reagem ao mouse.
// Por isso, não coloque dentro dele nada que deva acontecer uma vez só por quadro;
// ações ligadas a botões, como trocar de página, funcionam normalmente.
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
    ui.drawRect(px, py, panelWidth, theme::accentBarHeight, accent.r, accent.g, accent.b);
    ui.drawTextCentered(title, px, py + panelMargin, panelWidth, titleHeight,
                        theme::titleScale, theme::text.r, theme::text.g, theme::text.b);

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
