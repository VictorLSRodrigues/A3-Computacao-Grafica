
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Transformação individual de uma instância.
// A rotação é expressa em graus.
class Transform
{
private:
    // Estado interno da transformação.
    glm::vec3 position_{0.0f};
    glm::vec3 rotation_{0.0f};
    glm::vec3 scale_{1.0f};

public:
    // Consultas do estado atual.
    const glm::vec3& position() const
    {
        return position_;
    }

    const glm::vec3& rotation() const
    {
        return rotation_;
    }

    const glm::vec3& scale() const
    {
        return scale_;
    }

    // Define uma posição absoluta.
    void move(const glm::vec3& newPosition)
    {
        position_ = newPosition;
    }

    // Aplica deslocamento relativo.
    void translate(const glm::vec3& offset)
    {
        position_ += offset;
    }

    // Define uma rotação absoluta em graus.
    void setRotation(const glm::vec3& eulerDegrees)
    {
        rotation_ = eulerDegrees;
    }

    // Aplica rotação incremental em graus.
    void rotate(const glm::vec3& eulerDelta)
    {
        rotation_ += eulerDelta;
    }

    // Define escala absoluta.
    void setScale(const glm::vec3& newScale)
    {
        scale_ = newScale;
    }

    // Aplica um fator multiplicativo à escala.
    void scaleBy(const glm::vec3& factors)
    {
        scale_ *= factors;
    }

    // Gera a Model Matrix a partir do estado atual.
    glm::mat4 matrix() const
    {
        glm::mat4 m(1.0f);

        m = glm::translate(m, position_);

        m = glm::rotate(
            m,
            glm::radians(rotation_.y),
            glm::vec3(0.0f, 1.0f, 0.0f));

        m = glm::rotate(
            m,
            glm::radians(rotation_.x),
            glm::vec3(1.0f, 0.0f, 0.0f));

        m = glm::rotate(
            m,
            glm::radians(rotation_.z),
            glm::vec3(0.0f, 0.0f, 1.0f));

        m = glm::scale(m, scale_);

        return m;
    }
};
