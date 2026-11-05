#pragma once

#include <string>
#include <vector>

#include "texture.h"

// Fonte TrueType "assada" num atlas: uma textura com todas as letras lado a lado.
class Font
{
public:
    struct Glyph
    {
        // Retângulo da letra em pixels, relativo ao ponto de partida (a linha de base).
        // O y cresce pra baixo, então o topo (y0) costuma ser negativo.
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        // Região da letra no atlas (0 a 1).
        float s0 = 0, t0 = 0, s1 = 0, t1 = 0;
        // Quanto avançar pra escrever a próxima letra.
        float advance = 0;
    };

    // pixelHeight = tamanho em que as letras são assadas (maior = mais nítido ao ampliar).
    bool load(const std::string &path, float pixelHeight = 48.0f);

    const Texture &getTexture() const { return atlas; }
    float getPixelHeight() const { return pixelHeight; }

    // Letra de um código Unicode (de 32 a 255). Fora disso, devolve a do '?'.
    const Glyph &getGlyph(unsigned int codepoint) const;

    // Largura de um texto em pixels, na escala dada (1.0 = o tamanho assado).
    float measure(const std::string &text, float scale = 1.0f) const;

    // Converte texto UTF-8 em códigos Unicode ("ç" vira um código só, não dois bytes).
    static std::vector<unsigned int> decodeUtf8(const std::string &text);

private:
    Texture atlas;
    std::vector<Glyph> glyphs;
    float pixelHeight = 0.0f;
};