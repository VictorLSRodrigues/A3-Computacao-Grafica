#pragma once

#include <glad/glad.h>
#include <vector>

class Mesh
{
private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLsizei quantidadeVertices = 0;
    GLenum modo = GL_TRIANGLES;

public:
    Mesh() = default;
    ~Mesh() { limpar(); }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    bool criar(const std::vector<float>& vertices,
               int componentes = 3,
               GLenum modoDesenho = GL_TRIANGLES);
    void desenhar() const;
    void limpar();
};