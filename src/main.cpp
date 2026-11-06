
#include "window.h"

#include "graphics/shader.h"
#include "graphics/mesh.h"
#include "graphics/shapes.h"
#include "graphics/texture.h"

#include "camera.h"
#include "settings.h"

#include "scene/scene.h"

#include "ui/menu.h"
#include "ui/HUD.h"

#include <utility>
#include <memory>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

int main()
{
    // 1. Criar janela
    Window window;

    if (!window.create("OpenGL - RotorGrafico"))
        return -1;

    Texture defaultTexture;

    if (!defaultTexture.load(ASSETS_DIR "textures/default.png"))
        return -1;

    // 3. Criar pirâmide
    auto pyramidMesh = std::make_shared<Mesh>();
    if (!pyramidMesh->createIndexed(
            shapes::pyramid(),
            shapes::pyramidIndices(),
            3,
            GL_TRIANGLES,
            {3, 3, 2, 3}))
    {
        std::cout << "Falha ao criar a piramide!" << std::endl;
        return -1;
    }

    // 4. Carregar textura
    Texture texture;

    if (!texture.load(ASSETS_DIR "textures/terra.png"))
        return -1;

    Texture specular;

    if (!specular.load(ASSETS_DIR "textures/terraSpecular.png"))
        return -1;

    // Identificar os tipos das texturas
    texture.setType("diffuse");
    specular.setType("specular");

    // Transferir as texturas para a Mesh
    pyramidMesh->addTexture(std::move(texture));
    pyramidMesh->addTexture(std::move(specular));
    // 5. Shader da pirâmide e do macaco
    Shader cameraShader;

    if (!cameraShader.createFromFiles(
            ASSETS_DIR "shaders/default.vert",
            ASSETS_DIR "shaders/default.frag"))
    {
        std::cout << "Falha ao criar o shader da câmera!" << std::endl;
        return -1;
    }

    cameraShader.use();
    cameraShader.setInt("tex0", 0);
    cameraShader.setInt("specular0", 1);

    // 6. Shader da fonte de luz
    Shader lightShader;

    if (!lightShader.createFromFiles(
            ASSETS_DIR "shaders/light.vert",
            ASSETS_DIR "shaders/light.frag"))
    {
        std::cout << "Falha ao criar o shader da luz!" << std::endl;
        return -1;
    }

    // 7. Ativar profundidade
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // 8. Criar interface, menu e HUD
    UI ui;

    if (!ui.create())
    {
        std::cout << "Falha ao inicializar a UI!" << std::endl;
        return -1;
    }

    Menu menu;
    Settings settings;
    HUD hud;

    Scene scene;

    if (Scene::Object *monkey = scene.addModel(ASSETS_DIR "models/macaco.glb"))
    {
        monkey->transform.position = glm::vec3(1.5f, 0.0f, 0.0f);
        monkey->transform.scale = glm::vec3(0.5f);
    }

    Scene::Object *pyramid = scene.addMesh(pyramidMesh, "pyramid");

    // 9. Configurar câmera
    Camera camera(
        window.getWidth(),
        window.getHeight(),
        glm::vec3(0.0f, 0.0f, 2.0f));

    float deltaTime = 0.0f;
    double lastFrame = 0.0f;
    float rotation = 0.0f;

    // 10. Criar malha da luz
    Mesh lightMesh;

    if (!lightMesh.createIndexed(
            shapes::cube(),
            shapes::cubeIndices(),
            3,
            GL_TRIANGLES,
            {3}))
    {
        std::cout << "Erro ao criar a malha da luz." << std::endl;
        return -1;
    }

    // 11. Configuração da iluminação
    glm::vec4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos(0.5f, 0.5f, 0.5f);

    glm::mat4 lightModel(1.0f);

    lightModel = glm::translate(lightModel, lightPos);
    lightModel = glm::scale(lightModel, glm::vec3(0.2f));


    lastFrame = glfwGetTime();

    // 12. Loop principal
    while (!window.shouldClose())
    {
        double currentFrame = glfwGetTime();

        deltaTime = currentFrame - lastFrame;

        // Limita o deltaTime a 0.1 segundos
        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        lastFrame = currentFrame;

        // Atualizar HUD
        hud.update(window, deltaTime);

        // Atualizar menu
        menu.update(window);

        // Controlar câmera conforme estado do menu
        if (menu.isOpen())
        {
            window.setCursorMode(Window::CursorMode::Normal);
        }
        else
        {
            camera.processInput(window, deltaTime);
        }

        // Atualizar rotação da pirâmide
        rotation += 90.0f * deltaTime;
        pyramid->transform.rotation.y = rotation;

        // Limpar a tela e o buffer de profundidade
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

        // Atualizar tamanho da câmera
        camera.setViewportSize(
            window.getWidth(),
            window.getHeight());

        // Aplicar configurações do menu
        camera.walkSpeed = settings.walkSpeed;
        camera.sprintSpeed = settings.sprintSpeed;
        camera.sensitivity = settings.mouseSensitivity;

        // 13. Desenhar pirâmide e macaco
        cameraShader.use();

        cameraShader.setVec4(
            "lightColor",
            lightColor.x,
            lightColor.y,
            lightColor.z,
            lightColor.w);

        cameraShader.setVec3(
            "lightPos",
            lightPos.x,
            lightPos.y,
            lightPos.z);

        cameraShader.setVec3(
            "camPos",
            camera.position.x,
            camera.position.y,
            camera.position.z);

        cameraShader.setInt("lightType", static_cast<int>(settings.lightType));

        camera.setViewProjection(
            settings.fov,
            0.1f,
            100.0f,
            cameraShader,
            "camMatrix");

        // Desenhar os modelos da cena
        scene.draw(cameraShader, camera, defaultTexture);
        // 14. Desenhar cubo da luz
        lightShader.use();

        lightShader.setVec4(
            "lightColor",
            lightColor.x,
            lightColor.y,
            lightColor.z,
            lightColor.w);

        lightShader.setMat4("model", lightModel);

        camera.setViewProjection(
            settings.fov,
            0.1f,
            100.0f,
            lightShader,
            "camMatrix");

        lightMesh.draw();

        // 15. Interface por cima de tudo
        ui.begin(window);

        hud.draw(ui, camera.position);

        menu.draw(
            window,
            ui,
            settings);

        ui.end();

        // 16. Atualizar janela
        window.swapBuffers();
        window.pollEvents();
    }

    return 0;
}
