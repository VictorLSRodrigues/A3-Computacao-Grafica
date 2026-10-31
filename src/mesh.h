#pragma once

#include <glad/glad.h>
#include <vector>

class Mesh
{
private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    GLsizei quantidadeVertices = 0;
    GLsizei quantidadeIndices = 0;
    GLenum modo = GL_TRIANGLES;
    bool usaIndices = false;
public:
    Mesh() = default;
    ~Mesh() { limpar(); }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    bool criar(const std::vector<float>& vertices,
               int componentes = 3,
               GLenum modoDesenho = GL_TRIANGLES);
                          
    bool criarEbo(
        const std::vector<float>& vertices,
        const std::vector<unsigned int>& indices,
        int componentes = 3,
        GLenum modoDesenho = GL_TRIANGLES
    );
    void desenhar() const;
    void limpar();
};