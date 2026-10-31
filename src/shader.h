#pragma once

#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>

class Shader
{
private:
    GLuint programa = 0;

public:
    Shader() = default;
    ~Shader() { limpar(); }

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;

    bool criar(const char *vertexFonte, const char *fragmentFonte);
    bool criarDeArquivos(const std::string &caminhoVertex, const std::string &caminhoFragment);
    void usar() const;
    void limpar();
    void setVec2(const char *nome, float x, float y) const;
    void setVec4(const char *nome, float x, float y, float z, float w) const;
    void setMat4(const char *nome, const glm::mat4 &matriz) const;

};