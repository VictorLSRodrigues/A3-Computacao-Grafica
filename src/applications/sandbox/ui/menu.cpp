#include "menu.h"

#include <algorithm>
#include <string>

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

void Menu::draw(Window &window, UI &ui, Renderer &renderer, Camera &camera, Light &light, const std::function<void()> &launchDoom, const std::function<bool(const std::string&)> &saveScene, const std::function<void()> &restoreDefaults)
{
    if (!open)
        return;

    if (page == Page::Options)
        drawOptions(
            window, ui, renderer, camera, light,
            launchDoom, restoreDefaults);
    else if (page == Page::SaveScene)
        drawSaveScene(window, ui, saveScene);
    else
        drawMain(window, ui, saveScene);
}

void Menu::drawMain(
    const Window &window,
    UI &ui,
    const std::function<bool(const std::string&)> &saveScene)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());
    const float panelW = std::min(440.0f, std::max(1.0f, screenW - 32.0f));
    const float panelH = std::min(390.0f, std::max(250.0f, screenH - 32.0f));
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
    if (sandbox_ui::button(ui, "Salvar cena...", innerX, buttonY,
                           innerW, buttonH, ButtonTone::Neutral))
    {
        sceneName.clear();
        saveStatus.clear();
        page = Page::SaveScene;
    }

    if (!saveStatus.empty())
    {
        sandbox_ui::drawTextFit(
            ui, saveStatus, innerX, y + panelH - 18.0f, innerW, 0.34f,
            saveSucceeded ? sandbox_ui::palette::positive
                          : sandbox_ui::palette::danger);
    }

    buttonY += buttonH + gap;
    if (sandbox_ui::button(ui, "Sair", innerX, buttonY,
                           innerW, buttonH, ButtonTone::Danger))
        window.close();
}

void Menu::drawSaveScene(
    Window &window,
    UI &ui,
    const std::function<bool(const std::string&)> &saveScene)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());
    const float panelW = std::min(500.0f, std::max(1.0f, screenW - 32.0f));
    const float panelH = std::min(300.0f, std::max(250.0f, screenH - 32.0f));
    const float x = (screenW - panelW) * 0.5f;
    const float y = (screenH - panelH) * 0.5f;
    const float innerX = x + 28.0f;
    const float innerW = panelW - 56.0f;

    sandbox_ui::drawPanel(ui, window, x, y, panelW, panelH, true);
    sandbox_ui::drawHeader(ui, innerX, y + 18.0f, innerW,
                           "ROTOR / SANDBOX", "Salvar cena",
                           "Informe o nome do arquivo da cena");

    constexpr std::size_t maxNameLength = 48;
    if (window.keyPressed(GLFW_KEY_BACKSPACE) && !sceneName.empty())
        sceneName.pop_back();

    for (unsigned int codepoint : window.getTypedCharacters())
    {
        const bool allowed =
            (codepoint >= 'a' && codepoint <= 'z') ||
            (codepoint >= 'A' && codepoint <= 'Z') ||
            (codepoint >= '0' && codepoint <= '9') ||
            codepoint == ' ' || codepoint == '_' || codepoint == '-';
        if (allowed && sceneName.size() < maxNameLength)
            sceneName.push_back(static_cast<char>(codepoint));
    }

    const float inputY = y + 126.0f;
    const float inputH = 44.0f;
    ui.addRegion(innerX, inputY, innerW, inputH);
    ui.drawRect(innerX, inputY, innerW, inputH,
                sandbox_ui::palette::backdrop.r,
                sandbox_ui::palette::backdrop.g,
                sandbox_ui::palette::backdrop.b);
    ui.drawRect(innerX, inputY, innerW, 1.0f,
                sandbox_ui::palette::border.r,
                sandbox_ui::palette::border.g,
                sandbox_ui::palette::border.b);
    ui.drawRect(innerX, inputY + inputH - 1.0f, innerW, 1.0f,
                sandbox_ui::palette::border.r,
                sandbox_ui::palette::border.g,
                sandbox_ui::palette::border.b);
    sandbox_ui::drawTextFit(
        ui, sceneName.empty() ? "Digite o nome da cena..." : sceneName,
        innerX + 12.0f, inputY + 27.0f, innerW - 24.0f, 0.43f,
        sceneName.empty() ? sandbox_ui::palette::muted
                          : sandbox_ui::palette::text);
    sandbox_ui::drawTextFit(
        ui, "Use letras sem acento, números, espaços, _ ou -.",
        innerX, inputY + 66.0f, innerW, 0.34f,
        sandbox_ui::palette::muted);

    if (!saveStatus.empty())
    {
        sandbox_ui::drawTextFit(
            ui, saveStatus, innerX, inputY + 94.0f, innerW, 0.34f,
            saveSucceeded ? sandbox_ui::palette::positive
                          : sandbox_ui::palette::danger);
    }

    const float buttonY = y + panelH - 52.0f;
    const float buttonH = 36.0f;
    const float buttonGap = 10.0f;
    const float saveW = (innerW - buttonGap) * 0.5f;
    if (sandbox_ui::button(ui, "Salvar", innerX, buttonY,
                           saveW, buttonH, ButtonTone::Primary))
    {
        const std::size_t first = sceneName.find_first_not_of(' ');
        const std::size_t last = sceneName.find_last_not_of(' ');
        if (first == std::string::npos)
        {
            saveSucceeded = false;
            saveStatus = "Digite um nome para salvar.";
        }
        else
        {
            const std::string normalizedName =
                sceneName.substr(first, last - first + 1);
            saveSucceeded = saveScene && saveScene(normalizedName);
            saveStatus = saveSucceeded
                ? "Cena salva como " + normalizedName + ".json"
                : "Falha ao salvar a cena.";
            if (saveSucceeded)
                page = Page::Main;
        }
    }

    if (sandbox_ui::button(ui, "Cancelar", innerX + saveW + buttonGap,
                           buttonY, saveW, buttonH, ButtonTone::Neutral))
    {
        saveStatus.clear();
        page = Page::Main;
    }
}

