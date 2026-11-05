#include "texture.h"
#include <utility>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool Texture::load(const std::string& path)
{
    // Imagens começam pela linha de CIMA, mas o OpenGL espera a de BAIXO. Inverte ao ler.
    stbi_set_flip_vertically_on_load(true);

    int w = 0, h = 0, channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
    if (!data)
    {
        std::cout << "Não consegui carregar a textura: " << path << "\n"
                  << stbi_failure_reason() << std::endl;
        return false;
    }

    bool ok = create(w, h, channels, data);
    stbi_image_free(data); // a cópia na placa de vídeo basta
    return ok;
}

bool Texture::create(int newWidth, int newHeight, int channels, const unsigned char* data)
{
    destroy(); // se já havia uma textura aqui, apaga antes

    if (!data || newWidth <= 0 || newHeight <= 0)
    {
        std::cout << "Texture::create: dados ou tamanho inválidos." << std::endl;
        return false;
    }

    GLenum format;
    if (channels == 1)      format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;
    else
    {
        std::cout << "Textura com " << channels << " canais não é suportada." << std::endl;
        return false;
    }

    width = newWidth;
    height = newHeight;

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    // Linhas de imagens RGB nem sempre são múltiplas de 4 bytes.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D); // versões menores, pra não "tremer" de longe

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return true;
}


void Texture::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, id);
}

void Texture::destroy()
{
    if (id)
        glDeleteTextures(1, &id);
    id = 0;
    width = height = 0;
}


// Construtor de movimentação
Texture::Texture(Texture&& other) noexcept
    : id(other.id),
      width(other.width),
      height(other.height),
      type(std::move(other.type))
{
    other.id = 0;
    other.width = 0;
    other.height = 0;
}

// Operador de movimentação
Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        destroy();

        id = other.id;
        width = other.width;
        height = other.height;
        type = std::move(other.type);

        other.id = 0;
        other.width = 0;
        other.height = 0;
    }

    return *this;
}

// Definir o tipo da textura
void Texture::setType(const std::string& novoTipo)
{
    type = novoTipo;
}

// Consultar o tipo da textura
const std::string& Texture::getType() const
{
    return type;
}

bool Texture::loadFromMemory(const unsigned char* bytes, std::size_t size)
{
    if (!bytes || size == 0 || size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;

    stbi_set_flip_vertically_on_load(true);

    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(
        bytes, static_cast<int>(size), &w, &h, &channels, 0);
    if (!pixels)
    {
        std::cout << "Não consegui decodificar a textura embutida.\n"
                  << stbi_failure_reason() << std::endl;
        return false;
    }

    bool ok = create(w, h, channels, pixels);
    stbi_image_free(pixels);
    return ok;
}