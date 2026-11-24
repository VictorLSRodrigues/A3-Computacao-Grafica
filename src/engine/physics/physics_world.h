#pragma once

#include <glm/glm.hpp>
#include <cstddef>
#include <vector>

#include "engine/scene/transform.h"

// Simulação de corpos esféricos sob gravidade, com contatos entre esferas e
// superfícies de apoio planas ou curvas (bacia). Usa passos fixos, impulso de
// contato, atrito e dinâmica rotacional de esfera maciça.
class PhysicsWorld
{
public:
    void setGravity(const glm::vec3& gravity);
    const glm::vec3& gravity() const;

    // Define um plano horizontal infinito na coordenada Y.
    // Coeficientes de atrito são limitados a [0, 1], com dinâmico <= estático.
    void setGroundPlane(
        float height,
        float staticFriction = 0.6f,
        float dynamicFriction = 0.4f);
    // Define uma bacia circular centrada na origem. O perfil é parabólico
    // no fundo e sobe mais rapidamente perto da borda. A colisão usa a mesma
    // superfície analítica, não apenas a malha visual.
    void setGroundBowl(
        float radius,
        float depth,
        float lipHeight,
        float staticFriction = 0.6f,
        float dynamicFriction = 0.4f);
    void clearGroundPlane();

    // Registra uma Transform para receber a integração física.
    // A posição, orientação e velocidades iniciais viram o ponto de partida (reset()).
    // O raio é local e acompanha a escala do Transform; a massa deve ser positiva.
    // Para raio efetivo positivo, o corpo usa I = 2/5 * massa * raio².
    bool addBody(
        Transform& transform,
        const glm::vec3& initialVelocity = glm::vec3(0.0f),
        float sphereRadius = 0.0f,
        float restitution = 0.0f,
        float mass = 1.0f,
        float staticFriction = 0.6f,
        float dynamicFriction = 0.4f,
        const glm::vec3& initialAngularVelocity = glm::vec3(0.0f));

    // Registra uma esfera imóvel, que colide com corpos dinâmicos.
    void addStaticBody(
        Transform& transform,
        float sphereRadius,
        float restitution = 0.0f,
        float staticFriction = 0.6f,
        float dynamicFriction = 0.4f);

    // Forças/torques pontuais são acumulados até um passo físico consumi-los.
    // Para uma força mantida enquanto uma tecla está pressionada, use
    // setContinuousForce() a cada frame (inclusive com vetor zero ao soltar).
    // Forças, pontos e torques usam coordenadas de mundo.
    bool applyForce(Transform& transform, const glm::vec3& force);
    bool applyForceAtPoint(
        Transform& transform,
        const glm::vec3& force,
        const glm::vec3& worldPoint);
    bool applyTorque(Transform& transform, const glm::vec3& torque);

    // Forças contínuas permanecem ativas até serem substituídas ou zeradas.
    // São avaliadas em cada subpasso físico, independentemente do FPS de renderização.
    bool setContinuousForce(Transform& transform, const glm::vec3& force);
    bool setContinuousTorque(Transform& transform, const glm::vec3& torque);

    // Impulsos lineares alteram a velocidade imediatamente.
    bool applyImpulse(Transform& transform, const glm::vec3& impulse);

    // Impulso angular altera a velocidade angular sem deslocar o centro de massa.
    // A unidade do argumento é N.m.s (impulso de torque).
    bool applyAngularImpulse(Transform& transform, const glm::vec3& angularImpulse);
    bool applyImpulseAtPoint(
        Transform& transform,
        const glm::vec3& impulse,
        const glm::vec3& worldPoint);

    void update(float frameTime);
    void reset();

    // Passa a usar posição e orientação atuais como ponto de partida (útil após o Editor).
    void recordStartPositions();

    void clear();
    std::size_t bodyCount() const;
    unsigned long long stepCount() const;

    // Consultas sobre o vínculo entre uma Transform e um corpo dinâmico.
    bool hasDynamicBody(const Transform& transform) const;

    // Consultas úteis para diagnóstico e testes; retornam zero se o corpo não estiver registrado.
    glm::vec3 linearVelocity(const Transform& transform) const;
    glm::vec3 angularVelocity(const Transform& transform) const;

private:
    struct Body
    {
        Transform* transform = nullptr;
        glm::vec3 velocity{0.0f};
        glm::vec3 angularVelocity{0.0f};
        glm::vec3 forceAccumulator{0.0f};
        glm::vec3 torqueAccumulator{0.0f};
        glm::vec3 continuousForce{0.0f};
        glm::vec3 continuousTorque{0.0f};
        glm::vec3 startPosition{0.0f};
        glm::vec3 startVelocity{0.0f};
        glm::vec3 startAngularVelocity{0.0f};
        glm::quat startOrientation{1.0f, 0.0f, 0.0f, 0.0f};
        // Raio informado em espaço local e raio efetivo em espaço de mundo.
        float baseSphereRadius = 0.0f;
        float sphereRadius = 0.0f;
        float restitution = 0.0f;
        float staticFriction = 0.0f;
        float dynamicFriction = 0.0f;
        float inverseMass = 1.0f;
        float inverseInertia = 0.0f;
    };

    Body* findBody(Transform& transform);
    const Body* findBody(const Transform& transform) const;
    void step(float dt);
    void synchronizeBodyScale(Body& body);
    void resolveGroundCollisions();
    void resolveSphereCollisions();
    void addBodyInternal(
        Transform& transform,
        const glm::vec3& initialVelocity,
        const glm::vec3& initialAngularVelocity,
        float sphereRadius,
        float restitution,
        float staticFriction,
        float dynamicFriction,
        float inverseMass);

    static constexpr double kFixedStep = 1.0 / 120.0;
    static constexpr int kMaxStepsPerFrame = 12;
    static constexpr int kCollisionIterations = 8;
    static constexpr int kMaxAdaptiveSubsteps = 64;
    static constexpr double kTolerance = 1e-6;
    static constexpr float kRestitutionVelocityThreshold = 0.5f;
    static constexpr float kPenetrationSlop = 0.001f;
    static constexpr float kPositionCorrection = 0.8f;

    glm::vec3 gravity_{0.0f, -9.81f, 0.0f};
    std::vector<Body> bodies_;
    bool hasGroundPlane_ = false;
    bool hasGroundBowl_ = false;
    float groundHeight_ = 0.0f;
    float groundBowlRadius_ = 0.0f;
    float groundBowlDepth_ = 0.0f;
    float groundBowlLipHeight_ = 0.0f;
    float groundStaticFriction_ = 0.6f;
    float groundDynamicFriction_ = 0.4f;
    double accumulator_ = 0.0;
    unsigned long long stepCount_ = 0;
};
