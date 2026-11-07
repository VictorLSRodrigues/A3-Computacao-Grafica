#include "application.h"

#include <algorithm>
#include <iostream>
#include <utility>

#include "graphics/mesh.h"
#include "graphics/shapes.h"

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

    if (!loadScene())
        return false;

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

bool Application::loadScene()
{
    // Cada vértice guarda posição (3), cor (3), coordenadas da textura (2) e normal (3).
    auto pyramidMesh = std::make_shared<Mesh>();
    if (!pyramidMesh->createIndexed(
            shapes::pyramid(),
            shapes::pyramidIndices(),
            3,
            GL_TRIANGLES,
            {3, 3, 2, 3}))
    {
        std::cout << "Falha ao criar a piramide!" << std::endl;
        return false;
    }

    Texture diffuse;
    Texture specular;

    if (!diffuse.load(ASSETS_DIR "textures/terra.png") ||
        !specular.load(ASSETS_DIR "textures/terraSpecular.png"))
        return false;

    diffuse.setType("diffuse");
    specular.setType("specular");

    pyramidMesh->addTexture(std::move(diffuse));
    pyramidMesh->addTexture(std::move(specular));

    // Os modelos são opcionais: se algum falhar, a cena continua sem ele.
    if (Scene::Object *monkey = scene.addModel(ASSETS_DIR "models/macaco.glb"))
    {
        monkey->transform.position = glm::vec3(1.5f, 0.0f, 0.0f);
        monkey->transform.scale = glm::vec3(0.5f);
    }

    // Troque o nome do arquivo pelo do seu modelo.
    if (Scene::Object *multi = scene.addModel(ASSETS_DIR "models/multi.glb"))
    {
        const float multiScale = 0.3f; // ajuste aqui: menor = modelo menor
        multi->transform.position = glm::vec3(-1.5f, 0.0f, 0.0f);
        multi->transform.scale = glm::vec3(multiScale);
    }

    pyramid = scene.addMesh(pyramidMesh, "pyramid");
    return pyramid != nullptr;
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

    pyramidRotation += 90.0f * deltaTime;
    if (pyramid)
        pyramid->transform.rotation.y = pyramidRotation;
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