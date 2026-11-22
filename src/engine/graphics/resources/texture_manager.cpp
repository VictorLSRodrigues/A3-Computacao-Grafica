
#include "texture_manager.h"

#include <iostream>
#include <utility>

namespace
{
    // Cria uma chave única para cada combinação de caminho e tipo.
    std::string makeCacheKey(
        const std::string& path,
        const std::string& type)
    {
        return std::to_string(path.size()) + ":" + path + type;
    }

    // Prefixo que identifica todas as variações de um caminho.
    std::string makePathPrefix(const std::string& path)
    {
        return std::to_string(path.size()) + ":" + path;
    }

    // Mapas de cor-base são imagens sRGB; mapas que armazenam propriedades
    // (normal, roughness, metallic, specular etc.) são dados lineares.
    TextureColorSpace colorSpaceForType(const std::string& type)
    {
        return (type == "diffuse" || type == "fallback")
            ? TextureColorSpace::SRGB
            : TextureColorSpace::Linear;
    }
}

std::unordered_map<
    std::string,
    std::weak_ptr<Texture>
> TextureManager::textures;


std::shared_ptr<Texture> TextureManager::load(
    const std::string& path,
    const std::string& type)
{
    if (path.empty())
        return nullptr;

    const std::string key = makeCacheKey(path, type);

    // Verifica se já existe uma textura com esse caminho e tipo.
    auto it = textures.find(key);

    if (it != textures.end())
    {
        if (auto existing = it->second.lock())
            return existing;

        // Remove referências expiradas.
        textures.erase(it);
    }

    // Cria uma nova textura.
    auto texture = std::make_shared<Texture>();

    if (!texture->load(path, true, colorSpaceForType(type)))
    {
        std::cerr
            << "TextureManager: falha ao carregar "
            << path << std::endl;

        return nullptr;
    }

    texture->setType(type);

    textures[key] = texture;

    return texture;
}


std::shared_ptr<Texture> TextureManager::loadFromMemory(
    const std::string& cacheKey,
    const unsigned char* data,
    std::size_t size,
    const std::string& type)
{
    if (cacheKey.empty() || data == nullptr || size == 0)
        return nullptr;

    const std::string key = makeCacheKey(cacheKey, type);

    auto it = textures.find(key);

    if (it != textures.end())
    {
        if (auto existing = it->second.lock())
            return existing;

        textures.erase(it);
    }

    auto texture = std::make_shared<Texture>();

    if (!texture->loadFromMemory(data, size, colorSpaceForType(type)))
    {
        std::cerr << "TextureManager: falha ao carregar imagem da memória: "
                  << cacheKey << std::endl;
        return nullptr;
    }

    texture->setType(type);
    textures[key] = texture;

    return texture;
}