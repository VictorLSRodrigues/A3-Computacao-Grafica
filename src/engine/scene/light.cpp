#include "light.h"

#include <glm/gtc/matrix_transform.hpp>

#include "engine/graphics/geometry/shapes.h"

bool Light::create()
{
    return marker.createIndexed(
        shapes::cube(),
        shapes::cubeIndices(),
        3,
        GL_TRIANGLES,
        {3});
}

void Light::resetProperties()
{
    type = kDefaultType;
    color = kDefaultColor;
    position = kDefaultPosition;
    target = kDefaultTarget;
    intensity = kDefaultIntensity;
}

glm::vec3 Light::direction() const
{
    const glm::vec3 toTarget = target - position;

    // Se posição e alvo coincidem não há direção; aponta para baixo.
    if (glm::length(toTarget) < 0.0001f)
        return glm::vec3(0.0f, -1.0f, 0.0f);

    return glm::normalize(toTarget);
}

void Light::apply(Shader &shader, const Camera &camera) const
{
    shader.setVec4("lightColor", color.x, color.y, color.z, 1.0f);
    shader.setFloat("lightIntensity", intensity);
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
    markerShader.setVec4("lightColor", color.x, color.y, color.z, 1.0f);
    markerShader.setMat4("model", model);

    marker.draw();
}
