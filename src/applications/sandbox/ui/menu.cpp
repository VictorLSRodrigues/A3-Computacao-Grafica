#include "menu.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "applications/sandbox/ui/sandbox_ui_style.h"

#ifndef SANDBOX_DATA_DIR
#define SANDBOX_DATA_DIR "."
#endif

using sandbox_ui::ButtonTone;


void Menu::update(const Window& window)
{
    if (!window.keyPressed(GLFW_KEY_ESCAPE))
        return;

    if (open && page != Page::Main)
    {
        page = Page::Main;
        return;
    }

    open = !open;
    page = Page::Main;
}



void Menu::draw(
    Window& window,
    UI& ui,
    Renderer& renderer,
    Camera& camera,
    Light& light,
    const std::function<void()>& launchDoom,
    const std::function<bool(const std::string&)>& saveScene,
    const std::function<void()>& restoreDefaults)
{
    if (!open)
        return;

    switch (page)
    {
    case Page::Main:
        drawMain(window, ui);
        break;

    case Page::Options:
        drawOptions(
            window,
            ui,
            renderer,
            camera,
            light,
            launchDoom,
            restoreDefaults
        );
        break;

    case Page::SaveScene:
        drawSaveScene(window, ui, saveScene);
        break;

    case Page::Scenes:
        drawScenes(window, ui);
        break;
    }
}




void Menu::drawMain(const Window& window, UI& ui)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());

    const float panelW =
        std::min(440.0f, std::max(1.0f, screenW - 32.0f));

    const float panelH =
        std::min(450.0f, std::max(390.0f, screenH - 32.0f));

    const float x = (screenW - panelW) * 0.5f;
    const float y = (screenH - panelH) * 0.5f;

    sandbox_ui::drawPanel(
        ui, window, x, y, panelW, panelH, true
    );

    const float innerX = x + 28.0f;
    const float innerW = panelW - 56.0f;

    sandbox_ui::drawHeader(
        ui,
        innerX,
        y + 18.0f,
        innerW,
        "ROTOR / SANDBOX",
        "Menu principal",
        "Cena 3D e ferramentas de experimentação"
    );

    const float buttonH = 40.0f;
    const float gap = 8.0f;

    float buttonY = y + 116.0f;

    // Continuar
    if (sandbox_ui::button(
            ui,
            "Continuar",
            innerX,
            buttonY,
            innerW,
            buttonH,
            ButtonTone::Primary))
    {
        open = false;
    }

    // Carregar cena
    buttonY += buttonH + gap;

    if (sandbox_ui::button(
            ui,
            "Carregar cena",
            innerX,
            buttonY,
            innerW,
            buttonH,
            ButtonTone::Neutral))
    {
        scenePage = 0;
        page = Page::Scenes;
    }

    // Configurações
    buttonY += buttonH + gap;

    if (sandbox_ui::button(
            ui,
            "Configurações",
            innerX,
            buttonY,
            innerW,
            buttonH,
            ButtonTone::Neutral))
    {
        optionsCategory = OptionsCategory::Camera;
        page = Page::Options;
    }

    // Salvar cena
    buttonY += buttonH + gap;

    if (sandbox_ui::button(
            ui,
            "Salvar cena...",
            innerX,
            buttonY,
            innerW,
            buttonH,
            ButtonTone::Neutral))
    {
        sceneName.clear();
        saveStatus.clear();
        saveSucceeded = false;

        page = Page::SaveScene;
    }

    // Sair
    buttonY += buttonH + gap;

    if (sandbox_ui::button(
            ui,
            "Sair",
            innerX,
            buttonY,
            innerW,
            buttonH,
            ButtonTone::Danger))
    {
        window.close();
    }

    if (!saveStatus.empty())
    {
        sandbox_ui::drawTextFit(
            ui,
            saveStatus,
            innerX,
            y + panelH - 16.0f,
            innerW,
            0.32f,
            saveSucceeded
                ? sandbox_ui::palette::positive
                : sandbox_ui::palette::danger
        );
    }
}




