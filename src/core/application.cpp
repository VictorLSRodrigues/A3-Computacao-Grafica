#include "application.h"
#include "scene/world.h"
#include <algorithm>
#include <iostream>

bool Application::init()
{
    if (!window.create("OpenGL - RotorGrafico"))
        return false;

    if (!defaultTexture.load(ASSETS_DIR "textures/default.png"))
        return false;

    if (!loadShaders())
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

    if (!buildWorld(scene))

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    return true;
}

bool Application::loadShaders()
{
    if (!sceneShader.createFromFiles(
            ASSETS_DIR "shaders/default.vert",
            ASSETS_DIR "shaders/default.frag"))
    {
        std::cout << "Falha ao criar o shader da cena!" << std::endl;
        return false;
    }

    sceneShader.use();
    sceneShader.setInt("tex0", 0);
    sceneShader.setInt("specular0", 1);

    if (!lightShader.createFromFiles(
            ASSETS_DIR "shaders/light.vert",
            ASSETS_DIR "shaders/light.frag"))
    {
        std::cout << "Falha ao criar o shader da luz!" << std::endl;
        return false;
    }

    return true;
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
        render();
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
        window.setCursorMode(Window::CursorMode::Normal);
    else
        camera.processInput(window, deltaTime);

    light.position = settings.lightPosition;
    light.target = settings.lightTarget;
    camera.walkSpeed = settings.walkSpeed;
    camera.sprintSpeed = settings.sprintSpeed;
    camera.sensitivity = settings.mouseSensitivity;
    camera.setViewportSize(window.getWidth(), window.getHeight());
    scene.update(deltaTime);
}

void Application::render()
{
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sceneShader.use();
    light.apply(sceneShader, settings.lightType, camera);
    camera.setViewProjection(settings.fov, 0.1f, 100.0f, sceneShader, "camMatrix");
    scene.draw(sceneShader, camera, defaultTexture);

    light.drawMarker(lightShader, camera, settings.fov);
}

void Application::renderUI()
{
    ui.begin(window);
    hud.draw(ui, camera.position);
    menu.draw(window, ui, settings);
    ui.end();
}