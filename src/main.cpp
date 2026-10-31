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

    if (!janela.criar("OpenGL - Triangulo 3D"))
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

    if (!texture.load(ASSETS_DIR "textures/teste_uv.png"))
        return -1;

    // 4. Shader da pirâmide com iluminação
    Shader shaderCamera;

    if (!shaderCamera.criarDeArquivos(
        ASSETS_DIR "shaders/cam.vert",
        ASSETS_DIR "shaders/cam.frag"))
        return -1;

    // 5. Shader da fonte de luz
    Shader shaderLuz;

    if (!shaderLuz.criarDeArquivos(
        ASSETS_DIR "shaders/light.vert",
        ASSETS_DIR "shaders/light.frag"))
    {
        std::cout << "Falha ao criar o shader da luz!" << std::endl;
        return -1;
    }

    // 6. Criar quadrado de teste
    Mesh quad;

    if (!quad.criarEbo(
        formas::quadradoTeste(),
        formas::indicesQuadradoTeste(),
        3,
        GL_TRIANGLES,
        {3, 3, 2, 3}))
    {
        std::cout << "Erro ao criar o quadrado!" << std::endl;
        return -1;
    }

    // 7. Ativar profundidade
    glEnable(GL_DEPTH_TEST);

    // 8. Criar menu
    Menu menu;

    if (!menu.criar())
        return -1;

    // 9. Configurar câmera
    Camera camera(
        janela.getLargura(),
        janela.getAltura(),
        glm::vec3(0.0f, 0.0f, 2.0f)
    );

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
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

    // 12. Loop principal
    while (!janela.deveFechar())
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        menu.atualizar(janela);

        camera.Inputs(janela.ponteiro(), deltaTime);

        // Atualizar rotação
        rotation += 90.0f * deltaTime;

        glm::mat4 model(1.0f);

        model = glm::translate(model, pyramidPos);

        model = glm::rotate(
            model,
            glm::radians(rotation),
            glm::vec3(0.0f, 1.0f, 0.0f));

        // Limpar tela
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

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

        // 15. Desenhar menu
        glDisable(GL_DEPTH_TEST);

        menu.desenhar(janela);

        glEnable(GL_DEPTH_TEST);

        // Atualizar janela
        janela.trocarBuffers();
        janela.processarEventos();
    }

    // 16. Liberar recursos
    piramide.limpar();
    luz.limpar();
    quad.limpar();

    return 0;
}