void Menu::drawSaveScene(
    Window& window,
    UI& ui,
    const std::function<bool(const std::string&)>& saveScene)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());

    const float panelW =
        std::min(520.0f, std::max(1.0f, screenW - 32.0f));

    const float panelH =
        std::min(340.0f, std::max(300.0f, screenH - 32.0f));

    const float x = (screenW - panelW) * 0.5f;
    const float y = (screenH - panelH) * 0.5f;

    const float innerX = x + 28.0f;
    const float innerW = panelW - 56.0f;

    sandbox_ui::drawPanel(
        ui, window, x, y, panelW, panelH, true
    );

    sandbox_ui::drawHeader(
        ui,
        innerX,
        y + 18.0f,
        innerW,
        "ROTOR / SANDBOX",
        "Salvar cena",
        "Informe o nome do arquivo da cena"
    );

    constexpr std::size_t maxNameLength = 48;

    if (window.keyPressed(GLFW_KEY_BACKSPACE) && !sceneName.empty())
        sceneName.pop_back();

    // O Window atual não possui getTypedCharacters().
    // Utilizamos os eventos de teclado que ele realmente disponibiliza.
    const bool shiftDown =
        window.keyDown(GLFW_KEY_LEFT_SHIFT) ||
        window.keyDown(GLFW_KEY_RIGHT_SHIFT);

    for (int key = GLFW_KEY_A; key <= GLFW_KEY_Z; ++key)
    {
        if (window.keyPressed(key) &&
            sceneName.size() < maxNameLength)
        {
            const char base = shiftDown ? 'A' : 'a';

            sceneName.push_back(
                static_cast<char>(base + (key - GLFW_KEY_A))
            );
        }
    }

    for (int key = GLFW_KEY_0; key <= GLFW_KEY_9; ++key)
    {
        if (window.keyPressed(key) &&
            sceneName.size() < maxNameLength)
        {
            sceneName.push_back(
                static_cast<char>('0' + (key - GLFW_KEY_0))
            );
        }
    }

    if (window.keyPressed(GLFW_KEY_SPACE) &&
        sceneName.size() < maxNameLength)
    {
        sceneName.push_back(' ');
    }

    if (window.keyPressed(GLFW_KEY_MINUS) &&
        sceneName.size() < maxNameLength)
    {
        sceneName.push_back(shiftDown ? '_' : '-');
    }

    const float inputY = y + 126.0f;
    const float inputH = 44.0f;

    ui.addRegion(innerX, inputY, innerW, inputH);

    ui.drawRect(
        innerX, inputY, innerW, inputH,
        sandbox_ui::palette::backdrop.r,
        sandbox_ui::palette::backdrop.g,
        sandbox_ui::palette::backdrop.b
    );

    ui.drawRect(
        innerX, inputY, innerW, 1.0f,
        sandbox_ui::palette::border.r,
        sandbox_ui::palette::border.g,
        sandbox_ui::palette::border.b
    );

    ui.drawRect(
        innerX, inputY + inputH - 1.0f, innerW, 1.0f,
        sandbox_ui::palette::border.r,
        sandbox_ui::palette::border.g,
        sandbox_ui::palette::border.b
    );

    sandbox_ui::drawTextFit(
        ui,
        sceneName.empty()
            ? "Digite o nome da cena..."
            : sceneName,
        innerX + 12.0f,
        inputY + 27.0f,
        innerW - 24.0f,
        0.43f,
        sceneName.empty()
            ? sandbox_ui::palette::muted
            : sandbox_ui::palette::text
    );

    sandbox_ui::drawTextFit(
        ui,
        "Use letras sem acento, números, espaços, _ ou -.",
        innerX,
        inputY + 66.0f,
        innerW,
        0.34f,
        sandbox_ui::palette::muted
    );

    if (!saveStatus.empty())
    {
        sandbox_ui::drawTextFit(
            ui,
            saveStatus,
            innerX,
            inputY + 94.0f,
            innerW,
            0.34f,
            saveSucceeded
                ? sandbox_ui::palette::positive
                : sandbox_ui::palette::danger
        );
    }

    const float buttonY = y + panelH - 52.0f;
    const float buttonH = 36.0f;
    const float buttonGap = 10.0f;

    const float saveW = (innerW - buttonGap) * 0.5f;

    if (sandbox_ui::button(
            ui,
            "Salvar",
            innerX,
            buttonY,
            saveW,
            buttonH,
            ButtonTone::Primary))
    {
        const std::size_t first =
            sceneName.find_first_not_of(' ');

        const std::size_t last =
            sceneName.find_last_not_of(' ');

        if (first == std::string::npos)
        {
            saveSucceeded = false;
            saveStatus = "Digite um nome para salvar.";
        }
        else
        {
            const std::string normalizedName =
                sceneName.substr(first, last - first + 1);

            saveSucceeded =
                saveScene && saveScene(normalizedName);

            saveStatus = saveSucceeded
                ? "Cena salva como " + normalizedName + ".json"
                : "Falha ao salvar a cena.";

            if (saveSucceeded)
                page = Page::Main;
        }
    }

    if (sandbox_ui::button(
            ui,
            "Cancelar",
            innerX + saveW + buttonGap,
            buttonY,
            saveW,
            buttonH,
            ButtonTone::Neutral))
    {
        saveStatus.clear();
        saveSucceeded = false;
        page = Page::Main;
    }
}




