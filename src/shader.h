#pragma once

#include <glad/glad.h>
#include <string>

class Shader
{
private:
    GLuint programa = 0;

public:
    bool criar(const char* vertexFonte, const char* fragmentFonte);
    bool criarDeArquivos(const std::string& caminhoVertex, const std::string& caminhoFragment);
    void usar() const;
    void limpar();
};