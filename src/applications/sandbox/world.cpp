
#include "world.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "engine/graphics/mesh/mesh.h"
#include "engine/graphics/geometry/shapes.h"
#include "engine/graphics/resources/texture.h"

bool buildWorld(Scene &scene)
{
    // Cria a pirâmide original da cena.
    auto pyramidMesh = std::make_shared<Mesh>();

    if (!pyramidMesh->createIndexed(
            shapes::pyramid(),
            shapes::pyramidIndices(),
            3,
            GL_TRIANGLES,
            {3, 3, 2, 3}))
    {
        std::cout << "Falha ao criar a piramide!" << std::endl;
        return false;
    }

    Texture diffuse;
    Texture specular;

    if (!diffuse.load(ASSETS_DIR "sandbox/textures/terra.png") ||
        !specular.load(
            ASSETS_DIR "sandbox/textures/terraSpecular.png",
            true,
            TextureColorSpace::Linear))
    {
        std::cout << "Falha ao carregar as texturas da piramide!"
                  << std::endl;
        return false;
    }

    diffuse.setType("diffuse");
    specular.setType("specular");

    pyramidMesh->addTexture(std::move(diffuse));
    pyramidMesh->addTexture(std::move(specular));

    auto gridMesh = std::make_shared<Mesh>();

    std::vector<Vertex> gridVertices;
    std::vector<unsigned int> gridIndices;

    if (!shapes::gridPlane(
            20.0f,
            20.0f,
            40,
            40,
            gridVertices,
            gridIndices))
    {
        std::cerr << "Falha ao gerar o Grid Plane!" << std::endl;
        return false;
    }

    if (!gridMesh->createIndexed(
            gridVertices,
            gridIndices,
            GL_TRIANGLES))
    {
        std::cerr << "Falha ao criar a malha do Grid Plane!"
                  << std::endl;
        return false;
    }

    Scene::Object *grid = scene.addMesh(gridMesh, "grid_plane");

    if (!grid)
        return false;

    grid->transform.move(glm::vec3(0.0f, -1.5f, 0.0f));

    const auto addModel = [&scene](
                              const std::string &fileName,
                              const std::string &objectName,
                              const glm::vec3 &position,
                              float scale) -> bool
    {
        const std::string modelPath =
            std::string(ASSETS_DIR) +
            "sandbox/models/" +
            fileName;

        Scene::Object *object = scene.addModel(modelPath);

        if (!object)
        {
            std::cerr << "Falha ao adicionar o modelo: "
                      << modelPath << std::endl;
            return false;
        }

        object->name = objectName;
        object->transform.move(position);
        object->transform.setScale(glm::vec3(scale));

        std::cout << "[WORLD] Modelo carregado: "
                  << objectName << std::endl;

        return true;
    };

    if (!addModel(
            "Tablet_Vdd.glb",
            "tablet",
            glm::vec3(-1.50f, 0.15f, -2.5f),
            0.25f))
        return false;

    if (!addModel(
            "Mesa.glb",
            "mesa",
            glm::vec3(0.0f, -0.85f, -3.0f),
            0.35f))
        return false;

    if (!addModel(
            "Caderno.glb",
            "caderno",
            glm::vec3(-0.95f, -0.10f, -2.5f),
            0.25f))
        return false;

    if (!addModel(
            "Caneca.glb",
            "caneca",
            glm::vec3(-0.35f, 0.10f, -2.5f),
            0.20f))
        return false;

    if (!addModel(
            "celular.glb",
            "celular",
            glm::vec3(0.30f, 0.10f, -2.5f),
            0.20f))
        return false;

    if (!addModel(
            "Lapis.glb",
            "lapis",
            glm::vec3(0.85f, 0.10f, -2.5f),
            0.20f))
        return false;

    if (!addModel(
            "Lampada.glb",
            "lampada",
            glm::vec3(1.45f, 0.20f, -2.5f),
            0.25f))
        return false;

    if (!addModel(
            "Foto.glb",
            "foto",
            glm::vec3(-1.35f, 0.10f, -3.25f),
            0.20f))
        return false;

    if (!addModel(
            "Porta caneta.glb",
            "porta_caneta",
            glm::vec3(-0.30f, 0.10f, -3.25f),
            0.20f))
        return false;

    if (!addModel(
            "Suculenta_2.glb",
            "suculenta",
            glm::vec3(0.75f, 0.10f, -3.25f),
            0.20f))
        return false;

    if (!addModel(
            "Monitor.glb",
            "monitor",
            glm::vec3(0.0f, 0.25f, -3.45f),
            0.25f))
        return false;

    if (!addModel(
            "Teclado.glb",
            "teclado",
            glm::vec3(0.0f, -0.05f, -2.85f),
            0.20f))
        return false;

    Scene::Object *pyramid =
        scene.addMesh(pyramidMesh, "pyramid");

    if (!pyramid)
    {
        std::cout << "Falha ao adicionar a piramide na cena!"
                  << std::endl;
        return false;
    }

    pyramid->transform.move(glm::vec3(1.8f, -0.55f, -3.0f));
    pyramid->transform.setScale(glm::vec3(0.35f));

    // A pirâmide gira em torno do eixo Y.
    pyramid->update = [](Transform &transform, float deltaTime)
    {
        glm::vec3 newRotation = transform.rotation();

        newRotation.y = std::fmod(
            newRotation.y + 90.0f * deltaTime,
            360.0f);

        transform.setRotation(newRotation);
    };

    return true;
}

void handleModelShortcuts(
    const Window &window,
    Scene &scene,
    std::string &viewedModel)
{
    if (window.keyPressed(GLFW_KEY_1))
    {
        if (scene.showOnly("tablet"))
            viewedModel = "tablet";
        else
            std::cout << "Objeto tablet nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_2))
    {
        if (scene.showOnly("mesa"))
            viewedModel = "mesa";
        else
            std::cout << "Objeto mesa nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_3))
    {
        if (scene.showOnly("caderno"))
            viewedModel = "caderno";
        else
            std::cout << "Objeto caderno nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_4))
    {
        if (scene.showOnly("caneca"))
            viewedModel = "caneca";
        else
            std::cout << "Objeto caneca nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_5))
    {
        if (scene.showOnly("celular"))
            viewedModel = "celular";
        else
            std::cout << "Objeto celular nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_6))
    {
        if (scene.showOnly("lapis"))
            viewedModel = "lapis";
        else
            std::cout << "Objeto lapis nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_7))
    {
        if (scene.showOnly("lampada"))
            viewedModel = "lampada";
        else
            std::cout << "Objeto lampada nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_F8))
    {
        if (scene.showOnly("foto"))
            viewedModel = "foto";
        else
            std::cout << "Objeto foto nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_F9))
    {
        if (scene.showOnly("porta_caneta"))
            viewedModel = "porta_caneta";
        else
            std::cout << "Objeto porta-caneta nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_F10))
    {
        if (scene.showOnly("suculenta"))
            viewedModel = "suculenta";
        else
            std::cout << "Objeto suculenta nao encontrado." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_0))
    {
        scene.showAll();
        viewedModel = "Todos";
    }
}
