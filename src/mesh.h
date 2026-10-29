#pragma once

#include <glad/glad.h>
#include <vector>

class Mesh
{
private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLsizei quantidadeVertices = 0;

public:
    bool criar(const std::vector<float>& vertices);
    void desenhar() const;
    void limpar();
};