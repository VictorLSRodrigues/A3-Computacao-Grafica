#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <deque>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "engine/graphics/geometry/bounding_box.h"
#include "engine/graphics/mesh/mesh.h"

struct ModelLoadResult
{
    std::deque<Mesh> meshes;
    std::vector<glm::mat4> matricesMeshes;
    float boundingRadius = 0.0f;
    BoundingBox boundingBox;
};

// Interpreta arquivos glTF/GLB e prepara os recursos que compõem um Model.
class ModelLoader
{
public:
    static ModelLoadResult load(const std::string &path);
};

#endif
