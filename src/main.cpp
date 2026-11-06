
#include "window.h"

#include "graphics/shader.h"
#include "graphics/mesh.h"
#include "graphics/formas.h"
#include "graphics/texture.h"

#include "camera.h"
#include "settings.h"

#include "scene/scene.h"

#include "ui/menu.h"
#include "ui/HUD.h"

#include <utility>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

int main()
{
    // 1. Criar janela
    Window janela;

    if (!janela.criar("OpenGL - RotorGrafico"))
        return -1;

    Texture defaultTexture;

    if (!defaultTexture.load(ASSETS_DIR "textures/default.png"))
        return -1;

    // 3. Criar pirâmide
    auto piramide = std::make_shared<Mesh>();
    if (!piramide->criarEbo(
            formas::piramide(),
            formas::indicesPiramide(),
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
    piramide->adicionarTextura(std::move(texture));
    piramide->adicionarTextura(std::move(specular));
    // 5. Shader da pirâmide e do macaco
    Shader shaderCamera;

    if (!shaderCamera.criarDeArquivos(
            ASSETS_DIR "shaders/default.vert",
            ASSETS_DIR "shaders/default.frag"))
    {
        std::cout << "Falha ao criar o shader da câmera!" << std::endl;
        return -1;
    }

    shaderCamera.usar();
    shaderCamera.setInt("tex0", 0);
    shaderCamera.setInt("specular0", 1);

    // 6. Shader da fonte de luz
    Shader shaderLuz;

    if (!shaderLuz.criarDeArquivos(
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

    Scene::Object *pyramid = scene.addMesh(piramide, "piramide");

    // 9. Configurar câmera
    Camera camera(
        janela.getLargura(),
        janela.getAltura(),
        glm::vec3(0.0f, 0.0f, 2.0f));

    float deltaTime = 0.0f;
    double lastFrame = 0.0f;
    float rotation = 0.0f;

    // 10. Criar malha da luz
    Mesh luz;

    if (!luz.criarEbo(
            formas::cubo(),
            formas::indicesCubo(),
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

    glm::vec3 pyramidPos(0.0f, 0.0f, 0.0f);

    lastFrame = glfwGetTime();

    // 12. Loop principal
    while (!janela.deveFechar())
    {
        double currentFrame = glfwGetTime();

        deltaTime = currentFrame - lastFrame;

        // Limita o deltaTime a 0.1 segundos
        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        lastFrame = currentFrame;

        // Atualizar HUD
        hud.update(janela, deltaTime);

        // Atualizar menu
        menu.atualizar(janela);

        // Controlar câmera conforme estado do menu
        if (menu.estaAberto())
        {
            janela.setCursorMode(Window::CursorMode::Normal);
        }
        else
        {
            camera.Inputs(janela, deltaTime);
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
        camera.setSize(
            janela.getLargura(),
            janela.getAltura());

        // Aplicar configurações do menu
        camera.walkSpeed = settings.walkSpeed;
        camera.sprintSpeed = settings.sprintSpeed;
        camera.sensitivity = settings.mouseSensitivity;

        // 13. Desenhar pirâmide e macaco
        shaderCamera.usar();

        shaderCamera.setVec4(
            "lightColor",
            lightColor.x,
            lightColor.y,
            lightColor.z,
            lightColor.w);

        shaderCamera.setVec3(
            "lightPos",
            lightPos.x,
            lightPos.y,
            lightPos.z);

        shaderCamera.setVec3(
            "camPos",
            camera.Position.x,
            camera.Position.y,
            camera.Position.z);

        shaderCamera.setInt("lightType", static_cast<int>(settings.lightType));

        camera.Matrix(
            settings.fov,
            0.1f,
            100.0f,
            shaderCamera,
            "camMatrix");

        // Desenhar os modelos da cena
        scene.draw(shaderCamera, camera, defaultTexture);
        // 14. Desenhar cubo da luz
        shaderLuz.usar();

        shaderLuz.setVec4(
            "lightColor",
            lightColor.x,
            lightColor.y,
            lightColor.z,
            lightColor.w);

        shaderLuz.setMat4("model", lightModel);

        camera.Matrix(
            settings.fov,
            0.1f,
            100.0f,
            shaderLuz,
            "camMatrix");

        luz.desenhar();

        // 15. Interface por cima de tudo
        ui.begin(janela);

        hud.draw(ui, camera.Position);

        menu.desenhar(
            janela,
            ui,
            settings);

        ui.end();

        // 16. Atualizar janela
        janela.trocarBuffers();
        janela.processarEventos();
    }

    return 0;
}
