#include "mesh.h"

#include <iostream>
#include <cstddef>
#include <utility>
#include "shader.h"

namespace
{
    // Converte o modo de desenho do OpenGL na topologia da geometria.
    bool toTopology(GLenum mode, PrimitiveTopology &topology)
    {
        switch (mode)
        {
        case GL_POINTS:
            topology = PrimitiveTopology::Points;
            return true;
        case GL_LINES:
            topology = PrimitiveTopology::Lines;
            return true;
        case GL_LINE_LOOP:
            topology = PrimitiveTopology::LineLoop;
            return true;
        case GL_LINE_STRIP:
            topology = PrimitiveTopology::LineStrip;
            return true;
        case GL_TRIANGLES:
            topology = PrimitiveTopology::Triangles;
            return true;
        case GL_TRIANGLE_STRIP:
            topology = PrimitiveTopology::TriangleStrip;
            return true;
        case GL_TRIANGLE_FAN:
            topology = PrimitiveTopology::TriangleFan;
            return true;
        default:
            std::cout << "Mesh: modo de desenho não suportado." << std::endl;
            return false;
        }
    }
}

// Informa ao VAO como separar os atributos dentro de cada vértice.
void Mesh::setupAttributes(const std::vector<int> &attributes)
{
    int stride = 0;

    for (int c : attributes)
        stride += c;

    int offset = 0;

    for (size_t i = 0; i < attributes.size(); ++i)
    {
        glVertexAttribPointer(
            static_cast<GLuint>(i),
            attributes[i],
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>(stride * sizeof(float)),
            reinterpret_cast<void *>(offset * sizeof(float)));

        glEnableVertexAttribArray(
            static_cast<GLuint>(i));

        offset += attributes[i];
    }
}

// Configura o layout fixo da estrutura Vertex.
void Mesh::setupVertexAttributes()
{
    const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));

    // Localização 0: posição.
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, position)));
    glEnableVertexAttribArray(0);

    // Localização 1: cor.
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, color)));
    glEnableVertexAttribArray(1);

    // Localização 2: coordenadas da textura.
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, texUV)));
    glEnableVertexAttribArray(2);

    // Localização 3: normal.
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, normal)));
    glEnableVertexAttribArray(3);
}

// Envia a geometria atual aos buffers da GPU.
void Mesh::upload()
{
    vao.Bind();

    if (geometry.getVertexFormat() == VertexFormat::Structured)
    {
        vbo.Upload(geometry.getStructuredVertices());

        // Upload desvincula GL_ARRAY_BUFFER; associe-o novamente antes de configurar os atributos.
        vbo.Bind();

        setupVertexAttributes();
    }
    else
    {
        vbo.Upload(geometry.getVertexData());

        vbo.Bind();

        setupAttributes(geometry.getAttributeSizes());
    }

    vbo.Unbind();

    // O EBO fica associado ao VAO que está ativo.
    if (geometry.isIndexed())
        ebo.Upload(geometry.getIndexData());

    vao.Unbind();
}

// Cria uma malha sem índices.
bool Mesh::create(
    const std::vector<float> &vertices,
    int componentCount,
    GLenum drawMode,
    const std::vector<int> &layout)
{
    PrimitiveTopology topology;

    if (!toTopology(drawMode, topology) ||
        !geometry.create(vertices, componentCount, topology, layout))
        return false;

    this->drawMode = drawMode;
    upload();

    return true;
}

// Cria uma malha cujos triângulos referenciam vértices por índice.
bool Mesh::createIndexed(
    const std::vector<float> &vertices,
    const std::vector<unsigned int> &indices,
    int componentCount,
    GLenum drawMode,
    const std::vector<int> &layout)
{
    PrimitiveTopology topology;

    if (!toTopology(drawMode, topology) ||
        !geometry.createIndexed(vertices, indices, componentCount, topology, layout))
        return false;

    this->drawMode = drawMode;
    upload();

    return true;
}

