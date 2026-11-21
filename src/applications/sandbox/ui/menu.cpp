#include "menu.h"

#include <algorithm>

#include "applications/sandbox/ui/sandbox_ui_style.h"

using sandbox_ui::ButtonTone;

void Menu::update(const Window &window)
{
    if (!window.keyPressed(GLFW_KEY_ESCAPE))
        return;

    if (open && page != Page::Main)
    {
        page = Page::Main;
    }
    else
    {
        open = !open;
        page = Page::Main;
    }
}

void Menu::draw(const Window &window, UI &ui, Renderer &renderer, Camera &camera, Light &light,
                const std::function<void()>& launchDoom)
{
    if (!open)
        return;

    if (page == Page::Options)
        drawOptions(window, ui, renderer, camera, light, launchDoom);
    else
        drawMain(window, ui);
}

void Menu::drawMain(const Window &window, UI &ui)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());
    const float panelW = std::min(440.0f, std::max(1.0f, screenW - 32.0f));
    const float panelH = std::min(330.0f, std::max(250.0f, screenH - 32.0f));
    const float x = (screenW - panelW) * 0.5f;
    const float y = (screenH - panelH) * 0.5f;

    sandbox_ui::drawPanel(ui, window, x, y, panelW, panelH, true);
    const float innerX = x + 28.0f;
    const float innerW = panelW - 56.0f;

    sandbox_ui::drawHeader(ui, innerX, y + 18.0f, innerW,
                           "ROTOR / SANDBOX", "Menu principal",
                           "Cena 3D e ferramentas de experimentação");

    const float buttonH = 44.0f;
    const float gap = 10.0f;
    float buttonY = y + 126.0f;

    if (sandbox_ui::button(ui, "Continuar", innerX, buttonY,
                           innerW, buttonH, ButtonTone::Primary))
        open = false;

    buttonY += buttonH + gap;
    if (sandbox_ui::button(ui, "Configurações", innerX, buttonY,
                           innerW, buttonH, ButtonTone::Neutral))
    {
        optionsCategory = OptionsCategory::Camera;
        page = Page::Options;
    }

    buttonY += buttonH + gap;
    if (sandbox_ui::button(ui, "Sair", innerX, buttonY,
                           innerW, buttonH, ButtonTone::Danger))
        window.close();
}

