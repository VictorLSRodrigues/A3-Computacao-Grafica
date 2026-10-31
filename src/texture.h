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

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    bool load(const std::string& path);
    void bind(unsigned int unit = 0) const; // liga a textura na unidade indicada
    void destroy();

    int getWidth() const { return width; }
    int getHeight() const { return height; }
};