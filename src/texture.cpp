#include "texture.h"

#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool Texture::load(const std::string& path)
{
    destroy(); // se já havia uma textura aqui, apaga antes

    // Imagens começam pela linha de CIMA, mas o OpenGL espera a de BAIXO. Inverte ao ler.
    stbi_set_flip_vertically_on_load(true);

    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data)
    {
        std::cout << "Não consegui carregar a textura: " << path << "\n"
                  << stbi_failure_reason() << std::endl;
        width = height = 0;
        return false;
    }

    GLenum format;
    if (channels == 1)      format = GL_RED;
    else if (channels == 3) format = GL_RGB;
    else if (channels == 4) format = GL_RGBA;
    else
    {
        std::cout << "Textura com " << channels << " canais não é suportada: " << path << std::endl;
        stbi_image_free(data);
        width = height = 0;
        return false;
    }

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
    stbi_image_free(data); // a cópia na placa de vídeo basta

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