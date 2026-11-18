#include "application.h"

#include <algorithm>
#include <iostream>

#include "applications/sandbox/ui/environment_info.h"
#include "world.h"

bool Application::init()
{
    if (!window.create("OpenGL - RotorGrafico"))
        return false;

    if (!renderer.init())
        return false;

    if (!light.create())
    {
        std::cout << "Erro ao criar a malha da luz." << std::endl;
        return false;
    }

    if (!ui.create())
    {
        std::cout << "Falha ao inicializar a UI!" << std::endl;
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
        renderer.render(scene, light, camera, settings);
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

    // Copia para a luz e para a câmera os ajustes escolhidos no menu.
    light.position = settings.lightPosition;
    light.target = settings.lightTarget;
    light.color = glm::vec4(settings.lightColor, 1.0f);

    camera.walkSpeed = settings.walkSpeed;
    camera.sprintSpeed = settings.sprintSpeed;
    camera.sensitivity = settings.mouseSensitivity;
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
    hud.draw(ui, makeEnvironmentInfo(scene, camera, light, settings, editor, selectedModelName));

    // Desenha o menu.
    menu.draw(window, ui, settings);

    // Exibe as ferramentas somente no modo de edição
    // e quando o menu estiver fechado.
    if (editor.isEditing() && !menu.isOpen())
    {
        editor.drawGizmos();

        if (editor.isLightSelected())
            lightPanel.draw(window, ui, settings);

        // Desenha a barra de ferramentas.
        editor.drawToolbar();
    }

    // Finaliza a interface gráfica.
    ui.end();
}
