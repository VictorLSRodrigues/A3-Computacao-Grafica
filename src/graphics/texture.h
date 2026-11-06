#pragma once

#include <cstddef>
#include <glad/glad.h>
#include <string>

class Texture
{
private:
    GLuint id = 0;
    int width = 0;
    int height = 0;

    // Identifica o tipo da textura
    std::string type;

public:
    Texture() = default;
    ~Texture() { destroy(); }

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    // Permitir movimentação dos recursos OpenGL
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load(const std::string &path);
    bool loadFromMemory(const unsigned char* bytes, std::size_t size);
    void bind(unsigned int unit = 0) const; // liga a textura na unidade indicada
    void destroy();

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Identificação do tipo da textura
    void setType(const std::string& novoTipo);
    const std::string& getType() const;

    // Cria a textura a partir de dados já em memória (sem arquivo).
    // channels: 1 (um canal, como o atlas da fonte), 3 (RGB) ou 4 (RGBA).
    bool create(int newWidth, int newHeight, int channels, const unsigned char *data);
};