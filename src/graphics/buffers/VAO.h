
#pragma once

#include <glad/glad.h>

// Guarda a configuração que explica ao OpenGL como interpretar os dados dos vértices.
class VAO
{
private:
    GLuint ID = 0;

public:
    VAO();
    ~VAO();

    VAO(const VAO&) = delete;
    VAO& operator=(const VAO&) = delete;

    // Torna esta configuração de vértices a configuração ativa.
    void Bind() const;

    // Deixa de usar a configuração ativa.
    void Unbind() const;

    // Libera o objeto OpenGL.
    void Delete();
};
