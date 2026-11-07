#include "menu.h"

void Menu::update(const Window &window)
{
    if (!window.keyPressed(GLFW_KEY_ESCAPE))
        return;

    if (open && page != Page::Main)
    {
        page = Page::Main; // ESC nas opções retorna ao menu principal.
    }
    else
    {
        open = !open;
        page = Page::Main;
    }
}

void Menu::draw(const Window &window, UI &ui, Settings &settings)
{
    if (!open)
        return;

    if (page == Page::Options)
        drawOptions(window, ui, settings);
    else if (page == Page::Light)
        drawLight(window, ui, settings);
    else
        drawMain(window, ui);
}

void Menu::drawPanel(const Window &window, UI &ui, const char *title,
                     float panelW, float panelH, float &px, float &py)
{
    const float screenW = (float)window.getWidth();
    const float screenH = (float)window.getHeight();

    px = (screenW - panelW) / 2.0f;
    py = (screenH - panelH) / 2.0f;

    ui.drawRect(0.0f, 0.0f, screenW, screenH, 0.0f, 0.0f, 0.0f, 0.55f);
    ui.drawRect(px, py, panelW, panelH, 0.08f, 0.10f, 0.15f, 0.96f);
    ui.drawRect(px, py, panelW, 4.0f, 0.25f, 0.55f, 0.95f);
    ui.drawTextCentered(title, px, py + 30.0f, panelW, 70.0f, 0.85f, 1.0f, 1.0f, 1.0f);
}

void Menu::drawMain(const Window &window, UI &ui)
{
    const float panelW = 420.0f;
    const float margin = 30.0f;
    const float titleH = 70.0f;
    const float buttonW = panelW - 2.0f * margin;
    const float buttonH = 60.0f;
    const float gap = 16.0f;
    const int count = 4;
    const float panelH = margin + titleH + count * buttonH + (count - 1) * gap + margin;

    float px, py;
    drawPanel(window, ui, "Menu", panelW, panelH, px, py);

    const float bx = px + margin;
    float by = py + margin + titleH;

    if (ui.button("Continuar", bx, by, buttonW, buttonH, 0.20f, 0.45f, 0.85f))
        open = false;
    by += buttonH + gap;

    if (ui.button("Opções", bx, by, buttonW, buttonH, 0.25f, 0.30f, 0.40f))
        page = Page::Options;
    by += buttonH + gap;

    if (ui.button("Luz", bx, by, buttonW, buttonH, 0.25f, 0.30f, 0.40f))
        page = Page::Light;
    by += buttonH + gap;

    if (ui.button("Sair", bx, by, buttonW, buttonH, 0.70f, 0.15f, 0.15f))
        window.close();
}

void Menu::drawOptions(const Window &window, UI &ui, Settings &settings)
{
    const float panelW = 560.0f;
    const float margin = 30.0f;
    const float titleH = 70.0f;
    const float rowW = panelW - 2.0f * margin;
    const float rowH = 48.0f;
    const float gap = 14.0f;
    const int rows = 7; // Quatro ajustes, tipo de luz, restaurar e voltar.

    const float panelH = margin + titleH + rows * rowH + (rows - 1) * gap + margin;

    float px, py;
    drawPanel(window, ui, "Opções", panelW, panelH, px, py);

    const float x = px + margin;
    float y = py + margin + titleH;

    ui.stepper("Campo de visão", settings.fov, 5.0f, 30.0f, 110.0f, x, y, rowW, rowH, 0);
    y += rowH + gap;

    ui.stepper("Velocidade", settings.walkSpeed, 0.5f, 0.5f, 10.0f, x, y, rowW, rowH, 1);
    y += rowH + gap;

    ui.stepper("Velocidade (Shift)", settings.sprintSpeed, 0.5f, 1.0f, 20.0f, x, y, rowW, rowH, 1);
    y += rowH + gap;

    ui.stepper("Sensibilidade do mouse", settings.mouseSensitivity, 10.0f, 10.0f, 300.0f, x, y, rowW, rowH, 0);
    y += rowH + gap;

    const char *lightTypeLabel = "Tipo de luz: Pontual";
    switch (settings.lightType)
    {
    case LightType::Directional:
        lightTypeLabel = "Tipo de luz: Direcional";
        break;
    case LightType::Spot:
        lightTypeLabel = "Tipo de luz: Holofote";
        break;
    case LightType::Point:
    default:
        break;
    }

    if (ui.button(lightTypeLabel, x, y, rowW, rowH, 0.25f, 0.30f, 0.40f))
    {
        switch (settings.lightType)
        {
        case LightType::Point:
            settings.lightType = LightType::Directional;
            break;
        case LightType::Directional:
            settings.lightType = LightType::Spot;
            break;
        case LightType::Spot:
        default:
            settings.lightType = LightType::Point;
            break;
        }
    }
    y += rowH + gap;

    if (ui.button("Restaurar padrões", x, y, rowW, rowH, 0.25f, 0.30f, 0.40f))
        settings = Settings{};
    y += rowH + gap;

    if (ui.button("Voltar", x, y, rowW, rowH, 0.20f, 0.45f, 0.85f))
        page = Page::Main;
}

void Menu::drawLight(const Window &window, UI &ui, Settings &settings)
{
    const float panelW = 560.0f;
    const float margin = 30.0f;
    const float titleH = 70.0f;
    const float rowW = panelW - 2.0f * margin;
    const float rowH = 48.0f;
    const float gap = 14.0f;
    const int rows = 8; // Três da posição, três do alvo, restaurar e voltar.

    const float panelH = margin + titleH + rows * rowH + (rows - 1) * gap + margin;

    float px, py;
    drawPanel(window, ui, "Luz", panelW, panelH, px, py);

    const float x = px + margin;
    float y = py + margin + titleH;

    const float step = 0.25f;
    const float limit = 10.0f;

    ui.stepper("Posição X", settings.lightPosition.x, step, -limit, limit, x, y, rowW, rowH, 2);
    y += rowH + gap;
    ui.stepper("Posição Y", settings.lightPosition.y, step, -limit, limit, x, y, rowW, rowH, 2);
    y += rowH + gap;
    ui.stepper("Posição Z", settings.lightPosition.z, step, -limit, limit, x, y, rowW, rowH, 2);
    y += rowH + gap;

    ui.stepper("Alvo X", settings.lightTarget.x, step, -limit, limit, x, y, rowW, rowH, 2);
    y += rowH + gap;
    ui.stepper("Alvo Y", settings.lightTarget.y, step, -limit, limit, x, y, rowW, rowH, 2);
    y += rowH + gap;
    ui.stepper("Alvo Z", settings.lightTarget.z, step, -limit, limit, x, y, rowW, rowH, 2);
    y += rowH + gap;

    if (ui.button("Restaurar luz", x, y, rowW, rowH, 0.25f, 0.30f, 0.40f))
    {
        const Settings defaults;
        settings.lightPosition = defaults.lightPosition;
        settings.lightTarget = defaults.lightTarget;
    }
    y += rowH + gap;

    if (ui.button("Voltar", x, y, rowW, rowH, 0.20f, 0.45f, 0.85f))
        page = Page::Main;
}