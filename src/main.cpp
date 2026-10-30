#include "window.h"
#include "shader.h"
#include "mesh.h"
#include "formas.h"
#include "menu.h"
#include <iostream>

int main()
{
    Window janela;
    if (!janela.criar("OpenGL"))
        return -1;

    Shader shader;
    if (!shader.criarDeArquivos(ASSETS_DIR "shaders/basic.vert",
                                ASSETS_DIR "shaders/basic.frag"))
    {
        std::cout << "Falha ao criar o shader!" << std::endl;
        return -1;
    }

    Mesh triangulo;
    if (!triangulo.criar(formas::triangulo(1.0f)))
    {
        std::cout << "Falha ao criar o triangulo!" << std::endl;
        return -1;
    }

    Mesh retangulo;
    if (!retangulo.criar(formas::retangulo(), 2))
{
    std::cout << "Falha ao criar o retangulo!" << std::endl;
    return -1;
}

   Mesh quadrado;
    if (!quadrado.criar(formas::quadrado(), 2))
{
    std::cout << "Falha ao criar o retangulo!" << std::endl;
    return -1;
}
    Menu menu;
    if (!menu.criar())
        return -1;

    while (!janela.deveFechar())
    {
        menu.atualizar(janela);

        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.usar();
        quadrado.desenhar();

        menu.desenhar(janela); // por cima de tudo, só aparece se estiver aberto

        janela.trocarBuffers();
        janela.processarEventos();
    }

    return 0;
}