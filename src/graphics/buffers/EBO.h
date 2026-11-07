#pragma once

#include <glad/glad.h>
#include <vector>

// Armazena índices que permitem reutilizar vértices ao desenhar triângulos.
// O buffer OpenGL só é criado no primeiro uso, então a classe pode ser construída antes da janela.
class EBO
{
private:
    mutable GLuint ID = 0;
    GLsizei indexCount = 0;

    void ensureCreated() const;

public:
    EBO() = default;
    ~EBO();

    EBO(const EBO&) = delete;
    EBO& operator=(const EBO&) = delete;

    // Torna este buffer de índices parte do VAO ativo.
    void Bind() const;

    void Upload(
        const std::vector<unsigned int>& indices,
        GLenum usage = GL_STATIC_DRAW
    );

    // Libera o buffer OpenGL.
    void Delete();

    GLsizei GetCount() const
    {
        return indexCount;
    }
};