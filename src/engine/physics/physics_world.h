#pragma once

#include <glm/glm.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "engine/physics/physics_types.h"
#include "engine/physics/collision/collision_geometry.h"
#include "engine/physics/solver/contact_solver.h"

namespace rotor::physics
{

// Coordena corpos esféricos, caixas orientadas e malhas trianguladas.
// A geometria de colisão e a resolução de contatos são delegadas aos módulos
// collision e solver; o World controla tempo, integração e ordem dos sistemas.
class PhysicsWorld
{
public:
    void setGravity(const glm::vec3& gravity);
    const glm::vec3& gravity() const;

    // Escala apenas o tempo da simulação física. 1.0 usa tempo real,
    // 0.5 reproduz câmera lenta e 2.0 acelera a física.
    void setTimeScale(float timeScale);
    float timeScale() const;

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

    // Registra uma esfera dinâmica. A Transform representa o centro da esfera.
    // O raio é local e acompanha a maior escala absoluta do Transform; a massa
    // deve ser positiva. Para raio efetivo positivo, usa I = 2/5 * massa * raio².
    bool addSphereBody(
        Transform& transform,
        const glm::vec3& initialVelocity = glm::vec3(0.0f),
        float sphereRadius = 0.0f,
        float restitution = 0.0f,
        float mass = 1.0f,
        float staticFriction = 0.6f,
        float dynamicFriction = 0.4f,
        const glm::vec3& initialAngularVelocity = glm::vec3(0.0f));

    // Registra uma caixa dinâmica orientada. A Transform representa o centro
    // geométrico do colisor; as meias-dimensões são locais e acompanham sua escala.
    bool addBoxBody(
        Transform& transform,
        const glm::vec3& localHalfExtents,
        const glm::vec3& initialVelocity = glm::vec3(0.0f),
        float restitution = 0.0f,
        float mass = 1.0f,
        float staticFriction = 0.6f,
        float dynamicFriction = 0.4f,
        const glm::vec3& initialAngularVelocity = glm::vec3(0.0f));

    // Registra um corpo dinâmico usando os vértices e triângulos da geometria
    // original. A Transform representa o centro de massa aproximado da malha.
    // Contatos com superfícies estáticas usam os vértices reais; colisões entre
    // dois corpos dinâmicos ainda usam a OBB envolvente nesta primeira versão.
    bool addTriangleMeshBody(
        Transform& transform,
        std::shared_ptr<const TriangleMeshColliderData> mesh,
        float restitution = 0.0f,
        float mass = 1.0f,
        float staticFriction = 0.6f,
        float dynamicFriction = 0.4f,
        const glm::vec3& initialVelocity = glm::vec3(0.0f),
        const glm::vec3& initialAngularVelocity = glm::vec3(0.0f));

    // Registra uma esfera imóvel, que colide com corpos dinâmicos.
    void addStaticSphere(
        Transform& transform,
        float sphereRadius,
        float restitution = 0.0f,
        float staticFriction = 0.6f,
        float dynamicFriction = 0.4f);

