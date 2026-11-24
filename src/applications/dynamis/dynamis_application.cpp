#include "dynamis_application.h"

#include "engine/graphics/geometry/shapes.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

namespace
{
constexpr float kBowlRadius = 18.0f;
constexpr float kBowlDepth = 4.5f;
constexpr float kBowlLipHeight = 2.5f;

WindowConfig createWindowConfig()
{
    WindowConfig config;
    config.title = "Dynamis";
    config.width = 1280;
    config.height = 800;
    return config;
}

std::shared_ptr<Texture> createGroundGridTexture()
{
    // 48 divisões menores ao longo dos 36 m de diâmetro mantêm uma grade
    // de aproximadamente 0,75 m, mesmo com a área maior da bacia.
    constexpr int width = 768;
    constexpr int height = 768;
    std::vector<unsigned char> pixels(width * height * 4, 255);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const bool majorLine = (x % 64 == 0) || (y % 64 == 0);
            const bool minorLine = (x % 16 == 0) || (y % 16 == 0);
            const unsigned char gray = majorLine ? 214 : (minorLine ? 237 : 255);
            const std::size_t index =
                (static_cast<std::size_t>(y) * width + x) * 4;
            pixels[index + 0] = gray;
            pixels[index + 1] = gray;
            pixels[index + 2] = gray;
            pixels[index + 3] = 255;
        }
    }

    auto texture = std::make_shared<Texture>();
    if (!texture->create(
            width, height, 4, pixels.data(), TextureColorSpace::SRGB))
        return nullptr;

    texture->setType("diffuse");
    return texture;
}

// Bacia branca e ampla. A curvatura visual corresponde ao colisor analítico
// configurado no PhysicsWorld, para que as esferas rolem sobre a superfície real.
std::shared_ptr<Mesh> createGroundBowl()
{
    constexpr unsigned int radialSegments = 64;
    constexpr unsigned int angularSegments = 128;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    if (!shapes::bowl(
            kBowlRadius,
            kBowlDepth,
            kBowlLipHeight,
            radialSegments,
            angularSegments,
            vertices,
            indices))
        return nullptr;

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->createIndexed(vertices, indices))
        return nullptr;

    auto gridTexture = createGroundGridTexture();
    if (!gridTexture)
        return nullptr;
    mesh->setTexture(std::move(gridTexture));

    Material material;
    material.baseColorFactor = glm::vec4(1.0f);
    material.metallicFactor = 0.0f;
    material.roughnessFactor = 0.96f;
    mesh->setMaterial(material);
    return mesh;
}

std::shared_ptr<Texture> createRollingPattern(bool orangePattern)
{
    constexpr int width = 192;
    constexpr int height = 96;
    std::vector<unsigned char> pixels(width * height * 4);

    const unsigned char baseA[3] = {
        static_cast<unsigned char>(orangePattern ? 238 : 24),
        static_cast<unsigned char>(orangePattern ? 104 : 154),
        static_cast<unsigned char>(orangePattern ? 43 : 174)};
    const unsigned char baseB[3] = {
        static_cast<unsigned char>(orangePattern ? 29 : 236),
        static_cast<unsigned char>(orangePattern ? 42 : 220),
        static_cast<unsigned char>(orangePattern ? 58 : 105)};
    const unsigned char accent[3] = {255, 225, 104};

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Faixas longitudinais largas e uma faixa equatorial tornam a rotação
            // visível mesmo quando a esfera está relativamente distante da câmera.
            const int longitude = (x + width - 5) % 48;
            const bool meridian = longitude < 10;
            const bool equator = std::abs(y - height / 2) <= 3;

            const int spotX = orangePattern ? 48 : 144;
            const int spotY = 19;
            const int dx = x - spotX;
            const int dy = y - spotY;
            const bool poleMarker = dx * dx + dy * dy <= 8 * 8;

            const unsigned char* color = meridian ? baseB : baseA;
            if (equator || poleMarker)
                color = accent;

            const std::size_t index =
                (static_cast<std::size_t>(y) * width + x) * 4;
            pixels[index + 0] = color[0];
            pixels[index + 1] = color[1];
            pixels[index + 2] = color[2];
            pixels[index + 3] = 255;
        }
    }

    auto texture = std::make_shared<Texture>();
    if (!texture->create(
            width, height, 4, pixels.data(), TextureColorSpace::SRGB))
        return nullptr;

    texture->setType("diffuse");
    return texture;
}

