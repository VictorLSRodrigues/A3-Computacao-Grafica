#pragma once

#include <functional>
#include <string>

#include <glm/glm.hpp>

#include "UI.h"
#include "theme.h"
#include "window.h"

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

// Desenha um painel com título no centro da tela. A altura dele se ajusta ao conteúdo.
//
// `content` monta as linhas usando o Layout e é chamado DUAS vezes por quadro:
// a primeira só mede (nada é desenhado nem reage ao mouse) e a segunda desenha.
// Por isso, não coloque dentro dele nada que deva acontecer uma vez só por quadro;
// ações ligadas a botões, como trocar de página, funcionam normalmente.
inline void drawCenteredPanel(const Window &window, UI &ui, const std::string &title,
                              const PanelStyle &style,
                              const std::function<void(Layout &)> &content)
{
    const float contentWidth = style.width - 2.0f * style.margin;

    Layout measure(ui, 0.0f, 0.0f, contentWidth, style.rowHeight, style.gap, true);
    content(measure);

    const float panelHeight = style.margin + style.titleHeight + measure.height() + style.margin;

    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());
    const float px = (screenW - style.width) / 2.0f;
    const float py = (screenH - panelHeight) / 2.0f;

    if (style.dimBackground)
        ui.drawRect(0.0f, 0.0f, screenW, screenH, 0.0f, 0.0f, 0.0f, theme::dimOpacity);

    const glm::vec3 &bg = theme::panelBackground;
    const glm::vec3 &accent = theme::accent;
    ui.drawRect(px, py, style.width, panelHeight, bg.r, bg.g, bg.b, theme::panelOpacity);
    ui.drawRect(px, py, style.width, theme::accentBarHeight, accent.r, accent.g, accent.b);
    ui.drawTextCentered(title, px, py + style.margin, style.width, style.titleHeight,
                        theme::titleScale, theme::text.r, theme::text.g, theme::text.b);

    Layout layout(ui, px + style.margin, py + style.margin + style.titleHeight,
                  contentWidth, style.rowHeight, style.gap, false);
    content(layout);
}
