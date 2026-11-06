#pragma once

#include <deque>
#include <map>
#include <memory>
#include <string>

#include "camera.h"
#include "graphics/mesh.h"
#include "graphics/model.h"
#include "graphics/shader.h"
#include "graphics/texture.h"
#include "transform.h"

// Mantém os objetos da cena e coordena o carregamento e o desenho deles.
class Scene
{
public:
    struct Object
    {
        std::string name;
        std::shared_ptr<Model> model; // Modelo carregado de arquivo.
        std::shared_ptr<Mesh> mesh;   // Malha criada no código; apenas um dos dois é usado.
        Transform transform;
    };

    // Adiciona o modelo à cena, reutilizando-o se o caminho já estiver no cache.
    // Retorna o objeto criado ou nullptr se o carregamento falhar.
    Object* addModel(const std::string& path, const Transform& transform = Transform{});

    // Adiciona à cena uma malha já criada e configurada.
    Object* addMesh(std::shared_ptr<Mesh> mesh, const std::string& name = "",
                    const Transform& transform = Transform{});

    // Desenha os objetos com o shader ativo e a câmera configurada.
    void draw(Shader& shader, Camera& camera, const Texture& fallbackTexture);

private:
    std::map<std::string, std::shared_ptr<Model>> modelCache;
    std::deque<Object> objects; // Mantém válidos os ponteiros retornados ao adicionar objetos.
};