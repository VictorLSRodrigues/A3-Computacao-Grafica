#include "menu.h"

#include "engine/ui/panel.h"

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

void Menu::draw(const Window &window, UI &ui, Settings &settings, Light &light)
{
    if (!open)
        return;

    if (page == Page::Options)
        drawOptions(window, ui, settings, light);
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

        if (layout.button("Sair", ButtonStyle::Danger))
            window.close(); });
}

void Menu::drawOptions(const Window &window, UI &ui, Settings &settings, Light &light)
{
    PanelStyle style; // largura 560, linhas de 48 px e espaço de 14 px

    drawCenteredPanel(window, ui, "Opções", style, [&](Layout &layout)
                      {
        layout.stepper("Campo de visão", settings.fov, 5.0f, 30.0f, 110.0f, 0);
        layout.stepper("Velocidade", settings.walkSpeed, 0.5f, 0.5f, 10.0f, 1);
        layout.stepper("Velocidade (Shift)", settings.sprintSpeed, 0.5f, 1.0f, 20.0f, 1);
        layout.stepper("Sensibilidade do mouse", settings.mouseSensitivity, 10.0f, 10.0f, 300.0f, 0);

        if (layout.button(settings.showSkybox ? "Céu: ligado" : "Céu: desligado"))
            settings.showSkybox = !settings.showSkybox;

        if (layout.button("Restaurar padrões"))
        {
            settings = Settings{};
            light.resetProperties();
        }

        if (layout.button("Voltar", ButtonStyle::Primary))
            page = Page::Main; });
}
