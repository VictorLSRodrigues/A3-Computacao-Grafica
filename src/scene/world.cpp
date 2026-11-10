#include "world.h"

#include <cmath>
#include <iostream>
#include <memory>
#include <utility>

#include "graphics/mesh.h"
#include "graphics/shapes.h"
#include "graphics/texture.h"

bool buildWorld(Scene& scene)
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

    if (!diffuse.load(ASSETS_DIR "textures/terra.png") ||
        !specular.load(ASSETS_DIR "textures/terraSpecular.png"))
        return false;

    diffuse.setType("diffuse");
    specular.setType("specular");

    pyramidMesh->addTexture(std::move(diffuse));
    pyramidMesh->addTexture(std::move(specular));

    // Os modelos são opcionais: se algum falhar, a cena continua sem ele.
    if (Scene::Object* monkey = scene.addModel(ASSETS_DIR "models/macaco.glb"))
    {
        monkey->name = "macaco";
        monkey->transform.position = glm::vec3(1.5f, 0.0f, 0.0f);
        monkey->transform.scale = glm::vec3(0.5f);
    }

    //
    if (Scene::Object* multi = scene.addModel(ASSETS_DIR "models/multi.glb"))
    {
        multi->name = "multi";
        const float multiScale = 0.1f; // ajuste aqui: menor = modelo menor
        multi->transform.position = glm::vec3(-1.5f, 0.0f, 0.0f);
        multi->transform.scale = glm::vec3(multiScale);
    }

    Scene::Object* pyramid = scene.addMesh(pyramidMesh, "pyramid");
    if (!pyramid)
        return false;

    // A pirâmide gira em torno do eixo Y, 90 graus por segundo.
    pyramid->update = [](Transform& transform, float deltaTime)
    {
        transform.rotation.y = std::fmod(transform.rotation.y + 90.0f * deltaTime, 360.0f);
    };

    return true;
}