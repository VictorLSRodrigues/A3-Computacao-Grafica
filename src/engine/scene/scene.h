#pragma once

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <glm/glm.hpp>

#include "engine/graphics/mesh/mesh.h"
#include "engine/graphics/model/model.h"
#include "transform.h"
#include <cstddef>

// Mantém os objetos da cena, responde às consultas de picking e executa operações
// sobre os objetos. Não guarda o que está selecionado (isso é do Editor) nem desenha
// (isso é do Renderer, que consulta os objetos por getObjects()).
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

        // Caixa envolvente do modelo ou da malha; nullptr se o objeto não tiver nenhum dos dois.
        const BoundingBox *boundingBox() const
        {
            if (model)
                return &model->getBoundingBox();

            if (mesh)
                return &mesh->getGeometry().getBoundingBox();

            return nullptr;
        }
    };

    // Identifica uma face dentro de uma malha de um objeto (resultado de pickFace).
    struct FaceSelection
    {
        std::size_t meshIndex = 0;
        unsigned int faceIndex = 0;
        bool valid = false;
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

    // Retorna o objeto mais próximo atingido pelo raio, ou nullptr se nenhum for atingido.
    Object *pickObject(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDirection);

    // Procura a face atingida pelo raio do mouse. Retorna false se nenhuma for atingida.
    // hitObject e hitFace recebem o resultado; sem acerto, ficam nullptr e inválida.
    bool pickFace(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDirection,
        Object *&hitObject,
        FaceSelection &hitFace);

    // Objetos da cena, somente para leitura.
    const std::deque<Object> &getObjects() const { return objects; }

    // Recupera as posições globais dos vértices de uma face do objeto.
    bool getFacePositions(
        const Object *object,
        const FaceSelection &face,
        glm::vec3 &a,
        glm::vec3 &b,
        glm::vec3 &c) const;

    // Inicia a extrusão de uma face do objeto.
    bool beginFaceExtrusion(Object *object, const FaceSelection &face);

    // Atualiza a distância da extrusão.
    bool updateFaceExtrusion(Object *object, const FaceSelection &face, float distance);

    // Confirma a extrusão; face passa a apontar para a nova face criada.
    void finishFaceExtrusion(Object *object, FaceSelection &face);

    // Cancela a extrusão.
    void cancelFaceExtrusion(Object *object, const FaceSelection &face);

    // Atualiza as animações dos objetos; deltaTime é o tempo do quadro, em segundos.
    void update(float deltaTime);

private:
    std::map<std::string, std::shared_ptr<Model>> modelCache;
    std::deque<Object> objects; // Mantém válidos os ponteiros retornados ao adicionar objetos.
};