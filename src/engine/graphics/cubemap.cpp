#include "cubemap.h"

#include <iostream>
#include <utility>

#include <stb_image.h>

Cubemap::Cubemap(Cubemap&& other) noexcept : id(other.id)
{
    other.id = 0;
}

Cubemap& Cubemap::operator=(Cubemap&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        id = other.id;
        other.id = 0;
    }

    return *this;
}

bool Cubemap::load(const std::array<std::string, 6>& facePaths)
{
    destroy();

    // Ao contrário das texturas comuns, as faces do cubemap não usam a inversão vertical.
    stbi_set_flip_vertically_on_load(false);

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);

    // Permite ler linhas cujo tamanho em bytes não seja múltiplo de quatro.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    int size = 0;
    for (unsigned int i = 0; i < 6; i++)
    {
        int width = 0, height = 0, channels = 0;
        unsigned char* data = stbi_load(facePaths[i].c_str(), &width, &height, &channels, 3);
        if (!data)
        {
            std::cout << "Não consegui carregar a face do cubemap: " << facePaths[i] << "\n"
                      << stbi_failure_reason() << std::endl;
            destroy();
            return false;
        }

        if (width != height || (size != 0 && width != size))
        {
            std::cout << "As faces do cubemap devem ser quadradas e do mesmo tamanho: "
                      << facePaths[i] << std::endl;
            stbi_image_free(data);
            destroy();
            return false;
        }

        size = width;
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data); // Os pixels já foram enviados para a textura OpenGL.
    }

    // Sem repetição nas bordas, para não aparecerem emendas entre as faces.
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return true;
}

void Cubemap::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_CUBE_MAP, id);
}

void Cubemap::destroy()
{
    if (id)
        glDeleteTextures(1, &id);
    id = 0;
}