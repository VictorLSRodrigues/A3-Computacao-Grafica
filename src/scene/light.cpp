#include "light.h"

#include <glm/gtc/matrix_transform.hpp>

#include "graphics/shapes.h"

bool Light::create()
{
    return marker.createIndexed(
        shapes::cube(),
        shapes::cubeIndices(),
        3,
        GL_TRIANGLES,
        {3});
}

glm::vec3 Light::direction() const
{
    const glm::vec3 toTarget = target - position;

    // Se posição e alvo coincidem não há direção; aponta para baixo.
    if (glm::length(toTarget) < 0.0001f)
        return glm::vec3(0.0f, -1.0f, 0.0f);

    return glm::normalize(toTarget);
}

void Light::apply(Shader &shader, LightType type, const Camera &camera) const
{
    shader.setVec4("lightColor", color.x, color.y, color.z, color.w);
    shader.setVec3("lightPos", position.x, position.y, position.z);
    shader.setVec3("camPos", camera.position.x, camera.position.y, camera.position.z);
    const glm::vec3 dir = direction();
    shader.setVec3("lightDir", dir.x, dir.y, dir.z);
    shader.setInt("lightType", static_cast<int>(type));
}

void Light::drawMarker(Shader &markerShader) const{
    glm::mat4 model(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(0.2f));

    markerShader.use();
    markerShader.setVec4("lightColor", color.x, color.y, color.z, color.w);
    markerShader.setMat4("model", model);

    marker.draw();
}
