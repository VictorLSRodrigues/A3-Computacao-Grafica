#pragma once

#include <glm/glm.hpp>

struct BoundingBox
{
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};

    bool valid = false;

    void expand(const glm::vec3& point);
};