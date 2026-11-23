#include "application.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include "applications/sandbox/ui/environment_info.h"
#include "world.h"

#ifndef SANDBOX_DATA_DIR
#define SANDBOX_DATA_DIR "."
#endif

namespace
{
    constexpr const char *SCENE_STATE_PATH =
        SANDBOX_DATA_DIR "/user_data/scene_state.json";

    WindowConfig createWindowConfig()
    {
        WindowConfig config;
        config.title = "OpenGL - RotorGrafico";
        config.mode = WindowMode::Fullscreen;
        return config;
    }
}

SandboxApplication::SandboxApplication()
    : rotor::Application(createWindowConfig())
{
}

bool SandboxApplication::onInitialize()
{
    if (!renderer.init())
        return false;

    if (!ui.create())
    {
        std::cout << "Falha ao inicializar a UI!" << std::endl;
        return false;
    }

    if (!editor.create())
    {
        std::cout << "Falha ao inicializar os recursos do Editor!"
                  << std::endl;
        return false;
    }

    // Primeiro cria os objetos e suas transformações iniciais.
    if (!buildWorld(scene))
        return false;

        // Registra a cena original antes de carregar os dados locais.
        scene.captureDefaults();

    // Depois recupera as transformações salvas anteriormente.
    if (!scene.loadTransforms(SCENE_STATE_PATH, light))
    {
        std::cout << "Nenhum estado salvo foi restaurado. "
                  << "Utilizando as transformacoes iniciais."
                  << std::endl;
    }

    sceneReadyForPersistence = true;
    return true;
}

void SandboxApplication::onUpdate(float deltaTime)
{
    if (doomActive)
    {
        if (window.keyPressed(GLFW_KEY_F10))
        {
            doomGame.suspend();
            doomActive = false;
            window.setCursorMode(Window::CursorMode::Normal);
        }
        else
        {
            doomGame.update(window);
        }
        return;
    }

    hud.update(window, deltaTime);
    menu.update(window);

    // A câmera só recebe controles quando o menu está fechado.
    if (menu.isOpen())
    {
        window.setCursorMode(Window::CursorMode::Normal);
    }
    else
    {
        editor.handleModeToggle();

        camera.processInput(window, deltaTime);

        if (!editor.isEditing())
            handleModelShortcuts(window, scene, selectedModelName);
    }

    // Mantém a câmera com o tamanho atual da janela.
    camera.setViewportSize(window.getWidth(), window.getHeight());

    // O editor só trata o mouse quando o menu está fechado.
    editor.update(!menu.isOpen());

    // Atualiza as animações e os objetos da cena.
    scene.update(deltaTime);
}

void SandboxApplication::onRender()
{
    if (doomActive)
    {
        doomGame.render(window.getWidth(), window.getHeight());
        return;
    }

    renderer.render(scene, light, camera);
    renderUI();
}

void SandboxApplication::onShutdown()
{
    if (!sceneReadyForPersistence)
        return;

    // Salva as transformações quando a aplicação encerra.
    if (!scene.saveTransforms(SCENE_STATE_PATH, light))
    {
        std::cerr << "Falha ao salvar as transformacoes da cena!"
                  << std::endl;
    }
}

void SandboxApplication::renderUI()
{
    // Inicia a interface gráfica.
    ui.begin(window);

    // Desenha o HUD e as informações da cena.
    hud.draw(
        ui,
        window,
        makeEnvironmentInfo(scene, camera, light, editor, selectedModelName)
    );

    // Desenha o menu.
    menu.draw(
    window,
    ui,
    renderer,
    camera,
    light,

    // Inicialização do DOOM.
    [this]()
    {
        startDoom();
    },

    // Restauração completa da cena.
    [this]()
    {
        if (!scene.resetToDefaults())
        {
            std::cerr << "Falha ao restaurar os objetos da cena."
                      << std::endl;
            return;
        }

        editor.resetForSceneDefaults();
        light.resetProperties();

        selectedModelName = "Todos";

        // Salva imediatamente o estado restaurado no computador.
        if (!scene.saveTransforms(SCENE_STATE_PATH, light))
        {
            std::cerr << "Falha ao salvar o estado restaurado."
                      << std::endl;
        }
    }
);

    // Exibe as ferramentas somente no modo de edição
    // e quando o menu estiver fechado.
    if (editor.isEditing() && !menu.isOpen())
    {
        editor.drawGizmos();

        if (editor.isLightSelected())
        {
            const Light previousLight = light;
            lightPanel.draw(window, ui, light);
            editor.recordLightChange(previousLight);
        }

        // Desenha a barra de ferramentas.
        editor.drawToolbar();
    }

    // Finaliza a interface gráfica.
    ui.end();
}

void SandboxApplication::startDoom()
{
#ifdef _WIN32
    std::vector<wchar_t> applicationPath(32768, L'\0');
    const DWORD pathLength = GetModuleFileNameW(
        nullptr, applicationPath.data(), static_cast<DWORD>(applicationPath.size()));
    if (pathLength == 0 || pathLength >= applicationPath.size())
    {
        MessageBoxW(nullptr, L"Não foi possível localizar a pasta do aplicativo.",
                    L"DOOM", MB_OK | MB_ICONERROR);
        return;
    }

    const std::filesystem::path applicationDirectory =
        std::filesystem::path(std::wstring(applicationPath.data(), pathLength)).parent_path();
    const std::filesystem::path doomWad = applicationDirectory / L"doom1.wad";

    std::error_code fileError;
    if (!std::filesystem::is_regular_file(doomWad, fileError) || fileError)
    {
        const std::wstring message =
            L"Coloque um arquivo doom1.wad obtido legalmente ao lado de OpenGLProjeto.exe:\n\n" +
            applicationDirectory.wstring();
        MessageBoxW(nullptr, message.c_str(), L"DOOM - WAD necessário",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::string error;
    if (!doomGame.start(doomWad, error))
    {
        std::cerr << error << std::endl;
        MessageBoxW(nullptr,
                    L"Não foi possível iniciar o DOOM dentro da engine. Consulte a saída do aplicativo.",
                    L"DOOM", MB_OK | MB_ICONERROR);
        return;
    }

    doomActive = true;
    window.setCursorMode(Window::CursorMode::Hidden);
#else
    std::error_code fileError;
    const std::filesystem::path doomWad =
        std::filesystem::current_path() / "build" / "doom1.wad";
    if (!std::filesystem::is_regular_file(doomWad, fileError) || fileError)
    {
        std::cerr << "Arquivo doom1.wad não encontrado: " << doomWad << std::endl;
        return;
    }

    std::string error;
    if (!doomGame.start(doomWad, error))
    {
        std::cerr << error << std::endl;
        return;
    }

    doomActive = true;
#endif
}
