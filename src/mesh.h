#pragma once

#include <glad/glad.h>
#include <vector>

class Mesh
{
private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    GLsizei quantidadeVertices = 0;
    GLsizei quantidadeIndices = 0;
    GLenum modo = GL_TRIANGLES;
    bool usaIndices = false;

    // Tamanho de um vértice em floats (soma do layout), ou 0 se algo for inválido.
    static int computeStride(int componentes, const std::vector<int>& layout);

    // Configura os atributos 0, 1, 2... do VAO que estiver ligado.
    static void setupAttributes(int componentes, const std::vector<int>& layout);

public:
    Mesh() = default;
    ~Mesh() { limpar(); }

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // 'layout' é opcional. Vazio = formato antigo (um atributo só, com 'componentes' floats).
    // Exemplo: {3, 3, 2} = posição (3) + normal (3) + UV (2), nos atributos 0, 1 e 2 do shader.
    bool criar(const std::vector<float>& vertices,
               int componentes = 3,
               GLenum modoDesenho = GL_TRIANGLES,
               const std::vector<int>& layout = {});

    bool criarEbo(
        const std::vector<float>& vertices,
        const std::vector<unsigned int>& indices,
        int componentes = 3,
        GLenum modoDesenho = GL_TRIANGLES,
        const std::vector<int>& layout = {});

    void desenhar() const;
    void limpar();
};