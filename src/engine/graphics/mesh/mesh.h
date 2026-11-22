#pragma once

#include <glad/glad.h>
#include <vector>

#include "engine/graphics/buffers/VAO.h"
#include "engine/graphics/buffers/VBO.h"
#include "engine/graphics/buffers/EBO.h"
#include "engine/graphics/geometry/geometry.h"
#include "engine/graphics/resources/texture.h"
#include "material.h"
#include <memory>

class Shader;

// Representação gráfica de uma Geometry: guarda os buffers OpenGL, o material e as texturas
// e sabe se desenhar. Os dados e as consultas espaciais ficam na Geometry (getGeometry()).
class Mesh
{
public:
    Mesh() = default;
    ~Mesh() { destroy(); }

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    // Em layout, cada número informa quantos valores formam um atributo do vértice.
    // Exemplo: {3, 3, 2, 3} representa posição, cor, coordenadas da textura e normal.
    // Cria uma malha a partir de números, sem reutilizar vértices por índice.
    bool create(
        const std::vector<float> &vertices,
        int componentCount = 3,
        GLenum drawMode = GL_TRIANGLES,
        const std::vector<int> &layout = {});

    // Cria uma malha a partir de números e índices de vértice.
    bool createIndexed(
        const std::vector<float> &vertices,
        const std::vector<unsigned int> &indices,
        int componentCount = 3,
        GLenum drawMode = GL_TRIANGLES,
        const std::vector<int> &layout = {});

    // Cria uma malha a partir de vértices estruturados, sem índices.
    bool create(
        const std::vector<Vertex> &vertices,
        GLenum drawMode = GL_TRIANGLES);

    // Cria uma malha a partir de vértices estruturados e índices.
    bool createIndexed(
        const std::vector<Vertex> &vertices,
        const std::vector<unsigned int> &indices,
        GLenum drawMode = GL_TRIANGLES);

    // Dados e consultas espaciais da malha (faces, limites, raio de picking).
    const Geometry &getGeometry() const { return geometry; }

    // Restaura uma geometria extrudada e atualiza os buffers da GPU.
    bool restoreExtrudedGeometry(
        const std::vector<Vertex> &vertices,
        const std::vector<unsigned int> &indices);

    // Copia os dados de outra malha.
    bool copyFrom(const Mesh& source);
     
    // Extrusão: a Geometry altera os dados e a Mesh os reenvia à GPU.

    // Inicia uma operação de extrusão.
    bool beginExtrusion(unsigned int faceIndex);

    // Atualiza a distância da extrusão.
    bool updateExtrusion(float distance);

    // Finaliza e confirma a extrusão.
    void finishExtrusion();

    // Cancela e restaura a geometria original.
    void cancelExtrusion();

    // Gerenciamento de texturas compartilhadas.
    void addTexture(std::shared_ptr<Texture> texture);
    void setTexture(std::shared_ptr<Texture> texture);

    // Compatibilidade temporária com o Model atual.
    void addTexture(Texture &&texture);
    void setTexture(Texture &&texture); // Substitui uma textura do mesmo tipo.

    const std::vector<std::shared_ptr<Texture>> &getTextures() const;

    void setMaterial(const Material &material)
    {
        this->material = material;
        baseColor = material.baseColorFactor;
    }
    void setBaseColor(const glm::vec4 &color)
    {
        baseColor = color;
        material.baseColorFactor = color;
    }

    const Material &getMaterial() const { return material; }

    void draw() const;

    // Associa as texturas e desenha a malha; usa diffuseFallback se não houver textura difusa.
    void draw(Shader &shader, const Texture *diffuseFallback = nullptr) const;

    void destroy();

private:
    Geometry geometry;

    VAO vao;
    VBO vbo;
    EBO ebo;

    GLenum drawMode = GL_TRIANGLES;

    std::vector<std::shared_ptr<Texture>> textures;

    // Cor que multiplica a textura ao desenhar (vem do material do modelo).
    glm::vec4 baseColor{1.0f};

    Material material{};

    // Envia a geometria atual aos buffers e configura os atributos no VAO.
    void upload();

    // Informa ao VAO como separar os atributos de vértices em formato de números.
    static void setupAttributes(const std::vector<int> &attributeSizes);

    // Configura como os campos de Vertex são enviados ao OpenGL.
    static void setupVertexAttributes();
};
