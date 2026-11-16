#include "menu.h"

#include "panel.h"

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

void Menu::drawMain(const Window &window, UI &ui)
{
    PanelStyle style;
    style.width = 420.0f;
    style.rowHeight = 60.0f;
    style.gap = 16.0f;

    drawCenteredPanel(window, ui, "Menu", style, [&](Layout &layout)
                      {
        if (layout.button("Continuar", ButtonStyle::Primary))
            open = false;

        if (layout.button("Opções"))
            page = Page::Options;

        if (layout.button("Luz"))
            page = Page::Light;

        if (layout.button("Sair", ButtonStyle::Danger))
            window.close(); });
}

void Menu::drawOptions(const Window &window, UI &ui, Settings &settings)
{
    PanelStyle style; // largura 560, linhas de 48 px e espaço de 14 px

    drawCenteredPanel(window, ui, "Opções", style, [&](Layout &layout)
                      {
        layout.stepper("Campo de visão", settings.fov, 5.0f, 30.0f, 110.0f, 0);
        layout.stepper("Velocidade", settings.walkSpeed, 0.5f, 0.5f, 10.0f, 1);
        layout.stepper("Velocidade (Shift)", settings.sprintSpeed, 0.5f, 1.0f, 20.0f, 1);
        layout.stepper("Sensibilidade do mouse", settings.mouseSensitivity, 10.0f, 10.0f, 300.0f, 0);

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

        if (layout.button(lightTypeLabel))
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

        if (layout.button(settings.showSkybox ? "Céu: ligado" : "Céu: desligado"))
            settings.showSkybox = !settings.showSkybox;

        if (layout.button("Restaurar padrões"))
            settings = Settings{};

        if (layout.button("Voltar", ButtonStyle::Primary))
            page = Page::Main; });
}

void Menu::drawLight(const Window &window, UI &ui, Settings &settings)
{
    PanelStyle style;
    style.rowHeight = 40.0f;
    style.gap = 10.0f;

    drawCenteredPanel(window, ui, "Luz", style, [&](Layout &layout)
                      {
        const float step = 0.25f;
        const float limit = 10.0f;

        layout.stepper("Posição X", settings.lightPosition.x, step, -limit, limit, 2);
        layout.stepper("Posição Y", settings.lightPosition.y, step, -limit, limit, 2);
        layout.stepper("Posição Z", settings.lightPosition.z, step, -limit, limit, 2);

        layout.stepper("Alvo X", settings.lightTarget.x, step, -limit, limit, 2);
        layout.stepper("Alvo Y", settings.lightTarget.y, step, -limit, limit, 2);
        layout.stepper("Alvo Z", settings.lightTarget.z, step, -limit, limit, 2);

        layout.colorPicker("Cor", settings.lightColor);

        if (layout.button("Restaurar luz"))
        {
            const Settings defaults;
            settings.lightPosition = defaults.lightPosition;
            settings.lightTarget = defaults.lightTarget;
            settings.lightColor = defaults.lightColor;
        }

        if (layout.button("Voltar", ButtonStyle::Primary))
            page = Page::Main; });
}
