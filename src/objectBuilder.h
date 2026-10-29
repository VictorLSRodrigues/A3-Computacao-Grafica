#pragma once

#include <glad/glad.h>

class ObjectBuilder
{
private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint shaderProgram = 0;

    bool inicializado = false;

public:
    bool inicializar();
    void desenhar();
    void limpar();
};