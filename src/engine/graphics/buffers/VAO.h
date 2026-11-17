#pragma once

#include <glad/glad.h>

// Guarda a configuração que explica ao OpenGL como interpretar os dados dos vértices.
// O objeto OpenGL só é criado no primeiro uso, então a classe pode ser construída antes da janela.
class VAO
{
private:
    mutable GLuint ID = 0;

    void ensureCreated() const;

public:
    VAO() = default;
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