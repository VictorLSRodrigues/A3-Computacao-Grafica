#include "scene.h"

#include <iostream>
#include <utility>

Scene::Object *Scene::addModel(const std::string &path, const Transform &transform)
{
    std::shared_ptr<Model> model;

    auto cached = modelCache.find(path);
    if (cached != modelCache.end())
    {
        model = cached->second;
    }
    else
    {
        try
        {
            model = std::make_shared<Model>(path.c_str());
        }
        catch (const std::exception &error)
        {
            std::cout << "Falha ao carregar o modelo " << path << ": " << error.what() << std::endl;
            return nullptr;
        }
        modelCache[path] = model;
    }

    objects.push_back({path, model, nullptr, transform});
    return &objects.back();
}

Scene::Object *Scene::addMesh(std::shared_ptr<Mesh> mesh, const std::string &name,
                              const Transform &transform)
{
    if (!mesh)
        return nullptr;

    objects.push_back({name, nullptr, std::move(mesh), transform});
    return &objects.back();
}

void Scene::draw(Shader &shader, Camera &camera, const Texture &fallbackTexture)
{
    for (Object &object : objects)
    {
        const glm::mat4 matrix = object.transform.matrix();

        if (object.model)
        {
            // O modelo envia a matriz de cada parte dele, já combinada com a do objeto.
            object.model->draw(shader, camera, fallbackTexture, matrix);
        }
        else if (object.mesh)
        {
            shader.setMat4("model", matrix);
            object.mesh->draw(shader, &fallbackTexture);
        }
    }
}