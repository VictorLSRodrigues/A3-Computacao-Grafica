#include "font.h"

#include <fstream>
#include <iostream>
#include <iterator>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

namespace
{
    // ASCII mais os acentos do português (Latin-1).
    constexpr unsigned int FIRST_CHAR = 32;
    constexpr unsigned int LAST_CHAR = 255;
    constexpr int ATLAS_SIZE = 1024;
}

bool Font::load(const std::string& path, float newPixelHeight)
{
    // 1. Ler o arquivo .ttf inteiro pra memória
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        std::cout << "Não consegui abrir a fonte: " << path << std::endl;
        return false;
    }
    std::vector<unsigned char> ttf((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());

    // 2. Assar as letras num atlas (o stb_truetype faz o trabalho pesado)
    const int count = static_cast<int>(LAST_CHAR - FIRST_CHAR + 1);
    std::vector<stbtt_bakedchar> baked(count);
    std::vector<unsigned char> pixels(ATLAS_SIZE * ATLAS_SIZE);

    int result = stbtt_BakeFontBitmap(ttf.data(), 0, newPixelHeight,
                                      pixels.data(), ATLAS_SIZE, ATLAS_SIZE,
                                      FIRST_CHAR, count, baked.data());
    if (result <= 0)
    {
        std::cout << "As letras não couberam no atlas (tente um tamanho menor): " << path << std::endl;
        return false;
    }

    // 3. Subir o atlas pra placa de vídeo como textura de um canal
    if (!atlas.create(ATLAS_SIZE, ATLAS_SIZE, 1, pixels.data()))
        return false;

    // 4. Guardar as medidas de cada letra, já relativas ao ponto de partida
    glyphs.assign(count, Glyph{});
    for (int i = 0; i < count; ++i)
    {
        float x = 0.0f, y = 0.0f;
        stbtt_aligned_quad q;
        stbtt_GetBakedQuad(baked.data(), ATLAS_SIZE, ATLAS_SIZE, i, &x, &y, &q, 1);

        Glyph& g = glyphs[i];
        g.x0 = q.x0; g.y0 = q.y0; g.x1 = q.x1; g.y1 = q.y1;
        g.s0 = q.s0; g.t0 = q.t0; g.s1 = q.s1; g.t1 = q.t1;
        g.advance = x; // o x avançou exatamente o "advance" da letra
    }

    pixelHeight = newPixelHeight;
    return true;
}

const Font::Glyph& Font::getGlyph(unsigned int codepoint) const
{
    static const Glyph empty;
    if (glyphs.empty())
        return empty;

    if (codepoint < FIRST_CHAR || codepoint > LAST_CHAR)
        codepoint = '?';

    return glyphs[codepoint - FIRST_CHAR];
}

float Font::measure(const std::string& text, float scale) const
{
    float width = 0.0f;
    for (unsigned int codepoint : decodeUtf8(text))
        width += getGlyph(codepoint).advance;

    return width * scale;
}

// UTF-8: letras comuns usam 1 byte, mas "ç" ou "ã" usam 2 (ou mais pra outros símbolos).
std::vector<unsigned int> Font::decodeUtf8(const std::string& text)
{
    std::vector<unsigned int> result;

    for (size_t i = 0; i < text.size();)
    {
        unsigned char c = static_cast<unsigned char>(text[i]);
        unsigned int codepoint = c;
        int extra = 0; // quantos bytes de continuação seguem

        if (c < 0x80)
            extra = 0;
        else if ((c >> 5) == 0x6)
        {
            codepoint = c & 0x1F;
            extra = 1;
        }
        else if ((c >> 4) == 0xE)
        {
            codepoint = c & 0x0F;
            extra = 2;
        }
        else if ((c >> 3) == 0x1E)
        {
            codepoint = c & 0x07;
            extra = 3;
        }
        else
            codepoint = '?'; // byte inválido

        ++i;
        for (int k = 0; k < extra && i < text.size(); ++k, ++i)
            codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[i]) & 0x3F);

        result.push_back(codepoint);
    }

    return result;
}