#pragma once

#include <glad/glad.h>
#include <vector>

#include "../vertex.h"

// Armazena na placa de vídeo os dados dos vértices usados para desenhar uma malha.
class VBO
{
private:
    GLuint ID = 0;

public:
    VBO();
    ~VBO();

    VBO(const VBO&) = delete;
    VBO& operator=(const VBO&) = delete;

    // Torna este buffer o destino das operações sobre vértices.
    void Bind() const;
    void Unbind() const;

    // Envia vértices descritos como uma lista de números.
    void Upload(
        const std::vector<float>& vertices,
        GLenum usage = GL_STATIC_DRAW
    );

    // Envia vértices descritos pela estrutura Vertex.
    void Upload(
        const std::vector<Vertex>& vertices,
        GLenum usage = GL_STATIC_DRAW
    );

    // Libera o buffer OpenGL.
    void Delete();
};