void Menu::drawOptions(Window &window, UI &ui, Renderer &renderer, Camera &camera, Light &light, const std::function<void()> &launchDoom, const std::function<void()> &restoreDefaults)
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
    const float tabW = (innerW - tabGap * 4.0f) / 5.0f;
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
        {OptionsCategory::Window, "Janela"},
        {OptionsCategory::Doom, "DOOM"}};

    for (int i = 0; i < 5; ++i)
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

    case OptionsCategory::Window:
    {
        sandbox_ui::drawSection(ui, innerX, contentY, innerW, "MODO DA JANELA");
        sandbox_ui::drawTextFit(ui, "Escolha como o Sandbox deve ocupar a tela.",
                                innerX, contentY + 39.0f, innerW, 0.40f,
                                sandbox_ui::palette::muted);

        struct ModeOption
        {
            WindowMode mode;
            const char *label;
        };
        const ModeOption modes[] = {
            {WindowMode::Windowed, "Janela normal"},
            {WindowMode::Maximized, "Maximizada"},
            {WindowMode::Borderless, "Sem bordas"},
            {WindowMode::Fullscreen, "Tela cheia"}};

        const float modeGap = 10.0f;
        const float modeButtonWidth = (innerW - modeGap) * 0.5f;
        const float modeButtonHeight = 42.0f;
        const float modeStartY = contentY + 52.0f;

        for (int i = 0; i < 4; ++i)
        {
            const int column = i % 2;
            const int row = i / 2;
            const float buttonX = innerX + column * (modeButtonWidth + modeGap);
            const float buttonY = modeStartY + row * (modeButtonHeight + 10.0f);
            const bool selected = window.getMode() == modes[i].mode;

            if (sandbox_ui::button(ui, modes[i].label, buttonX, buttonY,
                                   modeButtonWidth, modeButtonHeight,
                                   selected ? ButtonTone::Selected : ButtonTone::Neutral))
            {
                windowModeChangeFailed = !window.setMode(modes[i].mode);
            }
        }

        const char *currentModeName = "Janela normal";
        const char *currentModeDescription = "Janela com bordas, barra de título e tamanho ajustável.";
        switch (window.getMode())
        {
        case WindowMode::Windowed:
            break;
        case WindowMode::Maximized:
            currentModeName = "Maximizada";
            currentModeDescription = "Janela maximizada, mantendo a barra de título.";
            break;
        case WindowMode::Borderless:
            currentModeName = "Sem bordas";
            currentModeDescription = "Ocupa o monitor sem bordas e sem trocar o modo de vídeo.";
            break;
        case WindowMode::Fullscreen:
            currentModeName = "Tela cheia";
            currentModeDescription = "Tela cheia associada ao monitor, utilizando seu modo de vídeo.";
            break;
        }

        sandbox_ui::drawTextFit(ui, std::string("Modo atual: ") + currentModeName,
                                innerX, contentY + 178.0f, innerW, 0.43f,
                                sandbox_ui::palette::text);
        sandbox_ui::drawTextFit(ui, currentModeDescription,
                                innerX, contentY + 205.0f, innerW, 0.37f,
                                sandbox_ui::palette::muted);

        if (windowModeChangeFailed)
        {
            sandbox_ui::drawTextFit(ui, "Não foi possível alterar o modo da janela.",
                                    innerX, contentY + 232.0f, innerW, 0.37f,
                                    sandbox_ui::palette::danger);
        }
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

    if (sandbox_ui::button(
            ui, "Restaurar padrões",
            restoreX, footerY - 2.0f,
            restoreW, 36.0f, ButtonTone::Neutral))
    {
        renderer.resetSettings();
        camera.resetSettings();
        light.resetProperties();

        windowModeChangeFailed =
            !window.setMode(WindowMode::Fullscreen);

        if (restoreDefaults)
            restoreDefaults();
    }
}