std::shared_ptr<Mesh> createRollingSphere(bool orangePattern)
{
    constexpr float radius = 0.5f;
    constexpr unsigned int sectors = 48;
    constexpr unsigned int stacks = 24;

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    if (!shapes::sphere(radius, sectors, stacks, vertices, indices))
        return nullptr;

    auto mesh = std::make_shared<Mesh>();
    if (!mesh->createIndexed(vertices, indices))
        return nullptr;

    auto texture = createRollingPattern(orangePattern);
    if (!texture)
        return nullptr;
    mesh->setTexture(std::move(texture));

    Material material;
    material.baseColorFactor = glm::vec4(1.0f);
    material.metallicFactor = 0.0f;
    material.roughnessFactor = 0.58f;
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
    light.type = LightType::Directional;
    light.position = glm::vec3(5.0f, 10.0f, 5.0f);
    light.target = glm::vec3(0.0f, 0.0f, 0.0f);
    light.intensity = 0.85f;
    renderer.ambientStrength = 0.20f;
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

    auto ground = createGroundBowl();
    if (!ground || !scene.addMesh(std::move(ground), "Dynamis Bowl"))
    {
        std::cout << "Falha ao criar a bacia procedural do Dynamis." << std::endl;
        return false;
    }

    // Duas esferas partem em sentidos opostos. As texturas marcadas deixam
    // evidente a rotação criada pelo atrito e a troca de movimento no impacto.
    auto orangeSphere = createRollingSphere(true);
    Transform orangeTransform;
    orangeTransform.move({-2.7f, 0.60f, 0.0f});

    Scene::Object* orangeObject = nullptr;
    if (!orangeSphere || !(orangeObject = scene.addMesh(
            std::move(orangeSphere), "Rolling Sphere (1 kg)", orangeTransform)))
    {
        std::cout << "Falha ao criar a esfera laranja do Dynamis." << std::endl;
        return false;
    }

    auto cyanSphere = createRollingSphere(false);
    Transform cyanTransform;
    cyanTransform.move({2.7f, 0.60f, 0.0f});

    Scene::Object* cyanObject = nullptr;
    if (!cyanSphere || !(cyanObject = scene.addMesh(
            std::move(cyanSphere), "Rolling Sphere (2 kg)", cyanTransform)))
    {
        std::cout << "Falha ao criar a esfera azul do Dynamis." << std::endl;
        return false;
    }

    // Atrito alto o bastante para transformar o deslizamento inicial em rolamento.
    physics.setGroundBowl(
        kBowlRadius, kBowlDepth, kBowlLipHeight, 0.95f, 0.78f);
    if (!physics.addBody(
            orangeObject->transform,
            glm::vec3(3.2f, 0.0f, 0.0f),
            0.5f,
            0.35f,
            1.0f,
            0.95f,
            0.78f) ||
        !physics.addBody(
            cyanObject->transform,
            glm::vec3(-2.4f, 0.0f, 0.0f),
            0.5f,
            0.35f,
            2.0f,
            0.95f,
            0.78f))
    {
        std::cout << "Falha ao registrar os corpos dinâmicos do Dynamis." << std::endl;
        return false;
    }


    // Visão inicial panorâmica para enxergar a extensão da bacia.
    const glm::vec3 cameraTarget(0.0f, 2.0f, 0.0f);
    camera.orientation = glm::normalize(cameraTarget - camera.position);
    camera.setViewportSize(window.getWidth(), window.getHeight());
    return true;
}

