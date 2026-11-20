
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include "texture.h"

class TextureManager
{
public:
    // Retorna uma textura existente ou carrega uma nova.
    static std::shared_ptr<Texture> load(
        const std::string& path,
        const std::string& type = "diffuse"
    );

    // Carrega uma textura diretamente da memória, utilizando o cache.
    static std::shared_ptr<Texture> loadFromMemory(
        const std::string& cacheKey,
        const unsigned char* data,
        std::size_t size,
        const std::string& type
    );

    // Procura uma textura pelo caminho e tipo.
    static std::shared_ptr<Texture> get(
        const std::string& path,
        const std::string& type = "diffuse"
    );

    // Remove todas as variações de uma imagem do cache.
    static void remove(const std::string& path);

    // Limpa o cache.
    static void clear();

private:
    static std::unordered_map<
        std::string,
        std::weak_ptr<Texture>
    > textures;
};
