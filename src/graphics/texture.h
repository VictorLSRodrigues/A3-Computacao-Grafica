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

    // Tipo usado pela malha para distinguir, por exemplo, texturas difusas e especulares.
    std::string type;

public:
    Texture() = default;
    ~Texture() { destroy(); }

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    // Permite transferir a propriedade do identificador OpenGL sem copiá-lo.
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    bool load(const std::string &path);
    bool loadFromMemory(const unsigned char* bytes, std::size_t size);
    void bind(unsigned int unit = 0) const; // Ativa a unidade e associa a textura a ela.
    void destroy();

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // Define e consulta a categoria da textura.
    void setType(const std::string& newType);
    const std::string& getType() const;

    // Cria uma textura a partir de pixels em memória (1 canal, RGB ou RGBA).
    bool create(int newWidth, int newHeight, int channels, const unsigned char *data);
};