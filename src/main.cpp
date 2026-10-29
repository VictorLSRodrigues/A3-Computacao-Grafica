#include <glad/glad.h>   // carrega as funções do OpenGL
#include <GLFW/glfw3.h>  // cria a janela e lê eventos
#include <iostream>      // para escrever no terminal (std::cout)
#include "ObjectBuilder.h"
#include <cmath>


// Chamada automaticamente pelo GLFW quando a janela muda de tamanho.
// Ajusta a área onde o OpenGL desenha para acompanhar a janela.
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
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
    
    //metodo que faz parte da criação de um triangulo 
    GLfloat vertices[] = 
    {
        -0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f,    
        0.5f, -0.5f * float(sqrt(3)) / 3, 0.0f,
        0.0f, 0.5f *float(sqrt(3)) / 2, 0.0f
    };

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

    //Metodos para Ciação de Objetos
    ObjectBuilder triangulo;

    if (!triangulo.inicializar())
    {
    std::cout << "Falha ao inicializar o triangulo!" << std::endl;
    }


    // Loop principal: roda uma vez por quadro até a janela ser fechada.
    while (!glfwWindowShouldClose(window))
    {
        // Pinta o fundo (r, g, b, transparência) e limpa a tela.
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        

        //metodo que faz parte da criação de um triangulo
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        triangulo.desenhar();
        


        // Mostra o quadro pronto e processa cliques, teclas, etc.
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza ao sair.
    glfwDestroyWindow(window);
    glfwTerminate();
    triangulo.limpar();

    return 0;
}