
#pragma once

#include <glad/glad.h>

class VAO
{
private:
    GLuint ID = 0;

public:
    VAO();
    ~VAO();

    VAO(const VAO&) = delete;
    VAO& operator=(const VAO&) = delete;

    void Bind() const;
    void Unbind() const;
    void Delete();
};
