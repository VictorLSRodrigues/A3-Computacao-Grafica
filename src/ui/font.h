#pragma once

#include <string>
#include <vector>

#include "graphics/texture.h"

// Converte uma fonte TrueType em uma imagem com os caracteres prontos para desenhar.
class Font
{
public:
    struct Glyph
    {
        // Retângulo da letra em pixels, relativo ao ponto de partida (a linha de base).
        // O y cresce pra baixo, então o topo (y0) costuma ser negativo.
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        // Região do caractere na imagem da fonte, em coordenadas de 0 a 1.
        float s0 = 0, t0 = 0, s1 = 0, t1 = 0;
        // Quanto avançar pra escrever a próxima letra.
        float advance = 0;
    };

    // Carrega a fonte e define a resolução usada para preparar os caracteres.
    bool load(const std::string &path, float pixelHeight = 48.0f);

    const Texture &getTexture() const { return atlas; }
    float getPixelHeight() const { return pixelHeight; }

    // Retorna o caractere correspondente; códigos fora do intervalo usam '?'.
    const Glyph &getGlyph(unsigned int codepoint) const;

    // Mede o texto em pixels para a escala informada.
    float measure(const std::string &text, float scale = 1.0f) const;

    // Decodifica UTF-8 para que caracteres acentuados sejam lidos como um único caractere.
    static std::vector<unsigned int> decodeUtf8(const std::string &text);

private:
    Texture atlas;
    std::vector<Glyph> glyphs;
    float pixelHeight = 0.0f;
};