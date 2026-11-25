
#include "mesh.h"

#include <iostream>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "engine/graphics/resources/shader.h"

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

    // Localização 2: coordenadas UV.
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

    // Localização 4: tangente e handedness.
    glVertexAttribPointer(
        4, 4, GL_FLOAT, GL_FALSE,
        stride,
        reinterpret_cast<const void *>(offsetof(Vertex, tangent)));
    glEnableVertexAttribArray(4);
}

// Envia a geometria atual aos buffers da GPU.
void Mesh::upload()
{
    vao.Bind();

    if (geometry.getVertexFormat() == VertexFormat::Structured)
    {
        vbo.Upload(geometry.getStructuredVertices());

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

    // O EBO fica associado ao VAO ativo.
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

// Cria uma malha com índices.
bool Mesh::createIndexed(
    const std::vector<float> &vertices,
    const std::vector<unsigned int> &indices,
    int componentCount,
    GLenum drawMode,
    const std::vector<int> &layout)
{
    PrimitiveTopology topology;

    if (!toTopology(drawMode, topology) ||
        !geometry.createIndexed(
            vertices,
            indices,
            componentCount,
            topology,
            layout))
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

// Inicia a extrusão de uma face.
bool Mesh::beginExtrusion(unsigned int faceIndex)
{
    return geometry.beginExtrusion(faceIndex);
}

// Atualiza a extrusão.
bool Mesh::updateExtrusion(float distance)
{
    if (!geometry.updateExtrusion(distance))
        return false;

    upload();

    return true;
}

// Finaliza a extrusão.
void Mesh::finishExtrusion()
{
    geometry.finishExtrusion();
}

// Cancela a extrusão.
void Mesh::cancelExtrusion()
{
    if (geometry.cancelExtrusion())
        upload();
}

bool Mesh::restoreExtrudedGeometry(
    const std::vector<Vertex>& vertices,
    const std::vector<unsigned int>& indices)
{
    if (!geometry.restoreExtrudedGeometry(vertices, indices))
        return false;

    drawMode = GL_TRIANGLES;

    upload();

    return true;
}

bool Mesh::copyFrom(const Mesh& source)
{
    if (this == &source)
        return true;

    bool created = false;

    const Geometry& sourceGeometry = source.geometry;

    if (sourceGeometry.getVertexFormat() == VertexFormat::Structured)
    {
        if (sourceGeometry.isIndexed())
        {
            created = createIndexed(
                sourceGeometry.getStructuredVertices(),
                sourceGeometry.getIndexData(),
                source.drawMode);
        }
        else
        {
            created = create(
                sourceGeometry.getStructuredVertices(),
                source.drawMode);
        }
    }
    else if (sourceGeometry.getVertexFormat() == VertexFormat::Raw)
    {
        const auto& layout = sourceGeometry.getAttributeSizes();

        if (sourceGeometry.isIndexed())
        {
            created = createIndexed(
                sourceGeometry.getVertexData(),
                sourceGeometry.getIndexData(),
                3,
                source.drawMode,
                layout);
        }
        else
        {
            created = create(
                sourceGeometry.getVertexData(),
                3,
                source.drawMode,
                layout);
        }
    }

    if (!created)
        return false;

    // Preserva o estado de uma geometria que já recebeu extrusão.
    if (sourceGeometry.hasExtrusion())
    {
        if (sourceGeometry.getVertexFormat() != VertexFormat::Structured ||
            !sourceGeometry.isIndexed())
        {
            return false;
        }

        if (!restoreExtrudedGeometry(
                sourceGeometry.getStructuredVertices(),
                sourceGeometry.getIndexData()))
        {
            return false;
        }
    }

    material = source.material;
    baseColor = source.baseColor;
    textures = source.textures;

    return true;
}

// Desenha a malha.
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

// O passe de sombras não precisa de materiais ou texturas: somente os vértices.
void Mesh::drawDepth() const
{
    draw();
}

// Desenha a malha aplicando texturas e propriedades do material.

void Mesh::draw(
    Shader &shader,
    const Texture *diffuseFallback) const
{
    bool hasDiffuse = false;
    bool hasSpecular = false;
    bool hasMetallicRoughness = false;
    bool hasNormal = false;

    // Percorre as texturas associadas à malha.
    for (const auto& texture : textures)
    {
        if (!texture)
            continue;

        const std::string& type = texture->getType();

        // Diffuse / Albedo.
        if (type == "diffuse" || type == "fallback")
        {
            hasDiffuse = true;

            texture->bind(0);
            shader.setInt("tex0", 0);
        }
        // Specular.
        else if (type == "specular")
        {
            hasSpecular = true;

            texture->bind(1);
            shader.setInt("specular0", 1);
        }
        // Metallic / Roughness.
        else if (type == "metallicRoughness")
        {
            hasMetallicRoughness = true;

            texture->bind(2);
            shader.setInt("metallicRoughnessMap", 2);
        }
        // Normal Map.
        else if (type == "normal" || type == "normalMap")
        {
            hasNormal = true;

            texture->bind(3);
            shader.setInt("normalMap", 3);
        }
    }

    // Fallback para Diffuse.
    if (!hasDiffuse)
    {
        if (diffuseFallback)
        {
            diffuseFallback->bind(0);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        shader.setInt("tex0", 0);
    }

    // Remove mapas ausentes para evitar herança de outra malha.
    if (!hasSpecular)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (!hasMetallicRoughness)
    {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (!hasNormal)
    {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Configura os samplers.
    shader.setInt("specular0", 1);
    shader.setInt("metallicRoughnessMap", 2);
    shader.setInt("normalMap", 3);

    // Informa ao shader quais mapas estão disponíveis.
    shader.setInt(
        "hasMetallicRoughnessMap",
        hasMetallicRoughness ? 1 : 0);

    shader.setInt(
        "hasNormalMap",
        hasNormal ? 1 : 0);

    // Propriedades do material.
    shader.setVec4(
        "baseColor",
        baseColor.r,
        baseColor.g,
        baseColor.b,
        baseColor.a);

    shader.setFloat(
        "metallicFactor",
        material.metallicFactor);

    shader.setFloat(
        "roughnessFactor",
        material.roughnessFactor);

    shader.setFloat(
        "shininess",
        material.shininess);

    // Desenha a geometria.
    draw();

    // Restaura a unidade de textura padrão.
    glActiveTexture(GL_TEXTURE0);
}


// Libera os recursos da malha.
void Mesh::destroy()
{
    vao.Delete();
    vbo.Delete();
    ebo.Delete();

    geometry.clear();

    // Libera as referências compartilhadas.
    // A textura só será destruída quando não houver
    // mais referências compartilhadas para ela.
    textures.clear();
}

// Adiciona uma textura compartilhada.
void Mesh::addTexture(std::shared_ptr<Texture> texture)
{
    if (!texture)
        return;

    textures.push_back(std::move(texture));
}

// Substitui uma textura pelo seu tipo.
void Mesh::setTexture(std::shared_ptr<Texture> texture)
{
    if (!texture)
        return;

    for (auto& current : textures)
    {
        if (!current)
            continue;

        const bool sameType =
            current->getType() == texture->getType();

        const bool replacingNeutral =
            texture->getType() == "diffuse" &&
            current->getType() == "fallback";

        if (sameType || replacingNeutral)
        {
            current = std::move(texture);
            return;
        }
    }

    textures.push_back(std::move(texture));
}

// Compatibilidade temporária com o Model atual.
void Mesh::addTexture(Texture&& texture)
{
    addTexture(
        std::make_shared<Texture>(std::move(texture)));
}

// Compatibilidade temporária com o Model atual.
void Mesh::setTexture(Texture&& texture)
{
    setTexture(
        std::make_shared<Texture>(std::move(texture)));
}

// Retorna as texturas compartilhadas da malha.
const std::vector<std::shared_ptr<Texture>>&
Mesh::getTextures() const
{
    return textures;
}
