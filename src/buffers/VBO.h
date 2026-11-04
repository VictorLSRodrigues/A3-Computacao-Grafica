#pragma once

#include <glad/glad.h>
#include <vector>

#include "../Vertex.h"

class VBO
{
private:
    GLuint ID = 0;

public:
    VBO();
    ~VBO();

    VBO(const VBO&) = delete;
    VBO& operator=(const VBO&) = delete;

    void Bind() const;
    void Unbind() const;

    // Enviar vértices antigos
    void Upload(
        const std::vector<float>& vertices,
        GLenum usage = GL_STATIC_DRAW
    );

    // Enviar vértices estruturados
    void Upload(
        const std::vector<Vertex>& vertices,
        GLenum usage = GL_STATIC_DRAW
    );

    void Delete();
};