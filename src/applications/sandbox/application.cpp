#include "application.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
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

bool Application::init()
{
    if (!window.create("OpenGL - RotorGrafico"))
        return false;

    if (!renderer.init())
        return false;

    if (!ui.create())
    {
        std::cout << "Falha ao inicializar a UI!" << std::endl;
        return false;
    }

    if (!editor.create())
    {
        std::cout << "Falha ao inicializar os recursos do Editor!" << std::endl;
        return false;
    }

    return buildWorld(scene);
}

void Application::run()
{
    double lastFrame = glfwGetTime();

    while (!window.shouldClose())
    {
        const double currentFrame = glfwGetTime();

        // Evita um salto grande de movimento se um quadro demorar demais.
        const float deltaTime = static_cast<float>(std::min(currentFrame - lastFrame, 0.1));
        lastFrame = currentFrame;

        update(deltaTime);
        renderer.render(scene, light, camera);
        renderUI();

        window.swapBuffers();
        window.pollEvents();
    }
}

void Application::update(float deltaTime)
{
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

    scene.update(deltaTime);
}

void Application::renderUI()
{
    // Inicia a interface gráfica.
    ui.begin(window);

    // Desenha o HUD e as informações da cena.
    hud.draw(ui, window, makeEnvironmentInfo(scene, camera, light, editor, selectedModelName));

    // Desenha o menu.
    menu.draw(window, ui, renderer, camera, light, [this]() { launchDoom(); });

    // Exibe as ferramentas somente no modo de edição
    // e quando o menu estiver fechado.
    if (editor.isEditing() && !menu.isOpen())
    {
        editor.drawGizmos();

        if (editor.isLightSelected())
            lightPanel.draw(window, ui, light);

        // Desenha a barra de ferramentas.
        editor.drawToolbar();
    }

    // Finaliza a interface gráfica.
    ui.end();
}

void Application::launchDoom()
{
#ifdef _WIN32
    wchar_t applicationPath[MAX_PATH] = {};
    const DWORD pathLength = GetModuleFileNameW(nullptr, applicationPath, MAX_PATH);
    if (pathLength == 0 || pathLength >= MAX_PATH)
    {
        MessageBoxW(nullptr, L"Não foi possível localizar a pasta do aplicativo.",
                    L"DOOM", MB_OK | MB_ICONERROR);
        return;
    }

    const std::filesystem::path applicationDirectory =
        std::filesystem::path(applicationPath).parent_path();
    const std::filesystem::path doomExecutable = applicationDirectory / L"DoomGeneric.exe";
    const std::filesystem::path doomWad = applicationDirectory / L"doom1.wad";

    std::error_code fileError;
    if (!std::filesystem::exists(doomExecutable, fileError) || fileError)
    {
        MessageBoxW(nullptr, L"DoomGeneric.exe nao foi encontrado. Recompile o projeto.",
                    L"DOOM", MB_OK | MB_ICONERROR);
        return;
    }

    fileError.clear();
    if (!std::filesystem::exists(doomWad, fileError) || fileError)
    {
        const std::wstring message =
            L"Coloque um arquivo doom1.wad obtido legalmente nesta pasta:\n\n" +
            applicationDirectory.wstring();
        MessageBoxW(nullptr, message.c_str(), L"DOOM - WAD necessário",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    const std::wstring commandText =
        L"\"" + doomExecutable.wstring() + L"\" -iwad doom1.wad";
    std::vector<wchar_t> commandLine(commandText.begin(), commandText.end());
    commandLine.push_back(L'\0');

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};
    const std::wstring workingDirectory = applicationDirectory.wstring();

    if (!CreateProcessW(doomExecutable.c_str(), commandLine.data(), nullptr, nullptr,
                        FALSE, 0, nullptr, workingDirectory.c_str(),
                        &startupInfo, &processInfo))
    {
        const DWORD error = GetLastError();
        const std::wstring message =
            L"Não foi possível iniciar DoomGeneric.exe. Erro do Windows: " +
            std::to_wstring(error);
        MessageBoxW(nullptr, message.c_str(), L"DOOM", MB_OK | MB_ICONERROR);
        return;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
#else
    std::cerr << "O lançamento do DOOM pelo menu só está disponível no Windows." << std::endl;
#endif
}
