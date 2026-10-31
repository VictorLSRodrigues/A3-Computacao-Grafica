
#include "window.h"
#include "shader.h"
#include "mesh.h"
#include "formas.h"
#include "menu.h"
#include "texture.h"

#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

int main()
{
    // 1. Criar janela
    Window janela;

    if (!janela.criar("OpenGL - Triangulo 3D"))
        return -1;

    // 2. Criar shader
    Shader shader;

    if (!shader.criarDeArquivos(
            ASSETS_DIR "shaders/basic.vert",
            ASSETS_DIR "shaders/basic.frag"))
    {
        std::cout << "Falha ao criar o shader!" << std::endl;
        return -1;
    }

    // 3. Criar triangulo
    Mesh triangulo;

    // 2D
    if (!triangulo.criar(formas::triangulo(1.0f), 3))
    {
        std::cout << "Falha ao criar o triangulo!" << std::endl;
        return -1;
    }

    Mesh piramide;
    // 3Dgit
    if (!piramide.criarEbo(
            formas::piramide(),
            formas::indicesPiramide(),
            3,
            GL_TRIANGLES,
            {3, 3, 2}))
    {
        std::cout << "Falha ao criar a piramide!" << std::endl;
        return -1;
    }

    Texture texture;
    if (!texture.load(ASSETS_DIR "textures/teste_uv.png"))
        return -1;

    Shader shaderTextured;
    if (!shaderTextured.criarDeArquivos(ASSETS_DIR "shaders/textured.vert",
                                        ASSETS_DIR "shaders/texture_test.frag"))
        return -1;

    // Quadrado de teste: posição (3) + normal (3) + UV (2) por vértice.
    std::vector<float> quadVertices = {
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f};
    std::vector<unsigned int> quadIndices = {0, 1, 2, 0, 2, 3};

    Mesh quad;
    if (!quad.criarEbo(quadVertices, quadIndices, 3, GL_TRIANGLES, {3, 3, 2}))
        return -1;

    // 4. Criar matrizes
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);

    view = glm::translate(
        view,
        glm::vec3(0.0f, 0.0f, -2.5f));

    proj = glm::perspective(
        glm::radians(45.0f),
        800.0f / 800.0f,
        0.1f,
        100.0f);

    // 5. Ativar profundidade
    glEnable(GL_DEPTH_TEST);

    // 6. Criar menu
    Menu menu;

    if (!menu.criar())
        return -1;

    // Controle de rotacao
    float rotation = 0.0f;
    double prevTime = glfwGetTime();

    // 7. Loop principal
    while (!janela.deveFechar())
    {
        menu.atualizar(janela);

        // Controle de tempo
        double currentTime = glfwGetTime();

        float deltaTime = static_cast<float>(
            currentTime - prevTime);

        prevTime = currentTime;

        // Atualizar rotacao
        rotation += 90.0f * deltaTime;

        // Matriz do triangulo
        model = glm::mat4(1.0f);

        model = glm::rotate(
            model,
            glm::radians(rotation),
            glm::vec3(0.0f, 1.0f, 0.0f));

        // 8. Limpar tela
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

        // 9. Ativar shader
        shaderTextured.usar();

        shaderTextured.setMat4("model", model);
        shaderTextured.setMat4("view", view);
        shaderTextured.setMat4("proj", proj);

        texture.bind(0);
        piramide.desenhar();

        // Desenhar menu
        glDisable(GL_DEPTH_TEST);

        menu.desenhar(janela);

        glEnable(GL_DEPTH_TEST);

        // Atualizar janela
        janela.trocarBuffers();
        janela.processarEventos();
    }

    // 10. Liberar recursos
    piramide.limpar();

    return 0;
}
