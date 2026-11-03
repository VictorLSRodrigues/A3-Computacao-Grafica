#pragma once

#include <glad/glad.h>
#include <string>

class Texture
{
private:
    GLuint id = 0;
    int width = 0;
    int height = 0;

public:
    Texture() = default;
    ~Texture() { destroy(); }

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    bool load(const std::string &path);
    void bind(unsigned int unit = 0) const; // liga a textura na unidade indicada
    void destroy();

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Cria a textura a partir de dados já em memória (sem arquivo).
    // channels: 1 (um canal, como o atlas da fonte), 3 (RGB) ou 4 (RGBA).
    bool create(int newWidth, int newHeight, int channels, const unsigned char *data);
};