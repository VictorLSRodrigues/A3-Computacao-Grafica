#include "menu.h"
#include "formas.h"

#include <iostream>

bool Menu::criar()
{
    if (!shaderUI.criarDeArquivos(ASSETS_DIR "shaders/ui.vert",
                                  ASSETS_DIR "shaders/ui.frag"))
    {
        std::cout << "Falha ao criar o shader da UI!" << std::endl;
        return false;
    }

    if (!retangulo.criarEbo(formas::retangulo(), formas::indicesRetangulo(), 2))
    {
        std::cout << "Falha ao criar o retangulo!" << std::endl;
        return false;
    }

    // Permite cores com transparência (o quarto número da cor).
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!font.load(ASSETS_DIR "fonts/DejaVuSans.ttf"))
        return false;

    if (!textShader.criarDeArquivos(ASSETS_DIR "shaders/text.vert",
                                    ASSETS_DIR "shaders/text.frag"))
    {
        std::cout << "Falha ao criar o shader de texto!" << std::endl;
        return false;
    }

    return true;
}

void Menu::drawText(const Window &window, const std::string &text, float x, float y, float scale,
                    float red, float green, float blue, float alpha)
{
    textShader.usar();
    textShader.setVec2("uScreen", (float)window.getLargura(), (float)window.getAltura());
    textShader.setVec4("uColor", red, green, blue, alpha);
    font.getTexture().bind(0);

    float cursorX = x;
    for (unsigned int codepoint : Font::decodeUtf8(text))
    {
        const Font::Glyph &glyph = font.getGlyph(codepoint);

        textShader.setVec2("uPos", cursorX + glyph.x0 * scale, y + glyph.y0 * scale);
        textShader.setVec2("uSize", (glyph.x1 - glyph.x0) * scale, (glyph.y1 - glyph.y0) * scale);
        textShader.setVec2("uUVPos", glyph.s0, glyph.t0);
        textShader.setVec2("uUVSize", glyph.s1 - glyph.s0, glyph.t1 - glyph.t0);
        retangulo.desenhar();

        cursorX += glyph.advance * scale;
    }
}

void Menu::atualizar(const Window &janela)
{
    if (janela.teclaApertou(GLFW_KEY_ESCAPE))
        aberto = !aberto;
}

void Menu::desenharRetangulo(const Window &janela, float x, float y, float largura, float altura,
                             float r, float g, float b, float a)
{
    shaderUI.usar();
    shaderUI.setVec2("uTela", (float)janela.getLargura(), (float)janela.getAltura());
    shaderUI.setVec2("uPos", x, y);
    shaderUI.setVec2("uTamanho", largura, altura);
    shaderUI.setVec4("uCor", r, g, b, a);
    retangulo.desenhar();
}

bool Menu::botao(const Window &janela, float x, float y, float largura, float altura,
                 const std::string &label)
{
    float mx = (float)janela.getMouseX();
    float my = (float)janela.getMouseY();
    bool sobre = mx >= x && mx <= x + largura && my >= y && my <= y + altura;

    if (sobre)
        desenharRetangulo(janela, x, y, largura, altura, 0.9f, 0.2f, 0.2f, 1.0f);
    else
        desenharRetangulo(janela, x, y, largura, altura, 0.6f, 0.1f, 0.1f, 1.0f);

    // Rótulo centralizado (a altura das maiúsculas é cerca de 62% do tamanho assado).
    const float scale = 0.6f;
    float textWidth = font.measure(label, scale);
    float capHeight = font.getPixelHeight() * scale * 0.62f;
    drawText(janela, label,
             x + (largura - textWidth) / 2.0f,
             y + (altura + capHeight) / 2.0f,
             scale, 1.0f, 1.0f, 1.0f, 1.0f);

    return sobre && janela.mouseClicou(GLFW_MOUSE_BUTTON_LEFT);
}

void Menu::desenhar(const Window &janela)
{
    if (!aberto)
        return;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float tela_l = (float)janela.getLargura();
    float tela_a = (float)janela.getAltura();

    // Fundo escurecido
    desenharRetangulo(
        janela, 0.0f, 0.0f, tela_l, tela_a,
        0.0f, 0.0f, 0.0f, 0.5f);

    // Painel central
    float painelL = 400.0f;
    float painelA = 300.0f;

    float px = (tela_l - painelL) / 2.0f;
    float py = (tela_a - painelA) / 2.0f;

    desenharRetangulo(
        janela, px, py, painelL, painelA,
        0.1f, 0.12f, 0.18f, 0.95f);

    // Título do painel   
    drawText(janela, "Configurações", px + 30.0f, py + 60.0f, 0.8f, 1.0f, 1.0f, 1.0f, 1.0f);

    // Botão sair
    if (botao(
            janela,
            px + 50.0f,
            py + painelA - 120.0f,
            300.0f,
            70.0f,
            "Sair"))
    {
        janela.fechar();
    }

    // Restaurar renderização 3D
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}