
#pragma once

#include <glad/glad.h>
#include <vector>

class EBO
{
private:
    GLuint ID = 0;
    GLsizei quantidadeIndices = 0;

public:
    EBO();
    ~EBO();

    EBO(const EBO&) = delete;
    EBO& operator=(const EBO&) = delete;

    void Bind() const;

    void Upload(
        const std::vector<unsigned int>& indices,
        GLenum usage = GL_STATIC_DRAW
    );

    void Delete();

    GLsizei GetCount() const
    {
        return quantidadeIndices;
    }
};
