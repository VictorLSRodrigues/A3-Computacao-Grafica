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

    if (!diffuse.load(ASSETS_DIR "sandbox/textures/terra.png") ||
        !specular.load(ASSETS_DIR "sandbox/textures/terraSpecular.png"))
        return false;

    diffuse.setType("diffuse");
    specular.setType("specular");

    pyramidMesh->addTexture(std::move(diffuse));
    pyramidMesh->addTexture(std::move(specular));

    // Os modelos são opcionais: se algum falhar, a cena continua sem ele.
    if (Scene::Object* sacredMonkey = scene.addModel(ASSETS_DIR "sandbox/models/macaco_sagrado.glb"))
    {
        sacredMonkey->name = "macaco_sagrado";
        sacredMonkey->transform.position = glm::vec3(0.75f, 0.6f, -1.5f);
        sacredMonkey->transform.scale = glm::vec3(0.5f);
    }

    if (Scene::Object* lampada = scene.addModel(ASSETS_DIR "sandbox/models/Lampada.glb"))
    {
        lampada->name = "lampada";
        lampada->transform.position = glm::vec3(-0.75f, -1.48f, -1.5f);
        lampada->transform.scale = glm::vec3(0.5f);
    }


    //
    if (Scene::Object* multi = scene.addModel(ASSETS_DIR "sandbox/models/Lampada.glb"))
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