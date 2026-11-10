#include "application.h"

#include <algorithm>
#include <iostream>

#include "scene/world.h"

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
        // Alterna entre o modo de navegação e o modo de edição.
        if (window.keyPressed(GLFW_KEY_TAB))
        {
            editingMode = !editingMode;

            if (editingMode)
                std::cout << "Modo de edicao ativado!" << std::endl;
            else
                std::cout << "Modo de navegacao ativado!" << std::endl;
        }

        camera.processInput(window, deltaTime);

        if (!editingMode)
        {
            if (window.keyPressed(GLFW_KEY_1))
            {
                if (scene.showOnly("macaco"))
                    selectedModelName = "macaco";
                else
                    std::cout << "Objeto macaco nao encontrado na cena." << std::endl;
            }
            else if (window.keyPressed(GLFW_KEY_2))
            {
                if (scene.showOnly("multi"))
                    selectedModelName = "multi";
                else
                    std::cout << "Objeto multi nao encontrado na cena." << std::endl;
            }
            else if (window.keyPressed(GLFW_KEY_3))
            {
                if (scene.showOnly("pyramid"))
                    selectedModelName = "pyramid";
                else
                    std::cout << "Objeto pyramid nao encontrado na cena." << std::endl;
            }
        }

        if (window.keyPressed(GLFW_KEY_0))
        {
            scene.showAll();
            selectedModelName = "Todos";
        }
    }
    // Copia para a luz e para a câmera os ajustes escolhidos no menu.
    light.position = settings.lightPosition;
    light.target = settings.lightTarget;

    camera.walkSpeed = settings.walkSpeed;
    camera.sprintSpeed = settings.sprintSpeed;
    camera.sensitivity = settings.mouseSensitivity;
    camera.setViewportSize(window.getWidth(), window.getHeight());

    scene.update(deltaTime);
}

// Desenha as ferramentas de transformação no canto superior direito.
void Application::drawTransformToolbar()
{
    const float buttonSize = 46.0f;
    const float gap = 7.0f;
    const float margin = 20.0f;

    const float totalWidth = buttonSize * 3.0f + gap * 2.0f;

    const float x = static_cast<float>(window.getWidth()) - totalWidth - margin;
    const float y = margin;

    // Botão Mover.
    if (ui.iconButton(
            UI::Icon::Move,
            "Mover",
            x, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Move))
    {
        activeTransformTool = TransformTool::Move;
    }

    // Botão Rotacionar.
    if (ui.iconButton(
            UI::Icon::Rotate,
            "Rotacionar",
            x + buttonSize + gap, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Rotate))
    {
        activeTransformTool = TransformTool::Rotate;
    }

    // Botão Escalar.
    if (ui.iconButton(
            UI::Icon::Scale,
            "Escalar",
            x + (buttonSize + gap) * 2.0f, y,
            buttonSize, buttonSize,
            activeTransformTool == TransformTool::Scale))
    {
        activeTransformTool = TransformTool::Scale;
    }
}

void Application::renderUI()
{
    ui.begin(window);
    hud.draw(ui, camera.position);
    hud.textmodel(ui, selectedModelName);
    menu.draw(window, ui, settings);

      // Mostra as ferramentas somente no modo de edição.
      if (editingMode && !menu.isOpen())
        drawTransformToolbar();

    ui.end();
}