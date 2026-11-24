#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

// Transformação individual de uma instância.
// A rotação pública continua expressa em graus (Euler), mas a orientação interna
// é mantida como quaternion para permitir integração rotacional estável.
class Transform
{
private:
    glm::vec3 position_{0.0f};
    glm::vec3 rotation_{0.0f};
    glm::vec3 scale_{1.0f};
    glm::quat orientation_{1.0f, 0.0f, 0.0f, 0.0f};

    void updateEulerFromOrientation()
    {
        // A ordem preserva a convenção histórica da matriz: Y, depois X, depois Z.
        float yaw = 0.0f;
        float pitch = 0.0f;
        float roll = 0.0f;
        glm::extractEulerAngleYXZ(
            glm::mat4_cast(orientation_), yaw, pitch, roll);
        rotation_ = glm::degrees(glm::vec3(pitch, yaw, roll));
    }

public:
    const glm::vec3& position() const { return position_; }
    const glm::vec3& rotation() const { return rotation_; }
    const glm::vec3& scale() const { return scale_; }
    const glm::quat& orientation() const { return orientation_; }

    void move(const glm::vec3& newPosition)
    {
        position_ = newPosition;
    }

    void translate(const glm::vec3& offset)
    {
        position_ += offset;
    }

    // Define uma rotação absoluta em graus, mantendo a ordem Y-X-Z anterior.
    void setRotation(const glm::vec3& eulerDegrees)
    {
        rotation_ = eulerDegrees;

        const glm::quat yaw = glm::angleAxis(
            glm::radians(eulerDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::quat pitch = glm::angleAxis(
            glm::radians(eulerDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::quat roll = glm::angleAxis(
            glm::radians(eulerDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));

        orientation_ = glm::normalize(yaw * pitch * roll);
    }

    // Define diretamente a orientação física. O Editor continua recebendo Euler
    // por rotation(), enquanto o Renderer usa o quaternion sem perda de composição.
    void setOrientation(const glm::quat& orientation)
    {
        const float normSquared = glm::dot(orientation, orientation);
        orientation_ = normSquared > 1e-12f
            ? glm::normalize(orientation)
            : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        updateEulerFromOrientation();
    }

    void rotate(const glm::vec3& eulerDelta)
    {
        setRotation(rotation_ + eulerDelta);
    }

    void setScale(const glm::vec3& newScale)
    {
        scale_ = newScale;
    }

    void scaleBy(const glm::vec3& factors)
    {
        scale_ *= factors;
    }

    glm::mat4 matrix() const
    {
        glm::mat4 result(1.0f);
        result = glm::translate(result, position_);
        result *= glm::mat4_cast(orientation_);
        result = glm::scale(result, scale_);
        return result;
    }
};