void DynamisApplication::onUpdate(float deltaTime)
{
    const bool wasEditing = editor.isEditing();
    editor.handleModeToggle();
    const bool isEditing = editor.isEditing();

    camera.setViewportSize(window.getWidth(), window.getHeight());

    Scene::Object* selectedObject = editor.getSelectedObject();
    const bool selectedDynamicBody =
        !isEditing && selectedObject && selectedObject->visible &&
        physics.hasDynamicBody(selectedObject->transform);

    if (selectedDynamicBody)
    {
        // No modo de simulação, a câmera orbita e acompanha o corpo escolhido.
        // Trocar a seleção reinicia apenas a referência do acompanhamento.
        if (followedObject != selectedObject || !camera.isFollowing())
        {
            camera.endFollow();
            camera.beginFollow(selectedObject->transform.position());
            followedObject = selectedObject;
        }

        camera.processFollowInput(
            window,
            selectedObject->transform.position(),
            physics.linearVelocity(selectedObject->transform),
            deltaTime,
            GLFW_MOUSE_BUTTON_RIGHT);
    }
    else
    {
        // Editor ou ausência de corpo dinâmico: volta à câmera livre.
        if (camera.isFollowing())
            camera.endFollow();
        followedObject = nullptr;
        camera.processInput(
            window,
            deltaTime,
            GLFW_MOUSE_BUTTON_RIGHT,
            true);
    }

    editor.update(true);
    scene.update(deltaTime);

    // R reinicia o experimento completo.
    if (!isEditing && window.keyPressed(GLFW_KEY_R))
        physics.reset();

    // Ao sair da edição, as posições transformadas viram o novo estado inicial.
    if (wasEditing && !isEditing)
    {
        physics.recordStartPositions();
        physics.reset();
    }

    // A física fica pausada no Editor. Também removemos qualquer força de
    // controle que estivesse ativa antes de abrir o Editor.
    if (isEditing)
    {
        if (forceControlledTransform)
            physics.setContinuousForce(*forceControlledTransform, glm::vec3(0.0f));
        forceControlledTransform = nullptr;
        return;
    }

    selectedObject = editor.getSelectedObject();
    Transform* activeControl = nullptr;
    if (selectedObject && selectedObject->visible &&
        physics.hasDynamicBody(selectedObject->transform))
    {
        activeControl = &selectedObject->transform;
    }

    if (forceControlledTransform && forceControlledTransform != activeControl)
        physics.setContinuousForce(*forceControlledTransform, glm::vec3(0.0f));

    if (activeControl)
    {
        // WASD acompanha a orientação horizontal da câmera de terceira pessoa.
        glm::vec3 forward(camera.orientation.x, 0.0f, camera.orientation.z);
        if (glm::length(forward) > 1e-5f)
            forward = glm::normalize(forward);
        else
            forward = glm::vec3(0.0f, 0.0f, -1.0f);

        const glm::vec3 right = glm::normalize(glm::cross(forward, camera.up));
        glm::vec3 inputDirection(0.0f);

        if (window.keyDown(GLFW_KEY_W)) inputDirection += forward;
        if (window.keyDown(GLFW_KEY_S)) inputDirection -= forward;
        if (window.keyDown(GLFW_KEY_D)) inputDirection += right;
        if (window.keyDown(GLFW_KEY_A)) inputDirection -= right;

        const float inputLength = glm::length(inputDirection);
        constexpr float kControlForce = 12.0f; // Newtons
        if (inputLength > 1e-5f)
            inputDirection /= inputLength;
        else
            inputDirection = glm::vec3(0.0f);

        // Define a força ativa, em vez de acumulá-la por frame de renderização.
        physics.setContinuousForce(
            *activeControl, inputDirection * kControlForce);
    }

    forceControlledTransform = activeControl;
    physics.update(deltaTime);
}

void DynamisApplication::onRender()
{
    renderer.render(scene, light, camera);

    // O Dynamis não possui painel próprio de comandos. A única interface é
    // a toolbar do Editor, exibida apenas durante a edição da cena.
    if (editor.isEditing())
    {
        ui.begin(window);
        editor.drawGizmos();
        editor.drawToolbar();
        ui.end();
    }
}
