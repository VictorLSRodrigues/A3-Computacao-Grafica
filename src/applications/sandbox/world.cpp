#include "world.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include "engine/graphics/mesh/mesh.h"
#include "engine/graphics/geometry/shapes.h"
#include "engine/graphics/resources/texture.h"

bool buildWorld(Scene &scene)
{
    // Cada vértice guarda posição (3), cor (3), coordenadas da textura (2) e normal (3).
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
        !specular.load(ASSETS_DIR "sandbox/textures/terraSpecular.png", true, TextureColorSpace::Linear))
        return false;

    diffuse.setType("diffuse");
    specular.setType("specular");

    pyramidMesh->addTexture(std::move(diffuse));
    pyramidMesh->addTexture(std::move(specular));

    // Os modelos são opcionais: se algum falhar, a cena continua sem ele.
    if (Scene::Object *sacredMonkey = scene.addModel(ASSETS_DIR "sandbox/models/macaco_sagrado.glb"))
    {
        sacredMonkey->name = "macaco_sagrado";
        sacredMonkey->transform.move(
            glm::vec3(0.75f, 0.6f, -1.5f));

        sacredMonkey->transform.setScale(
            glm::vec3(0.5f));
    }

    if (Scene::Object *lampada = scene.addModel(ASSETS_DIR "sandbox/models/lampada.glb"))
    {
        lampada->name = "lampada";
        lampada->transform.move(
            glm::vec3(-0.75f, -1.48f, -1.5f));

        lampada->transform.setScale(
            glm::vec3(0.5f));
    }

    //
    if (Scene::Object *multi = scene.addModel(ASSETS_DIR "sandbox/models/Caneta.glb"))
    {
        multi->name = "multi";
        const float multiScale = 0.1f; // ajuste aqui: menor = modelo menor
        multi->transform.move(
            glm::vec3(-1.5f, 0.0f, 0.0f));

        multi->transform.setScale(
            glm::vec3(multiScale));
    }

    Scene::Object *pyramid = scene.addMesh(pyramidMesh, "pyramid");
    if (!pyramid)
        return false;

    // A pirâmide gira em torno do eixo Y, 90 graus por segundo.
    pyramid->update = [](Transform& transform, float deltaTime)
{
    glm::vec3 newRotation = transform.rotation();

    newRotation.y = std::fmod(
        newRotation.y + 90.0f * deltaTime,
        360.0f
    );

    transform.setRotation(newRotation);
};
    return true;
}

void handleModelShortcuts(const Window &window, Scene &scene, std::string &viewedModel)
{
    if (window.keyPressed(GLFW_KEY_1))
    {
        if (scene.showOnly("macaco_sagrado"))
            viewedModel = "macaco";
        else
            std::cout << "Objeto macaco nao encontrado na cena." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_2))
    {
        if (scene.showOnly("multi"))
            viewedModel = "multi";
        else
            std::cout << "Objeto multi nao encontrado na cena." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_3))
    {
        if (scene.showOnly("pyramid"))
            viewedModel = "pyramid";
        else
            std::cout << "Objeto pyramid nao encontrado na cena." << std::endl;
    }
    else if (window.keyPressed(GLFW_KEY_0))
    {
        scene.showAll();
        viewedModel = "Todos";
    }
}