void Menu::drawOptions(const Window &window, UI &ui, Renderer &renderer, Camera &camera, Light &light,
                       const std::function<void()>& launchDoom)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());
    const float panelW = std::min(720.0f, std::max(1.0f, screenW - 32.0f));
    const float panelH = std::min(520.0f, std::max(250.0f, screenH - 32.0f));
    const float x = (screenW - panelW) * 0.5f;
    const float y = (screenH - panelH) * 0.5f;

    sandbox_ui::drawPanel(ui, window, x, y, panelW, panelH, true);
    const float innerX = x + 28.0f;
    const float innerW = panelW - 56.0f;

    sandbox_ui::drawHeader(ui, innerX, y + 16.0f, innerW,
                           "ROTOR / PREFERÊNCIAS", "Configurações",
                           "Ajustes da sessão atual");

    const float tabsY = y + 112.0f;
    const float tabGap = 8.0f;
    const float tabW = (innerW - tabGap * 3.0f) / 4.0f;
    const float tabH = 36.0f;

    struct CategoryTab
    {
        OptionsCategory category;
        const char *label;
    };
    const CategoryTab tabs[] = {
        {OptionsCategory::Camera, "Câmera"},
        {OptionsCategory::Environment, "Ambiente"},
        {OptionsCategory::Lighting, "Iluminação"},
        {OptionsCategory::Doom, "DOOM"}};

    for (int i = 0; i < 4; ++i)
    {
        const float tabX = innerX + i * (tabW + tabGap);
        const bool selected = optionsCategory == tabs[i].category;
        if (sandbox_ui::button(ui, tabs[i].label, tabX, tabsY,
                               tabW, tabH,
                               selected ? ButtonTone::Selected : ButtonTone::Neutral))
            optionsCategory = tabs[i].category;
    }

    sandbox_ui::drawDivider(ui, innerX, tabsY + tabH + 14.0f, innerW);

    const float contentY = tabsY + tabH + 28.0f;
    switch (optionsCategory)
    {
    case OptionsCategory::Camera:
    {
        sandbox_ui::drawSection(ui, innerX, contentY, innerW, "CÂMERA");
        const bool compactRows = innerW < 430.0f;
        const float rowHeight = compactRows ? 54.0f : 36.0f;
        const float rowGap = compactRows ? 8.0f : 12.0f;
        float rowY = contentY + 30.0f;

        sandbox_ui::stepper(ui, "Campo de visão", camera.fov,
                            5.0f, 30.0f, 110.0f, 0,
                            innerX, rowY, innerW, rowHeight);
        rowY += rowHeight + rowGap;
        sandbox_ui::stepper(ui, "Velocidade de caminhada", camera.walkSpeed,
                            0.5f, 0.5f, 10.0f, 1,
                            innerX, rowY, innerW, rowHeight);
        rowY += rowHeight + rowGap;
        sandbox_ui::stepper(ui, "Velocidade com Shift", camera.sprintSpeed,
                            0.5f, 1.0f, 20.0f, 1,
                            innerX, rowY, innerW, rowHeight);
        rowY += rowHeight + rowGap;
        sandbox_ui::stepper(ui, "Sensibilidade do mouse", camera.sensitivity,
                            10.0f, 10.0f, 300.0f, 0,
                            innerX, rowY, innerW, rowHeight);
        break;
    }

    case OptionsCategory::Environment:
        sandbox_ui::drawSection(ui, innerX, contentY, innerW, "AMBIENTE");
        if (sandbox_ui::toggleRow(ui, innerX, contentY + 24.0f, innerW, 62.0f,
                                  "Céu", "Exibe o ambiente ao redor da cena",
                                  renderer.showSkybox))
            renderer.showSkybox = !renderer.showSkybox;
        if (sandbox_ui::toggleRow(ui, innerX, contentY + 92.0f, innerW, 62.0f,
                                  "Chuva", "Ativa a precipitação na cena",
                                  renderer.rainScenario.enabled))
            renderer.rainScenario.enabled = !renderer.rainScenario.enabled;
        break;

    case OptionsCategory::Lighting:
    {
        sandbox_ui::drawSection(ui, innerX, contentY, innerW, "ILUMINAÇÃO AMBIENTAL");
        const bool compactRows = innerW < 430.0f;
        const float rowHeight = compactRows ? 54.0f : 36.0f;
        const float rowGap = compactRows ? 8.0f : 12.0f;
        sandbox_ui::stepper(ui, "Luz ambiente difusa", renderer.ambientStrength,
                            0.02f, 0.0f, 1.0f, 2,
                            innerX, contentY + 34.0f, innerW, rowHeight);
        sandbox_ui::stepper(ui, "Reflexão ambiental", renderer.environmentReflectionStrength,
                            0.05f, 0.0f, 2.0f, 2,
                            innerX, contentY + 34.0f + rowHeight + rowGap, innerW, rowHeight);
        break;
    }

    case OptionsCategory::Doom:
        sandbox_ui::drawSection(ui, innerX, contentY, innerW, "JOGAR DOOM");
        sandbox_ui::drawTextFit(ui, "Coloque doom1.wad ao lado de OpenGLProjeto.exe.",
                                innerX, contentY + 48.0f, innerW, 0.38f,
                                sandbox_ui::palette::text);
        sandbox_ui::drawTextFit(ui, "F10 volta ao Sandbox durante o jogo.",
                                innerX, contentY + 76.0f, innerW, 0.36f,
                                sandbox_ui::palette::muted);
        if (sandbox_ui::button(ui, "Iniciar DOOM", innerX, contentY + 112.0f,
                               std::min(220.0f, innerW), 42.0f, ButtonTone::Primary))
            launchDoom();
        break;
    }

    const float footerY = y + panelH - 58.0f;
    sandbox_ui::drawDivider(ui, innerX, footerY - 12.0f, innerW);
    sandbox_ui::drawTextFit(ui, "ESC  ·  voltar ao menu", innerX,
                            footerY + 22.0f, std::max(0.0f, innerW - 230.0f),
                            0.34f, sandbox_ui::palette::muted);

    const float actionGap = 8.0f;
    const float backW = std::min(112.0f, std::max(72.0f, innerW * 0.30f));
    const float restoreW = std::max(96.0f, innerW - backW - actionGap);
    const float backX = x + panelW - 28.0f - backW;
    const float restoreX = backX - actionGap - restoreW;

    if (sandbox_ui::button(ui, "Restaurar padrões", restoreX, footerY - 2.0f,
                           restoreW, 36.0f, ButtonTone::Neutral))
    {
        renderer.resetSettings();
        camera.resetSettings();
        light.resetProperties();
    }
    if (sandbox_ui::button(ui, "Voltar", backX, footerY - 2.0f,
                           backW, 36.0f, ButtonTone::Primary))
        page = Page::Main;
}
