#include "dynamis_application.h"

#include <array>
#include <iostream>
#include <memory>
#include <vector>
#include <utility>

namespace
{
WindowConfig createWindowConfig()
{
    WindowConfig config;
    config.title = "Dynamis";
    config.width = 1280;
    config.height = 800;
    return config;
}

// Malha simples e editável para testar a integração do Editor no Dynamis.
std::shared_ptr<Mesh> createTestCube()
{
    struct CubeFace
    {
        glm::vec3 center;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    const std::array<CubeFace, 6> faces{{
        {{0.0f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, -0.5f}, {0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
        {{-0.5f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
        {{0.0f, -0.5f, 0.0f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}
    }};

    const std::array<glm::vec2, 4> uvCoordinates{{
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    }};
    const std::array<glm::vec2, 4> corners{{
        {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f}
    }};

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(24);
    indices.reserve(36);

    for (const CubeFace &face : faces)
    {
        const unsigned int firstVertex = static_cast<unsigned int>(vertices.size());

        for (std::size_t i = 0; i < corners.size(); ++i)
        {
            const glm::vec2 &corner = corners[i];
            const glm::vec3 position = face.center +
                face.tangent * corner.x + face.bitangent * corner.y;

            vertices.push_back({
                position,
                glm::vec3(1.0f),
                uvCoordinates[i],
                face.normal,
                glm::vec4(face.tangent, 1.0f)
            });
        }

        indices.insert(indices.end(), {
            firstVertex, firstVertex + 1, firstVertex + 2,
            firstVertex, firstVertex + 2, firstVertex + 3
        });
    }

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->createIndexed(vertices, indices))
        return nullptr;

    Material material;
    material.baseColorFactor = glm::vec4(0.20f, 0.68f, 0.86f, 1.0f);
    material.metallicFactor = 0.0f;
    material.roughnessFactor = 0.72f;
    mesh->setMaterial(material);

    return mesh;
}
}

DynamisApplication::DynamisApplication()
    : rotor::Application(createWindowConfig())
{
}

bool DynamisApplication::onInitialize()
{
    // Mantém a iluminação ativa, mas oculta o cubo indicador da luz.
    renderer.showLightMarker = false;

    if (!renderer.init())
        return false;

    if (!ui.create())
    {
        std::cout << "Falha ao inicializar a UI do Dynamis." << std::endl;
        return false;
    }

    if (!editor.create())
    {
        std::cout << "Falha ao carregar os recursos do Editor." << std::endl;
        return false;
    }

    auto testCube = createTestCube();
    if (!testCube || !scene.addMesh(std::move(testCube), "Dynamis Cube"))
    {
        std::cout << "Falha ao criar o objeto de teste do Dynamis." << std::endl;
        return false;
    }

    camera.setViewportSize(window.getWidth(), window.getHeight());
    return true;
}

void DynamisApplication::onUpdate(float deltaTime)
{
    editor.handleModeToggle();

    // O controle de câmera continua igual ao Sandbox; o Editor trata a seleção
    // e as transformações quando o modo de edição está ativo.
    camera.processInput(window, deltaTime);
    camera.setViewportSize(window.getWidth(), window.getHeight());

    editor.update(true);
    scene.update(deltaTime);
}

void DynamisApplication::onRender()
{
    renderer.render(scene, light, camera);

    ui.begin(window);
    if (editor.isEditing())
    {
        editor.drawGizmos();
        editor.drawToolbar();
    }
    ui.end();
}
