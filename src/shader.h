#pragma once

#include <glad/glad.h>

class Shader
{
private:
    GLuint programa = 0;

public:
    bool criar(const char* vertexFonte, const char* fragmentFonte);
    void usar() const;
    void limpar();
};