void Menu::drawScenes(const Window& window, UI& ui)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());

    const float panelW =
        std::min(860.0f, std::max(1.0f, screenW - 32.0f));

    const float panelH =
        std::min(700.0f, std::max(380.0f, screenH - 32.0f));

    const float x = (screenW - panelW) * 0.5f;
    const float y = (screenH - panelH) * 0.5f;

    const float innerX = x + 28.0f;
    const float innerW = panelW - 56.0f;

    sandbox_ui::drawPanel(
        ui, window, x, y, panelW, panelH, true
    );

    sandbox_ui::drawHeader(
        ui,
        innerX,
        y + 16.0f,
        innerW,
        "ROTOR / SANDBOX",
        "Carregar cena",
        "Cenários encontrados na pasta user_data"
    );

    // Diretório de dados que também é utilizado pelo application.cpp.
    const std::filesystem::path sceneDirectory =
        std::filesystem::path(SANDBOX_DATA_DIR) / "user_data";

    std::vector<std::filesystem::path> sceneFiles;

    std::error_code directoryError;

    if (std::filesystem::is_directory(
            sceneDirectory, directoryError) &&
        !directoryError)
    {
        std::filesystem::directory_iterator it(
            sceneDirectory, directoryError);

        const std::filesystem::directory_iterator end;

        while (!directoryError && it != end)
        {
            std::error_code fileError;

            if (it->is_regular_file(fileError) &&
                !fileError &&
                it->path().extension() == ".json")
            {
                sceneFiles.push_back(it->path());
            }

            it.increment(directoryError);
        }
    }

    // Ordenação alfabética.
    std::sort(
        sceneFiles.begin(),
        sceneFiles.end(),
        [](const std::filesystem::path& a,
           const std::filesystem::path& b)
        {
            return a.filename().string() <
                   b.filename().string();
        }
    );

    constexpr std::size_t columns = 4;
    constexpr std::size_t rows = 3;
    constexpr std::size_t pageSize = columns * rows;

    constexpr float cardGap = 12.0f;

    const std::size_t pageCount = sceneFiles.empty()
        ? 1
        : (sceneFiles.size() + pageSize - 1) / pageSize;

    if (scenePage >= pageCount)
        scenePage = pageCount - 1;

    const float footerY = y + panelH - 58.0f;
    const float gridTop = y + 128.0f;

    const float gridHeight =
        std::max(1.0f, footerY - gridTop - 14.0f);

    const float cellW =
        (innerW - cardGap * (columns - 1)) / columns;

    const float cellH =
        (gridHeight - cardGap * (rows - 1)) / rows;

    const float cardSize =
        std::max(1.0f, std::min({142.0f, cellW, cellH}));

    if (sceneFiles.empty())
    {
        const std::string message = directoryError
            ? "Não foi possível ler a pasta user_data."
            : "Nenhum cenário .json encontrado em user_data.";

        sandbox_ui::drawCenteredText(
            ui,
            message,
            innerX,
            gridTop + 30.0f,
            innerW,
            72.0f,
            0.42f,
            sandbox_ui::palette::muted
        );
    }
    else
    {
        const std::size_t firstIndex =
            scenePage * pageSize;

        const std::size_t lastIndex =
            std::min(firstIndex + pageSize, sceneFiles.size());

        for (std::size_t i = firstIndex; i < lastIndex; ++i)
        {
            const std::size_t localIndex =
                i - firstIndex;

            const std::size_t row =
                localIndex / columns;

            const std::size_t column =
                localIndex % columns;

            const float cellX =
                innerX + static_cast<float>(column) *
                (cellW + cardGap);

            const float cellY =
                gridTop + static_cast<float>(row) *
                (cellH + cardGap);

            const float cardX =
                cellX + (cellW - cardSize) * 0.5f;

            const float cardY =
                cellY + (cellH - cardSize) * 0.5f;

            // Cartão visual. O carregamento será conectado
            // numa etapa posterior.
            ui.addRegion(cardX, cardY, cardSize, cardSize);

            ui.drawRect(
                cardX, cardY, cardSize, cardSize,
                sandbox_ui::palette::surface.r,
                sandbox_ui::palette::surface.g,
                sandbox_ui::palette::surface.b,
                0.98f
            );

            ui.drawRect(
                cardX, cardY, cardSize, 2.0f,
                sandbox_ui::palette::accent.r,
                sandbox_ui::palette::accent.g,
                sandbox_ui::palette::accent.b
            );

            ui.drawRect(
                cardX, cardY + cardSize - 1.0f,
                cardSize, 1.0f,
                sandbox_ui::palette::border.r,
                sandbox_ui::palette::border.g,
                sandbox_ui::palette::border.b
            );

            ui.drawRect(
                cardX, cardY, 1.0f, cardSize,
                sandbox_ui::palette::border.r,
                sandbox_ui::palette::border.g,
                sandbox_ui::palette::border.b
            );

            ui.drawRect(
                cardX + cardSize - 1.0f,
                cardY,
                1.0f,
                cardSize,
                sandbox_ui::palette::border.r,
                sandbox_ui::palette::border.g,
                sandbox_ui::palette::border.b
            );

            const std::string sceneName =
                sceneFiles[i].stem().string();

            const std::string visibleName =
                sandbox_ui::fitText(
                    ui,
                    sceneName,
                    cardSize - 16.0f,
                    0.40f
                );

            sandbox_ui::drawCenteredText(
                ui,
                visibleName,
                cardX + 8.0f,
                cardY + 12.0f,
                cardSize - 16.0f,
                cardSize - 24.0f,
                0.40f,
                sandbox_ui::palette::text
            );
        }
    }

    // Rodapé e navegação.
    sandbox_ui::drawDivider(
        ui,
        innerX,
        footerY - 12.0f,
        innerW
    );

    if (sandbox_ui::button(
            ui,
            "Voltar",
            innerX,
            footerY - 2.0f,
            100.0f,
            36.0f,
            ButtonTone::Neutral))
    {
        page = Page::Main;
    }

    const std::string countText =
        std::to_string(sceneFiles.size()) + " cenário(s)";

    sandbox_ui::drawTextFit(
        ui,
        countText,
        innerX + 112.0f,
        footerY + 21.0f,
        190.0f,
        0.34f,
        sandbox_ui::palette::muted
    );

    const float nextX = innerX + innerW - 100.0f;
    const float previousX = nextX - 108.0f;

    if (scenePage > 0 &&
        sandbox_ui::button(
            ui,
            "Anterior",
            previousX,
            footerY - 2.0f,
            100.0f,
            36.0f,
            ButtonTone::Neutral))
    {
        --scenePage;
    }

    if (scenePage + 1 < pageCount &&
        sandbox_ui::button(
            ui,
            "Próxima",
            nextX,
            footerY - 2.0f,
            100.0f,
            36.0f,
            ButtonTone::Primary))
    {
        ++scenePage;
    }
}

