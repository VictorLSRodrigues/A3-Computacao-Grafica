#include <glad/glad.h>  // carrega as funções do OpenGL
#include <GLFW/glfw3.h> // cria a janela e lê eventos
#include <iostream>     // para escrever no terminal (std::cout)
#include "shader.h"
#include "mesh.h"
#include <cmath>

// Código dos shaders (mini-programas que rodam na placa de vídeo).
// Por enquanto ficam como texto aqui; depois podem ir para arquivos.

// Vertex shader: roda para cada vértice e decide onde ele fica na tela.
const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos, 1.0);
}
)";

// Fragment shader: roda para cada pixel e decide a cor (aqui, laranja).
const char *fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 1.0);
}
)";

// Chamada automaticamente pelo GLFW quando a janela muda de tamanho.
// Ajusta a área onde o OpenGL desenha para acompanhar a janela.
void framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

int main()
{
    // Liga o GLFW. Se falhar, não adianta continuar.
    if (!glfwInit())
    {
        std::cout << "Erro ao inicializar o GLFW!" << std::endl;
        return -1;
    }

    // Pede OpenGL 3.3 no perfil Core.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Descobre o monitor principal e a resolução dele.
    // Passando o monitor no glfwCreateWindow, a janela abre em tela cheia.
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "OpenGL", monitor, nullptr);

    // Se a janela não foi criada, o ponteiro vem nulo.
    if (!window)
    {
        std::cout << "Erro ao criar a janela!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Diz ao OpenGL que vai desenhar nessa janela.
    glfwMakeContextCurrent(window);

    // Carrega as funções do OpenGL. Sem isso, os comandos gl... não funcionam.
    if (!gladLoadGL())
    {
        std::cout << "Erro ao inicializar o GLAD!" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Registra a função de redimensionar.
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    // Mostra no terminal qual versão do OpenGL está rodando.
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    // O Shader diz COMO desenhar (cor e posição).
    Shader shader;
    if (!shader.criar(vertexShaderSource, fragmentShaderSource))
    {
        std::cout << "Falha ao criar o shader!" << std::endl;
        return -1;
    }

    // Os 3 pontos do triângulo (x, y, z). A tela vai de -1 a +1.
    // O sqrt(3) deixa o triângulo equilátero.
    std::vector<float> verticesTriangulo = {
        -0.5f, -0.5f * float(std::sqrt(3)) / 3, 0.0f,
        0.5f, -0.5f * float(std::sqrt(3)) / 3, 0.0f,
        0.0f, 0.5f * float(std::sqrt(3)) / 2, 0.0f};

    // A Mesh diz O QUE desenhar (a forma) e guarda os pontos na placa de vídeo.
    Mesh triangulo;
    if (!triangulo.criar(verticesTriangulo))
    {
        std::cout << "Falha ao criar o triangulo!" << std::endl;
        return -1;
    }

    // Loop principal: roda uma vez por quadro até a janela ser fechada.
    while (!glfwWindowShouldClose(window))
    {
        // Pinta o fundo (r, g, b, transparência) e limpa a tela.
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Escolhe o shader e desenha a forma com ele.
        shader.usar();
        triangulo.desenhar();

        // Mostra o quadro pronto e processa cliques, teclas, etc.
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza ao sair: primeiro o que mora na placa de vídeo,
    // depois a janela (senão o OpenGL já não existe mais).
    triangulo.limpar();
    shader.limpar();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}