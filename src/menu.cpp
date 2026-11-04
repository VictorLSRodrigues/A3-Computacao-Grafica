#include "menu.h"
#include "formas.h"

#include <iostream>
#include <algorithm>

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

// Escreve o texto centralizado dentro de um retângulo (x, y, width, height).
void Menu::drawTextCentered(const Window& window, const std::string& text,
                            float x, float y, float width, float height, float scale,
                            float red, float green, float blue, float alpha)
{
    float textWidth = font.measure(text, scale);
    float capHeight = font.getPixelHeight() * scale * 0.62f; // altura das maiúsculas

    drawText(window, text,
             x + (width - textWidth) / 2.0f,
             y + (height + capHeight) / 2.0f,
             scale, red, green, blue, alpha);
}

// Desenha um botão e devolve true se foi clicado neste quadro.
// (r, g, b) é a cor do botão; com o mouse em cima ele clareia.
bool Menu::botao(const Window& janela, float x, float y, float largura, float altura,
                 const std::string& label, float r, float g, float b, bool enabled)
{
    float mx = (float)janela.getMouseX();
    float my = (float)janela.getMouseY();
    bool hovered = enabled && mx >= x && mx <= x + largura && my >= y && my <= y + altura;

    if (!enabled)
    {
        r = 0.22f; g = 0.24f; b = 0.28f; // cinza apagado
    }
    else if (hovered)
    {
        r = std::min(r * 1.35f, 1.0f);
        g = std::min(g * 1.35f, 1.0f);
        b = std::min(b * 1.35f, 1.0f);
    }

    desenharRetangulo(janela, x, y, largura, altura, r, g, b, 1.0f);
    drawTextCentered(janela, label, x, y, largura, altura, 0.6f,
                     1.0f, 1.0f, 1.0f, enabled ? 1.0f : 0.4f);

    return hovered && janela.mouseClicou(GLFW_MOUSE_BUTTON_LEFT);
}

void Menu::desenhar(const Window& janela)
{
    if (!aberto)
        return;

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float screenW = (float)janela.getLargura();
    const float screenH = (float)janela.getAltura();

    // Medidas do painel (tudo em pixels). Mexa nestes números pra ajustar o visual.
    const float panelW = 420.0f;
    const float margin = 30.0f;  // espaço entre a borda do painel e o conteúdo
    const float titleH = 70.0f;  // altura reservada pro título
    const float buttonW = panelW - 2.0f * margin;
    const float buttonH = 60.0f;
    const float gap = 16.0f;     // espaço entre os botões
    const int count = 3;         // quantidade de botões (atualize se acrescentar mais)

    // A altura do painel é calculada, então ele sempre fecha certinho em volta dos botões.
    const float panelH = margin + titleH + count * buttonH + (count - 1) * gap + margin;
    const float px = (screenW - panelW) / 2.0f;
    const float py = (screenH - panelH) / 2.0f;

    // Fundo escurecido
    desenharRetangulo(janela, 0.0f, 0.0f, screenW, screenH, 0.0f, 0.0f, 0.0f, 0.55f);

    // Painel, com uma faixa de destaque no topo
    desenharRetangulo(janela, px, py, panelW, panelH, 0.08f, 0.10f, 0.15f, 0.96f);
    desenharRetangulo(janela, px, py, panelW, 4.0f, 0.25f, 0.55f, 0.95f, 1.0f);

    // Título centralizado
    drawTextCentered(janela, "Menu", px, py + margin, panelW, titleH, 0.85f,
                     1.0f, 1.0f, 1.0f, 1.0f);

    // Botões empilhados e centralizados
    const float bx = px + margin;
    float by = py + margin + titleH;

    if (botao(janela, bx, by, buttonW, buttonH, "Continuar", 0.20f, 0.45f, 0.85f))
        aberto = false;
    by += buttonH + gap;

    botao(janela, bx, by, buttonW, buttonH, "Opções", 0.25f, 0.30f, 0.40f, false);
    by += buttonH + gap;

    if (botao(janela, bx, by, buttonW, buttonH, "Sair", 0.70f, 0.15f, 0.15f))
        janela.fechar();

    // Restaurar renderização 3D
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}