void Menu::drawOptions(
    Window& window,
    UI& ui,
    Renderer& renderer,
    Camera& camera,
    Light& light,
    const std::function<void()>& launchDoom,
    const std::function<void()>& restoreDefaults)
{
    const float screenW = static_cast<float>(window.getWidth());
    const float screenH = static_cast<float>(window.getHeight());

    const float panelW =
        std::min(720.0f, std::max(1.0f, screenW - 32.0f));

    const float panelH =
        std::min(520.0f, std::max(250.0f, screenH - 32.0f));

    const float x = (screenW - panelW) * 0.5f;
    const float y = (screenH - panelH) * 0.5f;

    sandbox_ui::drawPanel(
        ui, window, x, y, panelW, panelH, true
    );

    const float innerX = x + 28.0f;
    const float innerW = panelW - 56.0f;

    sandbox_ui::drawHeader(
        ui,
        innerX,
        y + 16.0f,
        innerW,
        "ROTOR / PREFERÊNCIAS",
        "Configurações",
        "Ajustes da sessão atual"
    );

    const float tabsY = y + 112.0f;
    const float tabGap = 8.0f;
    const float tabW = (innerW - tabGap * 4.0f) / 5.0f;
    const float tabH = 36.0f;

    struct CategoryTab
    {
        OptionsCategory category;
        const char* label;
    };

    const CategoryTab tabs[] = {
        {OptionsCategory::Camera, "Câmera"},
        {OptionsCategory::Environment, "Ambiente"},
        {OptionsCategory::Lighting, "Iluminação"},
        {OptionsCategory::Window, "Janela"},
        {OptionsCategory::Doom, "DOOM"}
    };

    for (int i = 0; i < 5; ++i)
    {
        const float tabX = innerX + i * (tabW + tabGap);

        const bool selected =
            optionsCategory == tabs[i].category;

        if (sandbox_ui::button(
                ui,
                tabs[i].label,
                tabX,
                tabsY,
                tabW,
                tabH,
                selected
                    ? ButtonTone::Selected
                    : ButtonTone::Neutral))
        {
            optionsCategory = tabs[i].category;
        }
    }

    sandbox_ui::drawDivider(
        ui,
        innerX,
        tabsY + tabH + 14.0f,
        innerW
    );

    const float contentY = tabsY + tabH + 28.0f;

    switch (optionsCategory)
    {
    case OptionsCategory::Camera:
    {
        sandbox_ui::drawSection(
            ui, innerX, contentY, innerW, "CÂMERA"
        );

        const bool compactRows = innerW < 430.0f;
        const float rowHeight = compactRows ? 54.0f : 36.0f;
        const float rowGap = compactRows ? 8.0f : 12.0f;

        float rowY = contentY + 30.0f;

        sandbox_ui::stepper(
            ui, "Campo de visão", camera.fov,
            5.0f, 30.0f, 110.0f, 0,
            innerX, rowY, innerW, rowHeight
        );

        rowY += rowHeight + rowGap;

        sandbox_ui::stepper(
            ui, "Velocidade de caminhada", camera.walkSpeed,
            0.5f, 0.5f, 10.0f, 1,
            innerX, rowY, innerW, rowHeight
        );

        rowY += rowHeight + rowGap;

        sandbox_ui::stepper(
            ui, "Velocidade com Shift", camera.sprintSpeed,
            0.5f, 1.0f, 20.0f, 1,
            innerX, rowY, innerW, rowHeight
        );

        rowY += rowHeight + rowGap;

        sandbox_ui::stepper(
            ui, "Sensibilidade do mouse", camera.sensitivity,
            10.0f, 10.0f, 300.0f, 0,
            innerX, rowY, innerW, rowHeight
        );

        break;
    }

    case OptionsCategory::Environment:
    {
        sandbox_ui::drawSection(
            ui, innerX, contentY, innerW, "AMBIENTE"
        );

        if (sandbox_ui::toggleRow(
                ui, innerX, contentY + 24.0f, innerW, 62.0f,
                "Céu",
                "Exibe o ambiente ao redor da cena",
                renderer.showSkybox))
        {
            renderer.showSkybox = !renderer.showSkybox;
        }

        if (sandbox_ui::toggleRow(
                ui, innerX, contentY + 92.0f, innerW, 62.0f,
                "Chuva",
                "Ativa a precipitação na cena",
                renderer.rainScenario.enabled))
        {
            renderer.rainScenario.enabled =
                !renderer.rainScenario.enabled;
        }

        break;
    }

    case OptionsCategory::Lighting:
    {
        sandbox_ui::drawSection(
            ui, innerX, contentY, innerW,
            "ILUMINAÇÃO AMBIENTAL"
        );

        const bool compactRows = innerW < 430.0f;
        const float rowHeight = compactRows ? 54.0f : 36.0f;
        const float rowGap = compactRows ? 8.0f : 12.0f;

        sandbox_ui::stepper(
            ui, "Luz ambiente difusa",
            renderer.ambientStrength,
            0.02f, 0.0f, 1.0f, 2,
            innerX, contentY + 34.0f,
            innerW, rowHeight
        );

        sandbox_ui::stepper(
            ui, "Reflexão ambiental",
            renderer.environmentReflectionStrength,
            0.05f, 0.0f, 2.0f, 2,
            innerX, contentY + 34.0f + rowHeight + rowGap,
            innerW, rowHeight
        );

        break;
    }

    case OptionsCategory::Window:
    {
        sandbox_ui::drawSection(
            ui, innerX, contentY, innerW, "MODO DA JANELA"
        );

        sandbox_ui::drawTextFit(
            ui,
            "Escolha como o Sandbox deve ocupar a tela.",
            innerX,
            contentY + 39.0f,
            innerW,
            0.40f,
            sandbox_ui::palette::muted
        );

        struct ModeOption
        {
            WindowMode mode;
            const char* label;
        };

        const ModeOption modes[] = {
            {WindowMode::Windowed, "Janela normal"},
            {WindowMode::Maximized, "Maximizada"},
            {WindowMode::Borderless, "Sem bordas"},
            {WindowMode::Fullscreen, "Tela cheia"}
        };

        const float modeGap = 10.0f;
        const float modeButtonWidth =
            (innerW - modeGap) * 0.5f;

        const float modeButtonHeight = 42.0f;
        const float modeStartY = contentY + 52.0f;

        for (int i = 0; i < 4; ++i)
        {
            const int column = i % 2;
            const int row = i / 2;

            const float buttonX =
                innerX + column * (modeButtonWidth + modeGap);

            const float buttonY =
                modeStartY + row * (modeButtonHeight + 10.0f);

            const bool selected =
                window.getMode() == modes[i].mode;

            if (sandbox_ui::button(
                    ui,
                    modes[i].label,
                    buttonX,
                    buttonY,
                    modeButtonWidth,
                    modeButtonHeight,
                    selected
                        ? ButtonTone::Selected
                        : ButtonTone::Neutral))
            {
                windowModeChangeFailed =
                    !window.setMode(modes[i].mode);
            }
        }

        const char* currentModeName = "Janela normal";

        const char* currentModeDescription =
            "Janela com bordas, barra de título e tamanho ajustável.";

        switch (window.getMode())
        {
        case WindowMode::Windowed:
            break;

        case WindowMode::Maximized:
            currentModeName = "Maximizada";
            currentModeDescription =
                "Janela maximizada, mantendo a barra de título.";
            break;

        case WindowMode::Borderless:
            currentModeName = "Sem bordas";
            currentModeDescription =
                "Ocupa o monitor sem bordas e sem trocar o modo de vídeo.";
            break;

        case WindowMode::Fullscreen:
            currentModeName = "Tela cheia";
            currentModeDescription =
                "Tela cheia associada ao monitor, utilizando seu modo de vídeo.";
            break;
        }

        sandbox_ui::drawTextFit(
            ui,
            std::string("Modo atual: ") + currentModeName,
            innerX,
            contentY + 178.0f,
            innerW,
            0.43f,
            sandbox_ui::palette::text
        );

        sandbox_ui::drawTextFit(
            ui,
            currentModeDescription,
            innerX,
            contentY + 205.0f,
            innerW,
            0.37f,
            sandbox_ui::palette::muted
        );

        if (windowModeChangeFailed)
        {
            sandbox_ui::drawTextFit(
                ui,
                "Não foi possível alterar o modo da janela.",
                innerX,
                contentY + 232.0f,
                innerW,
                0.37f,
                sandbox_ui::palette::danger
            );
        }

        break;
    }

    case OptionsCategory::Doom:
    {
        sandbox_ui::drawSection(
            ui, innerX, contentY, innerW, "JOGAR DOOM"
        );

        sandbox_ui::drawTextFit(
            ui,
            "Coloque doom1.wad ao lado de OpenGLProjeto.exe.",
            innerX,
            contentY + 48.0f,
            innerW,
            0.38f,
            sandbox_ui::palette::text
        );

        sandbox_ui::drawTextFit(
            ui,
            "F10 volta ao Sandbox durante o jogo.",
            innerX,
            contentY + 76.0f,
            innerW,
            0.36f,
            sandbox_ui::palette::muted
        );

        if (sandbox_ui::button(
                ui,
                "Iniciar DOOM",
                innerX,
                contentY + 112.0f,
                std::min(220.0f, innerW),
                42.0f,
                ButtonTone::Primary))
        {
            if (launchDoom)
                launchDoom();
        }

        break;
    }
    }

    const float footerY = y + panelH - 58.0f;

    sandbox_ui::drawDivider(
        ui, innerX, footerY - 12.0f, innerW
    );

    sandbox_ui::drawTextFit(
        ui,
        "ESC  ·  voltar ao menu",
        innerX,
        footerY + 22.0f,
        std::max(0.0f, innerW - 230.0f),
        0.34f,
        sandbox_ui::palette::muted
    );

    const float actionGap = 8.0f;

    const float backW =
        std::min(112.0f, std::max(72.0f, innerW * 0.30f));

    const float restoreW =
        std::max(96.0f, innerW - backW - actionGap);

    const float backX =
        x + panelW - 28.0f - backW;

    const float restoreX =
        backX - actionGap - restoreW;

    if (sandbox_ui::button(
            ui,
            "Restaurar padrões",
            restoreX,
            footerY - 2.0f,
            restoreW,
            36.0f,
            ButtonTone::Neutral))
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
