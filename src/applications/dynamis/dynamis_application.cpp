#include "dynamis_application.h"

#include "engine/graphics/geometry/shapes.h"

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

// Plano procedural que serve como chão inicial do ambiente Dynamis.
std::shared_ptr<Mesh> createGroundPlane()
{
    constexpr float width = 12.0f;
    constexpr float depth = 12.0f;
    constexpr unsigned int segmentsX = 24;
    constexpr unsigned int segmentsZ = 24;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    if (!shapes::gridPlane(
            width, depth, segmentsX, segmentsZ, vertices, indices))
        return nullptr;

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->createIndexed(vertices, indices))
        return nullptr;

    // A textura branca evita o fallback azul do Renderer; o material mantém
    // o piso em branco neutro.
    const unsigned char whitePixel[] = {255, 255, 255, 255};
    auto whiteTexture = std::make_shared<Texture>();
    if (!whiteTexture->create(1, 1, 4, whitePixel))
        return nullptr;

    whiteTexture->setType("diffuse");
    mesh->setTexture(std::move(whiteTexture));

    Material material;
    material.baseColorFactor = glm::vec4(0.96f, 0.98f, 1.0f, 1.0f);
    material.metallicFactor = 0.0f;
    material.roughnessFactor = 0.96f;
    mesh->setMaterial(material);

    return mesh;
}

// Malha de teste criada com a nova primitiva esférica do Engine.
std::shared_ptr<Mesh> createTestSphere()
{
    constexpr float radius = 0.5f;
    constexpr unsigned int sectors = 32;
    constexpr unsigned int stacks = 16;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    if (!shapes::sphere(radius, sectors, stacks, vertices, indices))
        return nullptr;

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->createIndexed(vertices, indices))
        return nullptr;

    Material material;
    // O Renderer fornece default.png como textura fallback para malhas sem diffuse.
    // Branco evita tingir a textura com a cor do material.
    material.baseColorFactor = glm::vec4(1.0f);
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
    // Iluminação de laboratório: a luz direcional ilumina o piso de forma
    // uniforme, sem perder intensidade com a distância.
    light.type = LightType::Directional;
    light.position = glm::vec3(5.0f, 10.0f, 5.0f);
    light.target = glm::vec3(0.0f, 0.0f, 0.0f);
    light.intensity = 1.40f;

    // Um pouco mais de iluminação ambiente suaviza as faces menos iluminadas.
    renderer.ambientStrength = 0.42f;

    // Mantém a iluminação ativa, mas oculta o marcador da luz.
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

    auto ground = createGroundPlane();
    if (!ground || !scene.addMesh(std::move(ground), "Dynamis Ground"))
    {
        std::cout << "Falha ao criar o chão procedural do Dynamis." << std::endl;
        return false;
    }

    auto testSphere = createTestSphere();
    Transform sphereTransform;
    sphereTransform.move({0.0f, 0.5f, 0.0f});

    if (!testSphere || !scene.addMesh(
            std::move(testSphere), "Dynamis Sphere", sphereTransform))
    {
        std::cout << "Falha ao criar a esfera de teste do Dynamis." << std::endl;
        return false;
    }

    const glm::vec3 cameraTarget(0.0f, 0.5f, 0.0f);
    camera.orientation = glm::normalize(cameraTarget - camera.position);
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
