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
    // componentes: quantos números cada vértice tem (3 = x,y,z | 4 = x,y,z,w)
    // modoDesenho: GL_TRIANGLES, GL_LINES, GL_POINTS, GL_LINE_LOOP...
    bool criar(const std::vector<float>& vertices,
               int componentes = 3,
               GLenum modoDesenho = GL_TRIANGLES);
    void desenhar() const;
    void limpar();
};