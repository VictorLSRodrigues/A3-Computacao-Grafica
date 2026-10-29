#include <glad/glad.h>   // carrega as funções do OpenGL
#include <GLFW/glfw3.h>  // cria a janela e lê eventos
#include <iostream>      // para escrever no terminal (std::cout)
#include <cmath>

// metodo que faz parte da criação de um triangulo 
const char* vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
" gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\n\0";

const char* fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
" FragColor = vec4(0.8f, 0.3f, 0.02f, 1.0f);\n"
"}\n\0";

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

    //metodos que fazem parte da criação de um triangulo 
    //metodos para a Interpretação de vertices, fragment shaders, programa de Shaders
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    //Vertice buffer Object
    GLuint VAO, VBO;

    //VAO antes do VBO
    glGenVertexArrays(1, &VAO); 
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3* sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); 
    glBindVertexArray(0);


    // Loop principal: roda uma vez por quadro até a janela ser fechada.
    while (!glfwWindowShouldClose(window))
    {
        // Pinta o fundo (r, g, b, transparência) e limpa a tela.
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        

        //metodo que faz parte da criação de um triangulo
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);


        // Mostra o quadro pronto e processa cliques, teclas, etc.
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //metodo que faz parte da criação de um triangulo 
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Limpeza ao sair.
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}