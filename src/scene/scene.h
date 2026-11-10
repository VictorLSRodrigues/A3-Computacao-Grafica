#pragma once

#include <deque>
#include <functional>
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
        // Opcional: chamada a cada quadro para animar o objeto.
        std::function<void(Transform &, float)> update;
        bool visible = true;
    };

    // Adiciona o modelo à cena, reutilizando-o se o caminho já estiver no cache.
    // Retorna o objeto criado ou nullptr se o carregamento falhar.
    Object *addModel(const std::string &path, const Transform &transform = Transform{});

    // Exibe somente o objeto com o nome informado. Retorna false se ele não existir.
    bool showOnly(const std::string &name);
    
    // Exibe todos os objetos.
    void showAll();
    
    // Adiciona à cena uma malha já criada e configurada.
    Object *addMesh(std::shared_ptr<Mesh> mesh, const std::string &name = "",
                    const Transform &transform = Transform{});

    // Atualiza as animações dos objetos; deltaTime é o tempo do quadro, em segundos.
    void update(float deltaTime);
    // Desenha os objetos com o shader ativo e a câmera configurada.
    void draw(Shader &shader, Camera &camera, const Texture &fallbackTexture);

private:
    std::map<std::string, std::shared_ptr<Model>> modelCache;
    std::deque<Object> objects; // Mantém válidos os ponteiros retornados ao adicionar objetos.
};