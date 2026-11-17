#pragma once

#include <array>
#include <glad/glad.h>
#include <string>

// Textura formada por seis imagens quadradas, uma para cada lado de um cubo.
// A ordem das faces é: +X, -X, +Y, -Y, +Z, -Z.
class Cubemap
{
private:
    GLuint id = 0;

public:
    Cubemap() = default;
    ~Cubemap() { destroy(); }

    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    Cubemap(Cubemap&& other) noexcept;
    Cubemap& operator=(Cubemap&& other) noexcept;

    // Carrega as seis imagens; todas precisam ser quadradas e do mesmo tamanho.
    bool load(const std::array<std::string, 6>& facePaths);

    void bind(unsigned int unit = 0) const; // Ativa a unidade e associa o cubemap a ela.
    void destroy();
};