#pragma once

#include <glm/glm.hpp>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texUV;
    glm::vec3 normal;
    glm::vec4 tangent;
};