    // Registra uma caixa imóvel orientada (OBB). O centro e as meias-dimensões
    // são locais ao Transform e acompanham sua posição, rotação e escala.
    // Útil para superfícies rígidas como mesas e plataformas.
    bool addStaticBox(
        Transform& transform,
        const glm::vec3& localCenter,
        const glm::vec3& localHalfExtents,
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
    bool setLinearDamping(Transform& transform, float damping);
    bool setAngularDamping(Transform& transform, float damping);

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
    // Mostra um resumo de profiling por segundo no console e em physics_profile.log.
    // Pode ser desligado sem alterar o comportamento da simulação.
    void setProfilingEnabled(bool enabled);
    bool profilingEnabled() const;
    void reset();

    // Remove sobreposições já existentes antes de iniciar a simulação, sem gerar
    // velocidades ou impulsos artificiais. Útil após carregar/editar uma cena.
    void stabilizeInitialOverlaps(int iterations = 24);

    // Passa a usar posição e orientação atuais como ponto de partida (útil após o Editor).
    void captureInitialPose();

    void clear();
    std::size_t bodyCount() const;
    unsigned long long stepCount() const;

    // Consultas sobre o vínculo entre uma Transform e um corpo dinâmico.
    bool hasDynamicBody(const Transform& transform) const;

    // Consultas úteis para diagnóstico e testes; retornam zero se o corpo não estiver registrado.
    glm::vec3 linearVelocity(const Transform& transform) const;
    glm::vec3 angularVelocity(const Transform& transform) const;

private:
    using Body = RigidBody;
    using BodyShape = ColliderShape;

    Body* findBody(Transform& transform);
    const Body* findBody(const Transform& transform) const;
    bool hasStaticBoxCollider(const Transform& transform) const;
    struct ContactConstraint
    {
        RigidBody* bodyA = nullptr;
        RigidBody* bodyB = nullptr;
        ContactManifold manifold;
        PhysicsMaterial material;
        solver::ContactSolverCache solverCache;
        // Posições na última avaliação dos contatos. A correção posicional é
        // translacional; podemos atualizar a penetração sem reconstruir a malha.
        glm::vec3 lastPositionA{0.0f};
        glm::vec3 lastPositionB{0.0f};
    };

    struct AxisAlignedBounds
    {
        glm::vec3 minimum{0.0f};
        glm::vec3 maximum{0.0f};
    };

    struct IndexedBounds
    {
        std::size_t index = 0;
        AxisAlignedBounds bounds;
    };

    struct BodyPair
    {
        std::size_t a = 0;
        std::size_t b = 0;
    };

    struct BodyMotionProxy
    {
        AxisAlignedBounds sweptBounds;
        glm::vec3 linearDisplacement{0.0f};
        float angularTravel = 0.0f;
        float minimumHalfExtent = 0.0f;
        float totalTravel = 0.0f;
        bool active = false;
    };

    struct ProfilingWindow
    {
        std::uint64_t updateCalls = 0;
        std::uint64_t fixedSteps = 0;
        std::uint64_t substeps = 0;
        std::uint64_t droppedTimeEvents = 0;
        std::uint64_t contactCollectionCalls = 0;
        std::uint64_t initialManifolds = 0;
        std::uint64_t initialContactPoints = 0;
        std::uint64_t positionCollectionCalls = 0;
        std::uint64_t positionManifolds = 0;
        std::uint64_t positionContactPoints = 0;
        std::uint64_t positionPasses = 0;
        std::uint64_t velocityIterations = 0;
        std::uint64_t velocityContactSolves = 0;
        std::uint64_t bodyPairPossible = 0;
        std::uint64_t bodyPairTests = 0;
        std::uint64_t bodyPairAabbRejected = 0;
        std::uint64_t bodyPairHits = 0;
        std::uint64_t staticBoxTests = 0;
        std::uint64_t staticBoxAabbRejected = 0;
        std::uint64_t groundBodyTests = 0;
        std::uint64_t groundAabbRejected = 0;
        std::uint64_t maxContactManifolds = 0;
        std::uint64_t maxStepsInUpdate = 0;
        std::uint64_t maxSubstepsInFixedStep = 0;
        double inputSimulationSeconds = 0.0;
        double stepMilliseconds = 0.0;
        double maxStepMilliseconds = 0.0;
        double setupMilliseconds = 0.0;
        double integrationMilliseconds = 0.0;
        double initialContactMilliseconds = 0.0;
        double contactPreparationMilliseconds = 0.0;
        double velocitySolveMilliseconds = 0.0;
        double positionContactMilliseconds = 0.0;
        double positionCorrectionMilliseconds = 0.0;
    };

    void step(float dt);
    void collectContactConstraints(std::vector<ContactConstraint>& contacts);
    static AxisAlignedBounds makeBodyBounds(const RigidBody& body);
    static AxisAlignedBounds makeStaticBoxBounds(const StaticBoxCollider& collider);
    static bool overlaps(const AxisAlignedBounds& a, const AxisAlignedBounds& b);
    void emitProfilingReportIfDue();
    void addSphereBodyInternal(
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
    static constexpr int kCollisionIterations = 48;
    static constexpr int kPositionIterations = 6;
    static constexpr int kMaxAdaptiveSubsteps = 32;
    static constexpr double kTolerance = 1e-6;

    glm::vec3 gravity_{0.0f, -9.81f, 0.0f};
    std::vector<Body> bodies_;
    std::vector<StaticBoxCollider> staticBoxes_;
    // Workspaces persist between fixed steps; collecting contacts no longer
    // allocates fresh vectors for every positional-correction pass.
    std::vector<ContactConstraint> contactsScratch_;
    collision::MeshCollisionScratch meshCollisionScratch_;

    // Broadphase sweep-and-prune e dados de movimento reutilizados entre passos.
    std::vector<AxisAlignedBounds> bodyBoundsByIndexScratch_;
    std::vector<IndexedBounds> bodySweepScratch_;
    std::vector<IndexedBounds> staticBoxBoundsScratch_;
    std::vector<BodyPair> bodyPairScratch_;
    std::vector<BodyMotionProxy> bodyMotionScratch_;
    bool hasGroundPlane_ = false;
    bool hasGroundBowl_ = false;
    float groundHeight_ = 0.0f;
    float groundBowlRadius_ = 0.0f;
    float groundBowlDepth_ = 0.0f;
    float groundBowlLipHeight_ = 0.0f;
    float groundStaticFriction_ = 0.6f;
    float groundDynamicFriction_ = 0.4f;
    float timeScale_ = 1.0f;
    double accumulator_ = 0.0;
    unsigned long long stepCount_ = 0;

    bool profilingEnabled_ = false;
    bool profileCollectionActive_ = false;
    ProfilingWindow profilingWindow_;
    std::chrono::steady_clock::time_point profilingWindowStart_{};
};

} // namespace rotor::physics