// Cria uma malha de vértices estruturados sem índices.
bool Mesh::create(
    const std::vector<Vertex> &vertices,
    GLenum drawMode)
{
    PrimitiveTopology topology;

    if (!toTopology(drawMode, topology) ||
        !geometry.create(vertices, topology))
        return false;

    this->drawMode = drawMode;
    upload();

    return true;
}

// Cria uma malha de vértices estruturados com índices.
bool Mesh::createIndexed(
    const std::vector<Vertex> &vertices,
    const std::vector<unsigned int> &indices,
    GLenum drawMode)
{
    PrimitiveTopology topology;

    if (!toTopology(drawMode, topology) ||
        !geometry.createIndexed(vertices, indices, topology))
        return false;

    this->drawMode = drawMode;
    upload();

    return true;
}

bool Mesh::beginExtrusion(unsigned int faceIndex)
{
    return geometry.beginExtrusion(faceIndex);
}

bool Mesh::updateExtrusion(float distance)
{
    if (!geometry.updateExtrusion(distance))
        return false;

    // A geometria mudou: reenvia os vértices e os índices.
    upload();

    return true;
}

void Mesh::finishExtrusion()
{
    geometry.finishExtrusion();
}

void Mesh::cancelExtrusion()
{
    if (geometry.cancelExtrusion())
        upload();
}

void Mesh::draw() const
{
    vao.Bind();

    if (geometry.isIndexed())
    {
        glDrawElements(
            drawMode,
            ebo.GetCount(),
            GL_UNSIGNED_INT,
            nullptr);
    }
    else
    {
        glDrawArrays(
            drawMode,
            0,
            static_cast<GLsizei>(geometry.getVertexCount()));
    }

    vao.Unbind();
}
void Mesh::draw(Shader &shader, const Texture *diffuseFallback) const
{
    bool hasDiffuse = false;
    bool hasSpecular = false;
    bool hasMetallicRoughness = false;

    // Associa cada textura à unidade usada pelo shader.
    for (const Texture &texture : textures)
    {
        if (texture.getType() == "diffuse" || texture.getType() == "fallback")
        {
            hasDiffuse = true;
            texture.bind(0);
            shader.setInt("tex0", 0);
        }
        else if (texture.getType() == "specular")
        {
            hasSpecular = true;
            texture.bind(1);
            shader.setInt("specular0", 1);
        }
        else if (texture.getType() == "metallicRoughness")
        {
            hasMetallicRoughness = true;
            texture.bind(2);
            shader.setInt("metallicRoughnessMap", 2);
        }
    }

    if (!hasDiffuse && diffuseFallback)
    {
        diffuseFallback->bind(0);
        shader.setInt("tex0", 0);
    }

    if (!hasSpecular)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        shader.setInt("specular0", 1);
    }

    if (!hasMetallicRoughness)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
        shader.setInt("metallicRoughnessMap", 2);
    }

    shader.setVec4("baseColor", baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    shader.setFloat("metallicFactor", material.metallicFactor);
    shader.setFloat("roughnessFactor", material.roughnessFactor);
    shader.setFloat("shininess", material.shininess);

    shader.setInt(
        "hasMetallicRoughnessMap",
        hasMetallicRoughness ? 1 : 0);

    draw();

    // Restaurar a unidade de textura padrão
    glActiveTexture(GL_TEXTURE0);
}

void Mesh::destroy()
{
    vao.Delete();
    vbo.Delete();
    ebo.Delete();

    geometry.clear();
    textures.clear();
}

void Mesh::addTexture(Texture &&texture)
{
    textures.push_back(std::move(texture));
}

void Mesh::setTexture(Texture &&texture)
{
    for (Texture &current : textures)
    {
        const bool sameType = current.getType() == texture.getType();
        const bool replacingNeutral =
            texture.getType() == "diffuse" && current.getType() == "fallback";

        if (sameType || replacingNeutral)
        {
            current = std::move(texture);
            return;
        }
    }

    textures.push_back(std::move(texture));
}

const std::vector<Texture> &Mesh::getTextures() const
{
    return textures;
}
