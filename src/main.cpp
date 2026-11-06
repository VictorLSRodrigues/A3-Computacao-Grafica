
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
    // Inicializa a janela e os recursos necessários para renderizar.
    Window window;

    if (!window.create("OpenGL - RotorGrafico"))
        return -1;

    Texture defaultTexture;

    if (!defaultTexture.load(ASSETS_DIR "textures/default.png"))
        return -1;

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
        return -1;
    }

    // Carrega e associa as texturas usadas pela pirâmide.
    Texture texture;

    if (!texture.load(ASSETS_DIR "textures/terra.png"))
        return -1;

    Texture specular;

    if (!specular.load(ASSETS_DIR "textures/terraSpecular.png"))
        return -1;

    texture.setType("diffuse");
    specular.setType("specular");

    pyramidMesh->addTexture(std::move(texture));
    pyramidMesh->addTexture(std::move(specular));

    // O shader principal desenha os objetos da cena com iluminação e textura.
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

    // Este shader desenha a representação visual da fonte de luz.
    Shader lightShader;

    if (!lightShader.createFromFiles(
            ASSETS_DIR "shaders/light.vert",
            ASSETS_DIR "shaders/light.frag"))
    {
        std::cout << "Falha ao criar o shader da luz!" << std::endl;
        return -1;
    }

    // Objetos próximos escondem os que estão atrás; faces voltadas para longe da câmera são descartadas.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Prepara a interface, o menu de opções e as informações de depuração.
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

    // Adiciona o modelo e a pirâmide à cena.
    if (Scene::Object *monkey = scene.addModel(ASSETS_DIR "models/macaco.glb"))
    {
        monkey->transform.position = glm::vec3(1.5f, 0.0f, 0.0f);
        monkey->transform.scale = glm::vec3(0.5f);
    }

    Scene::Object *pyramid = scene.addMesh(pyramidMesh, "pyramid");

    // A câmera começa em frente aos objetos.
    Camera camera(
        window.getWidth(),
        window.getHeight(),
        glm::vec3(0.0f, 0.0f, 2.0f));

    float deltaTime = 0.0f;
    double lastFrame = 0.0f;
    float rotation = 0.0f;

    // Um cubo simples torna a posição da luz visível na cena.
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

    // Cor e posição usadas pelo shader de iluminação.
    glm::vec4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos(0.5f, 0.5f, 0.5f);

    glm::mat4 lightModel(1.0f);

    lightModel = glm::translate(lightModel, lightPos);
    lightModel = glm::scale(lightModel, glm::vec3(0.2f));


    lastFrame = glfwGetTime();

    // Mantém a aplicação atualizando e desenhando até a janela ser fechada.
    while (!window.shouldClose())
    {
        double currentFrame = glfwGetTime();

        deltaTime = currentFrame - lastFrame;

        // Evita um salto grande de movimento se um quadro demorar demais.
        if (deltaTime > 0.1f)
            deltaTime = 0.1f;

        lastFrame = currentFrame;

        // Atualiza informações e controles da interface.
        hud.update(window, deltaTime);

        menu.update(window);

        // A câmera só recebe controles quando o menu está fechado.
        if (menu.isOpen())
        {
            window.setCursorMode(Window::CursorMode::Normal);
        }
        else
        {
            camera.processInput(window, deltaTime);
        }

        // Anima a rotação da pirâmide usando o tempo do quadro.
        rotation += 90.0f * deltaTime;
        pyramid->transform.rotation.y = rotation;

        // Começa um novo quadro, limpando a imagem anterior e a profundidade.
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

        // Mantém a projeção da câmera compatível com o tamanho atual da janela.
        camera.setViewportSize(
            window.getWidth(),
            window.getHeight());

        // Copia para a câmera os ajustes escolhidos no menu.
        camera.walkSpeed = settings.walkSpeed;
        camera.sprintSpeed = settings.sprintSpeed;
        camera.sensitivity = settings.mouseSensitivity;

        // Envia os parâmetros de iluminação e câmera, depois desenha os objetos da cena.
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

        scene.draw(cameraShader, camera, defaultTexture);

        // Desenha o cubo que indica a posição da fonte de luz.
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

        // Desenha HUD e menu por cima da cena 3D.
        ui.begin(window);

        hud.draw(ui, camera.position);

        menu.draw(
            window,
            ui,
            settings);

        ui.end();

        // Mostra o quadro pronto e processa eventos de teclado e mouse.
        window.swapBuffers();
        window.pollEvents();
    }

    return 0;
}
