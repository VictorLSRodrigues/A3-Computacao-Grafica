#include "window.h"
#include "shader.h"
#include "mesh.h"
#include "formas.h"
#include "menu.h"
#include "texture.h"
#include "camera.h"

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

int main()
{
    // 1. Criar janela
    Window janela;

    if (!janela.criar("OpenGL - RotorGrafico"))
        return -1;

    // 2. Criar pirâmide
    Mesh piramide;

    if (!piramide.criarEbo(
            formas::piramide(),
            formas::indicesPiramide(),
            3,
            GL_TRIANGLES,
            {3, 3, 2, 3}))
    {
        std::cout << "Falha ao criar a piramide!" << std::endl;
        return -1;
    }

    // 3. Carregar textura
    Texture texture;

    if (!texture.load(ASSETS_DIR "textures/terra.png"))
        return -1;

    Texture specular;
    if (!specular.load(ASSETS_DIR "textures/terraSpecular.png"))
        return -1;

    // 4. Shader da pirâmide com iluminação
    Shader shaderCamera;

    if (!shaderCamera.criarDeArquivos(
            ASSETS_DIR "shaders/default.vert",
            ASSETS_DIR "shaders/default.frag"))
        return -1;

    shaderCamera.usar();
    shaderCamera.setInt("tex0", 0);
    shaderCamera.setInt("specular0", 1);

    // 5. Shader da fonte de luz
    Shader shaderLuz;

    if (!shaderLuz.criarDeArquivos(
            ASSETS_DIR "shaders/light.vert",
            ASSETS_DIR "shaders/light.frag"))
    {
        std::cout << "Falha ao criar o shader da luz!" << std::endl;
        return -1;
    }

    // 6. Ativar profundidade
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // 7. Criar menu
    Menu menu;

    if (!menu.criar())
        return -1;

    // 8. Configurar câmera
    Camera camera(
        janela.getLargura(),
        janela.getAltura(),
        glm::vec3(0.0f, 0.0f, 2.0f));

    float deltaTime = 0.0f;
    double lastFrame = 0.0f;
    float rotation = 0.0f;

    // 9. Criar malha da luz
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

    // 10. Configuração da iluminação
    glm::vec4 lightColor(1.0f, 1.0f, 1.0f, 1.0f);
    glm::vec3 lightPos(0.5f, 0.5f, 0.5f);

    glm::mat4 lightModel(1.0f);
    lightModel = glm::translate(lightModel, lightPos);
    lightModel = glm::scale(lightModel, glm::vec3(0.2f));

    glm::vec3 pyramidPos(0.0f, 0.0f, 0.0f);

    lastFrame = glfwGetTime();
    // 11. Loop principal
    while (!janela.deveFechar())
    {
        double currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        // Limita o deltaTime a 0.1 segundos
        if (deltaTime > 0.1f)
            deltaTime = 0.1f;
        lastFrame = currentFrame;

        menu.atualizar(janela);
        if (menu.estaAberto())
            janela.setCursorMode(Window::CursorMode::Normal);
        else
            camera.Inputs(janela, deltaTime);

        // Atualizar rotação
        rotation += 90.0f * deltaTime;

        glm::mat4 model(1.0f);

        model = glm::translate(model, pyramidPos);

        model = glm::rotate(
            model,
            glm::radians(rotation),
            glm::vec3(0.0f, 1.0f, 0.0f));

        // Limpar a tela e o buffer de profundidade
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

        camera.setSize(janela.getLargura(), janela.getAltura());

        // 13. Desenhar pirâmide
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

        camera.Matrix(
            45.0f,
            0.1f,
            100.0f,
            shaderCamera,
            "camMatrix");

        shaderCamera.setMat4("model", model);

        texture.bind(0);
        specular.bind(1);
        piramide.desenhar();

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
            45.0f,
            0.1f,
            100.0f,
            shaderLuz,
            "camMatrix");

        luz.desenhar();

        // 15. Desenhar menu (por cima de tudo)
        menu.desenhar(janela);

        // Atualizar janela
        janela.trocarBuffers();
        janela.processarEventos();
    }

    // 16. Liberar recursos
    piramide.limpar();

    return 0;
}
