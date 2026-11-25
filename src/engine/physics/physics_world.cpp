#include "physics_world.h"
#include "engine/physics/solver/contact_solver.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <glm/gtx/quaternion.hpp>

namespace rotor::physics
{


namespace
{
float clampFriction(float coefficient)
{
    return std::isfinite(coefficient)
        ? std::clamp(coefficient, 0.0f, 1.0f) : 0.0f;
}

float clampRestitution(float coefficient)
{
    return std::isfinite(coefficient)
        ? std::clamp(coefficient, 0.0f, 1.0f) : 0.0f;
}

bool isFiniteVector(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z);
}
}

PhysicsWorld::AxisAlignedBounds PhysicsWorld::makeBodyBounds(const RigidBody& body)
{
    AxisAlignedBounds bounds;
    const float infinity = std::numeric_limits<float>::infinity();
    bounds.minimum = glm::vec3(infinity);
    bounds.maximum = glm::vec3(-infinity);
    if (!body.transform)
        return bounds;

    const glm::vec3 position = body.transform->position();
    if (!isFiniteVector(position))
        return bounds;
    if (body.collider.shape == ColliderShape::Sphere)
    {
        const float radius = std::max(0.0f, body.collider.sphereRadius);
        if (!std::isfinite(radius))
            return bounds;
        const glm::vec3 extent(radius);
        bounds.minimum = position - extent;
        bounds.maximum = position + extent;
        return bounds;
    }

    const glm::quat orientation = body.transform->orientation();
    glm::vec3 center = position;
    if (body.collider.shape == ColliderShape::TriangleMesh)
        center += orientation * body.collider.boxCenter;

    const glm::mat3 rotation = glm::mat3_cast(orientation);
    const glm::vec3 halfExtents = glm::max(body.collider.boxHalfExtents, glm::vec3(0.0f));
    const glm::vec3 worldExtent =
        glm::abs(rotation[0]) * halfExtents.x +
        glm::abs(rotation[1]) * halfExtents.y +
        glm::abs(rotation[2]) * halfExtents.z;
    if (!isFiniteVector(center) || !isFiniteVector(worldExtent))
        return bounds;
    bounds.minimum = center - worldExtent;
    bounds.maximum = center + worldExtent;
    return bounds;
}

PhysicsWorld::AxisAlignedBounds PhysicsWorld::makeStaticBoxBounds(
    const StaticBoxCollider& collider)
{
    AxisAlignedBounds bounds;
    const float infinity = std::numeric_limits<float>::infinity();
    bounds.minimum = glm::vec3(infinity);
    bounds.maximum = glm::vec3(-infinity);
    if (!collider.transform)
        return bounds;

    const glm::vec3 scale = collider.transform->scale();
    if (!isFiniteVector(scale))
        return bounds;
    const glm::vec3 absoluteScale = glm::abs(scale);
    const glm::quat orientation = collider.transform->orientation();
    const glm::vec3 center = collider.transform->position() +
        orientation * (scale * collider.localCenter);
    const glm::vec3 halfExtents = absoluteScale * collider.localHalfExtents;
    const glm::mat3 rotation = glm::mat3_cast(orientation);
    const glm::vec3 worldExtent =
        glm::abs(rotation[0]) * halfExtents.x +
        glm::abs(rotation[1]) * halfExtents.y +
        glm::abs(rotation[2]) * halfExtents.z;
    if (!isFiniteVector(center) || !isFiniteVector(worldExtent))
        return bounds;
    bounds.minimum = center - worldExtent;
    bounds.maximum = center + worldExtent;
    return bounds;
}

bool PhysicsWorld::overlaps(
    const AxisAlignedBounds& a,
    const AxisAlignedBounds& b)
{
    if (!isFiniteVector(a.minimum) || !isFiniteVector(a.maximum) ||
        !isFiniteVector(b.minimum) || !isFiniteVector(b.maximum) ||
        a.minimum.x > a.maximum.x || a.minimum.y > a.maximum.y ||
        a.minimum.z > a.maximum.z || b.minimum.x > b.maximum.x ||
        b.minimum.y > b.maximum.y || b.minimum.z > b.maximum.z)
        return false;
    constexpr float contactMargin = 1e-4f;
    return !(a.maximum.x + contactMargin < b.minimum.x ||
             a.minimum.x - contactMargin > b.maximum.x ||
             a.maximum.y + contactMargin < b.minimum.y ||
             a.minimum.y - contactMargin > b.maximum.y ||
             a.maximum.z + contactMargin < b.minimum.z ||
             a.minimum.z - contactMargin > b.maximum.z);
}

void PhysicsWorld::setGravity(const glm::vec3& gravity)
{
    if (isFiniteVector(gravity))
        gravity_ = gravity;
}

const glm::vec3& PhysicsWorld::gravity() const
{
    return gravity_;
}

void PhysicsWorld::setTimeScale(float timeScale)
{
    if (std::isfinite(timeScale))
        timeScale_ = std::clamp(timeScale, 0.1f, 2.0f);
}

float PhysicsWorld::timeScale() const
{
    return timeScale_;
}

void PhysicsWorld::setGroundPlane(
    float height,
    float staticFriction,
    float dynamicFriction)
{
    if (!std::isfinite(height))
        return;
    groundHeight_ = height;
    hasGroundPlane_ = true;
    hasGroundBowl_ = false;
    groundStaticFriction_ = clampFriction(staticFriction);
    groundDynamicFriction_ = std::min(
        clampFriction(dynamicFriction), groundStaticFriction_);
}

void PhysicsWorld::setGroundBowl(
    float radius,
    float depth,
    float lipHeight,
    float staticFriction,
    float dynamicFriction)
{
    if (!std::isfinite(radius) || !std::isfinite(depth) ||
        !std::isfinite(lipHeight) || radius <= 0.0f ||
        depth < 0.0f || lipHeight < 0.0f)
    {
        clearGroundPlane();
        return;
    }

    hasGroundPlane_ = false;
    hasGroundBowl_ = true;
    groundBowlRadius_ = radius;
    groundBowlDepth_ = depth;
    groundBowlLipHeight_ = lipHeight;
    groundStaticFriction_ = clampFriction(staticFriction);
    groundDynamicFriction_ = std::min(
        clampFriction(dynamicFriction), groundStaticFriction_);
}

void PhysicsWorld::clearGroundPlane()
{
    hasGroundPlane_ = false;
    hasGroundBowl_ = false;
}

bool PhysicsWorld::addSphereBody(
    Transform& transform,
    const glm::vec3& initialVelocity,
    float sphereRadius,
    float restitution,
    float mass,
    float staticFriction,
    float dynamicFriction,
    const glm::vec3& initialAngularVelocity)
{
    if (findBody(transform) || hasStaticBoxCollider(transform) ||
        !isFiniteVector(initialVelocity) ||
        !isFiniteVector(initialAngularVelocity) || !std::isfinite(sphereRadius) ||
        sphereRadius < 0.0f || !(mass > 0.0f) || !std::isfinite(mass))
        return false;

    const float inverseMass = 1.0f / mass;
    if (!std::isfinite(inverseMass))
        return false;

    addSphereBodyInternal(
        transform,
        initialVelocity,
        initialAngularVelocity,
        sphereRadius,
        restitution,
        staticFriction,
        dynamicFriction,
        inverseMass);
    return true;
}

bool PhysicsWorld::addBoxBody(
    Transform& transform,
    const glm::vec3& localHalfExtents,
    const glm::vec3& initialVelocity,
    float restitution,
    float mass,
    float staticFriction,
    float dynamicFriction,
    const glm::vec3& initialAngularVelocity)
{
    if (findBody(transform) || hasStaticBoxCollider(transform) ||
        !isFiniteVector(localHalfExtents) ||
        localHalfExtents.x <= 1e-5f ||
        localHalfExtents.y <= 1e-5f ||
        localHalfExtents.z <= 1e-5f ||
        !isFiniteVector(initialVelocity) ||
        !isFiniteVector(initialAngularVelocity) ||
        !(mass > 0.0f) || !std::isfinite(mass))
    {
        return false;
    }

    const float inverseMass = 1.0f / mass;
    if (!std::isfinite(inverseMass))
        return false;

    Body body;
    body.transform = &transform;
    body.collider.shape = BodyShape::Box;
    body.velocity = initialVelocity;
    body.angularVelocity = initialAngularVelocity;
    body.startPosition = transform.position();
    body.startVelocity = initialVelocity;
    body.startAngularVelocity = initialAngularVelocity;
    body.startOrientation = transform.orientation();
    body.collider.baseBoxHalfExtents = localHalfExtents;
    body.material.restitution = clampRestitution(restitution);
    body.material.staticFriction = clampFriction(staticFriction);
    body.material.dynamicFriction = std::min(
        clampFriction(dynamicFriction), body.material.staticFriction);
    body.inverseMass = inverseMass;

    bodies_.push_back(body);
    bodies_.back().synchronizeColliderAndInertia();
    return true;
}

bool PhysicsWorld::addTriangleMeshBody(
    Transform& transform,
    std::shared_ptr<const TriangleMeshColliderData> mesh,
    float restitution,
    float mass,
    float staticFriction,
    float dynamicFriction,
    const glm::vec3& initialVelocity,
    const glm::vec3& initialAngularVelocity)
{
    if (findBody(transform) || hasStaticBoxCollider(transform) ||
        !mesh || !mesh->valid() || !isFiniteVector(initialVelocity) ||
        !isFiniteVector(initialAngularVelocity) || !(mass > 0.0f) ||
        !std::isfinite(mass))
    {
        return false;
    }

    const float inverseMass = 1.0f / mass;
    if (!std::isfinite(inverseMass))
        return false;

    Body body;
    body.transform = &transform;
    body.collider.shape = BodyShape::TriangleMesh;
    body.collider.triangleMesh = std::move(mesh);
    body.collider.baseBoxHalfExtents = body.collider.triangleMesh->localHalfExtents;
    body.velocity = initialVelocity;
    body.angularVelocity = initialAngularVelocity;
    body.startPosition = transform.position();
    body.startVelocity = initialVelocity;
    body.startAngularVelocity = initialAngularVelocity;
    body.startOrientation = transform.orientation();
    body.material.restitution = clampRestitution(restitution);
    body.material.staticFriction = clampFriction(staticFriction);
    body.material.dynamicFriction = std::min(
        clampFriction(dynamicFriction), body.material.staticFriction);
    body.inverseMass = inverseMass;

    bodies_.push_back(std::move(body));
    bodies_.back().synchronizeColliderAndInertia();
    return true;
}

void PhysicsWorld::addStaticSphere(
    Transform& transform,
    float sphereRadius,
    float restitution,
    float staticFriction,
    float dynamicFriction)
{
    if (findBody(transform) || hasStaticBoxCollider(transform))
        return;
    addSphereBodyInternal(
        transform,
        glm::vec3(0.0f),
        glm::vec3(0.0f),
        sphereRadius,
        restitution,
        staticFriction,
        dynamicFriction,
        0.0f);
}

bool PhysicsWorld::addStaticBox(
    Transform& transform,
    const glm::vec3& localCenter,
    const glm::vec3& localHalfExtents,
    float restitution,
    float staticFriction,
    float dynamicFriction)
{
    if (findBody(transform) || hasStaticBoxCollider(transform) ||
        !isFiniteVector(localCenter) || !isFiniteVector(localHalfExtents) ||
        localHalfExtents.x <= 0.0f || localHalfExtents.y <= 0.0f ||
        localHalfExtents.z <= 0.0f)
    {
        return false;
    }

    StaticBoxCollider collider;
    collider.transform = &transform;
    collider.localCenter = localCenter;
    collider.localHalfExtents = localHalfExtents;
    collider.material.restitution = clampRestitution(restitution);
    collider.material.staticFriction = clampFriction(staticFriction);
    collider.material.dynamicFriction = std::min(
        clampFriction(dynamicFriction), collider.material.staticFriction);
    staticBoxes_.push_back(collider);
    return true;
}

void PhysicsWorld::addSphereBodyInternal(
    Transform& transform,
    const glm::vec3& initialVelocity,
    const glm::vec3& initialAngularVelocity,
    float sphereRadius,
    float restitution,
    float staticFriction,
    float dynamicFriction,
    float inverseMass)
{
    if (findBody(transform) || hasStaticBoxCollider(transform) ||
        !isFiniteVector(initialVelocity) || !isFiniteVector(initialAngularVelocity) ||
        !std::isfinite(sphereRadius) ||
        sphereRadius < 0.0f || !std::isfinite(inverseMass) || inverseMass < 0.0f)
        return;

    Body body;
    body.transform = &transform;
    body.collider.shape = BodyShape::Sphere;
    body.velocity = initialVelocity;
    body.angularVelocity = initialAngularVelocity;
    body.startPosition = transform.position();
    body.startVelocity = initialVelocity;
    body.startAngularVelocity = initialAngularVelocity;
    body.startOrientation = transform.orientation();
    body.collider.baseSphereRadius = std::max(0.0f, sphereRadius);
    body.collider.sphereRadius = body.collider.baseSphereRadius;
    body.material.restitution = clampRestitution(restitution);
    body.material.staticFriction = clampFriction(staticFriction);
    body.material.dynamicFriction = std::min(
        clampFriction(dynamicFriction), body.material.staticFriction);
    body.inverseMass = inverseMass;

    bodies_.push_back(body);
    bodies_.back().synchronizeColliderAndInertia();
}


PhysicsWorld::Body* PhysicsWorld::findBody(Transform& transform)
{
    for (Body& body : bodies_)
    {
        if (body.transform == &transform)
            return &body;
    }
    return nullptr;
}

const PhysicsWorld::Body* PhysicsWorld::findBody(const Transform& transform) const
{
    for (const Body& body : bodies_)
    {
        if (body.transform == &transform)
            return &body;
    }
    return nullptr;
}

bool PhysicsWorld::hasStaticBoxCollider(const Transform& transform) const
{
    for (const StaticBoxCollider& collider : staticBoxes_)
    {
        if (collider.transform == &transform)
            return true;
    }
    return false;
}

bool PhysicsWorld::applyForce(Transform& transform, const glm::vec3& force)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(force))
        return false;

    body->forceAccumulator += force;
    return true;
}

bool PhysicsWorld::applyForceAtPoint(
    Transform& transform,
    const glm::vec3& force,
    const glm::vec3& worldPoint)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f ||
        !isFiniteVector(force) || !isFiniteVector(worldPoint))
        return false;

    body->forceAccumulator += force;
    body->torqueAccumulator += glm::cross(
        worldPoint - transform.position(), force);
    return true;
}

bool PhysicsWorld::applyTorque(Transform& transform, const glm::vec3& torque)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(torque))
        return false;
    body->synchronizeColliderAndInertia();
    if (body->inverseMass <= 0.0f)
        return false;

    body->torqueAccumulator += torque;
    return true;
}

bool PhysicsWorld::setContinuousForce(
    Transform& transform, const glm::vec3& force)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(force))
        return false;

    body->continuousForce = force;
    return true;
}

bool PhysicsWorld::setContinuousTorque(
    Transform& transform, const glm::vec3& torque)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(torque))
        return false;
    body->synchronizeColliderAndInertia();
    if (body->inverseMass <= 0.0f)
        return false;

    body->continuousTorque = torque;
    return true;
}

bool PhysicsWorld::setLinearDamping(Transform& transform, float damping)
{
    Body* body = findBody(transform);
    if (!body || !std::isfinite(damping) || damping < 0.0f)
        return false;
    body->linearDamping = std::min(damping, 1000.0f);
    return true;
}

bool PhysicsWorld::setAngularDamping(Transform& transform, float damping)
{
    Body* body = findBody(transform);
    if (!body || !std::isfinite(damping) || damping < 0.0f)
        return false;
    body->angularDamping = std::min(damping, 1000.0f);
    return true;
}

bool PhysicsWorld::applyImpulse(Transform& transform, const glm::vec3& impulse)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(impulse))
        return false;

    body->velocity += impulse * body->inverseMass;
    return true;
}

bool PhysicsWorld::applyAngularImpulse(
    Transform& transform, const glm::vec3& angularImpulse)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f || !isFiniteVector(angularImpulse))
        return false;
    body->synchronizeColliderAndInertia();

    body->angularVelocity += body->inverseInertiaTensorWorld() * angularImpulse;
    return true;
}

bool PhysicsWorld::applyImpulseAtPoint(
    Transform& transform,
    const glm::vec3& impulse,
    const glm::vec3& worldPoint)
{
    Body* body = findBody(transform);
    if (!body || body->inverseMass <= 0.0f ||
        !isFiniteVector(impulse) || !isFiniteVector(worldPoint))
        return false;
    body->synchronizeColliderAndInertia();

    const glm::vec3 arm = worldPoint - transform.position();
    body->velocity += impulse * body->inverseMass;
    body->angularVelocity += body->inverseInertiaTensorWorld() * glm::cross(arm, impulse);
    return true;
}

void PhysicsWorld::update(float frameTime)
{
    if (!(frameTime > 0.0f) || !std::isfinite(frameTime))
        return;

    if (profilingEnabled_)
    {
        if (profilingWindowStart_ == std::chrono::steady_clock::time_point{})
            profilingWindowStart_ = std::chrono::steady_clock::now();
        ++profilingWindow_.updateCalls;
        profilingWindow_.inputSimulationSeconds += frameTime;
    }

    accumulator_ += frameTime * timeScale_;

    int steps = 0;
    while (accumulator_ + kTolerance >= kFixedStep && steps < kMaxStepsPerFrame)
    {
        step(static_cast<float>(kFixedStep));
        accumulator_ -= kFixedStep;
        ++steps;
    }

    // Mantém um limite de trabalho por quadro. O tempo excedente fica limitado
    // a menos de um passo para evitar uma espiral de atualização atrasada.
    if (steps == kMaxStepsPerFrame && accumulator_ >= kFixedStep)
    {
        if (profilingEnabled_)
            ++profilingWindow_.droppedTimeEvents;
        accumulator_ = std::fmod(accumulator_, kFixedStep);
    }

    if (profilingEnabled_)
        profilingWindow_.maxStepsInUpdate = std::max<std::uint64_t>(
            profilingWindow_.maxStepsInUpdate, static_cast<std::uint64_t>(steps));

    // Forças pontuais acumuladas são consumidas quando ao menos um passo fixo
    // foi processado. Forças contínuas permanecem ativas até serem substituídas.
    if (steps > 0)
    {
        for (Body& body : bodies_)
        {
            body.forceAccumulator = glm::vec3(0.0f);
            body.torqueAccumulator = glm::vec3(0.0f);
        }
    }

    emitProfilingReportIfDue();
}

void PhysicsWorld::setProfilingEnabled(bool enabled)
{
    profilingEnabled_ = enabled;
    profilingWindow_ = ProfilingWindow{};
    profilingWindowStart_ = enabled
        ? std::chrono::steady_clock::now()
        : std::chrono::steady_clock::time_point{};
    meshCollisionScratch_.collectStats = enabled;
    meshCollisionScratch_.counters = collision::MeshCollisionCounters{};
    if (enabled)
    {
        // O launcher inicia os aplicativos sem console no Windows; por isso,
        // mantemos também um arquivo de diagnóstico no diretório de trabalho.
        std::ofstream log("physics_profile.log", std::ios::out | std::ios::trunc);
    }
}

bool PhysicsWorld::profilingEnabled() const
{
    return profilingEnabled_;
}

void PhysicsWorld::emitProfilingReportIfDue()
{
    if (!profilingEnabled_ ||
        profilingWindowStart_ == std::chrono::steady_clock::time_point{})
        return;

    const auto now = std::chrono::steady_clock::now();
    const double wallSeconds = std::chrono::duration<double>(
        now - profilingWindowStart_).count();
    if (wallSeconds < 1.0)
        return;

    const ProfilingWindow& p = profilingWindow_;
    const std::uint64_t fixedSteps = p.fixedSteps;
    const std::uint64_t substeps = p.substeps;
    const double stepDivisor = fixedSteps > 0 ? static_cast<double>(fixedSteps) : 1.0;

    std::size_t dynamicBodies = 0;
    for (const Body& body : bodies_)
    {
        if (body.transform && body.inverseMass > 0.0f)
            ++dynamicBodies;
    }

    const auto& mesh = meshCollisionScratch_.counters;
    std::ostringstream report;
    report << std::fixed << std::setprecision(3)
              << "\n[PHYSICS PROFILE] janela=" << wallSeconds << "s"
              << " | corpos=" << bodies_.size() << " (dinamicos=" << dynamicBodies << ")"
              << " | updates=" << p.updateCalls
              << " | delta_sim=" << p.inputSimulationSeconds << "s\n"
              << "  passos=" << fixedSteps << " (" << fixedSteps / wallSeconds << "/s)"
              << " | subpassos=" << substeps
              << " (media=" << substeps / stepDivisor
              << ", max=" << p.maxSubstepsInFixedStep << ")"
              << " | passos/update_max=" << p.maxStepsInUpdate
              << " | descarte_tempo=" << p.droppedTimeEvents << "\n"
              << "  passo_ms medio/max=" << p.stepMilliseconds / stepDivisor
              << "/" << p.maxStepMilliseconds
              << " | uso_1_core=" << (p.stepMilliseconds / (wallSeconds * 10.0)) << "%\n"
              << "  fases_ms/passo: setup=" << p.setupMilliseconds / stepDivisor
              << " integracao=" << p.integrationMilliseconds / stepDivisor
              << " contatos=" << p.initialContactMilliseconds / stepDivisor
              << " preparo=" << p.contactPreparationMilliseconds / stepDivisor
              << " solver=" << p.velocitySolveMilliseconds / stepDivisor
              << " cache_contatos_pos=" << p.positionContactMilliseconds / stepDivisor
              << " correcao_pos=" << p.positionCorrectionMilliseconds / stepDivisor << "\n"
              << "  contatos: coletas=" << p.contactCollectionCalls
              << " manifolds_iniciais=" << p.initialManifolds
              << " pontos_iniciais=" << p.initialContactPoints
              << " manifolds_pos_reutilizados=" << p.positionManifolds
              << " pontos_pos=" << p.positionContactPoints
              << " novas_coletas_pos=" << p.positionCollectionCalls
              << " passes_pos=" << p.positionPasses
              << " max_manifolds/coleta=" << p.maxContactManifolds << "\n"
              << "  solver: iteracoes_vel=" << p.velocityIterations
              << " resolucoes_contato=" << p.velocityContactSolves << "\n"
              << "  broadphase: pares_possiveis=" << p.bodyPairPossible
              << " pares_candidatos=" << p.bodyPairTests
              << " pares_descartados_AABB=" << p.bodyPairAabbRejected
              << " pares_colidindo=" << p.bodyPairHits
              << " testes_caixa_estatica=" << p.staticBoxTests
              << " caixas_descartadas_AABB=" << p.staticBoxAabbRejected
              << " testes_chao=" << p.groundBodyTests
              << " corpos_descartados_chao=" << p.groundAabbRejected << "\n"
              << "  malhas: consultas=" << mesh.contactQueries
              << " suporte=" << mesh.supportQueries
              << " nos_BVH=" << mesh.bvhNodesVisited
              << " nos_podados=" << mesh.bvhNodesRejected
              << " nos_suporte=" << mesh.supportBvhNodesVisited
              << " nos_suporte_podados=" << mesh.supportBvhNodesRejected
              << " triangulos_candidatos=" << mesh.candidateTriangles
              << " triangulos_testados=" << mesh.trianglesTested
              << " vertices_testados=" << mesh.verticesTested
              << " vertices_suporte=" << mesh.supportVerticesTested
              << '\n';
    const std::string reportText = report.str();
    std::cout << reportText << std::flush;
    {
        std::ofstream log("physics_profile.log", std::ios::out | std::ios::app);
        if (log)
            log << reportText;
    }

    profilingWindow_ = ProfilingWindow{};
    meshCollisionScratch_.counters = collision::MeshCollisionCounters{};
    profilingWindowStart_ = now;
}

void PhysicsWorld::step(float dt)
{
    using Clock = std::chrono::steady_clock;
    const auto stepStart = profilingEnabled_ ? Clock::now() : Clock::time_point{};
    auto phaseStart = profilingEnabled_ ? Clock::now() : Clock::time_point{};

    // Escolhe subpassos somente para movimentos que podem atingir outra forma.
    // Um objeto pequeno e parado, longe de qualquer trajetória, não força toda
    // a cena a usar sua dimensão como referência.
    bodyMotionScratch_.resize(bodies_.size());
    for (std::size_t index = 0; index < bodies_.size(); ++index)
    {
        Body& body = bodies_[index];
        body.synchronizeColliderAndInertia();
        BodyMotionProxy& motion = bodyMotionScratch_[index];
        motion = BodyMotionProxy{};
        if (!body.transform)
            continue;

        const AxisAlignedBounds currentBounds = makeBodyBounds(body);
        motion.sweptBounds = currentBounds;
        motion.active = true;
        const glm::vec3 halfExtents = body.collider.shape == BodyShape::Sphere
            ? glm::vec3(body.collider.sphereRadius)
            : body.collider.boxHalfExtents;
        motion.minimumHalfExtent = std::min(
            halfExtents.x, std::min(halfExtents.y, halfExtents.z));
        if (motion.minimumHalfExtent <= 1e-6f)
        {
            // Malhas abertas podem ter espessura geométrica zero (por exemplo,
            // uma placa). Usa uma fração da menor dimensão não nula para não
            // desativar a subdivisão adaptativa nesses corpos.
            float smallestPositiveExtent = std::numeric_limits<float>::max();
            for (int axis = 0; axis < 3; ++axis)
            {
                if (halfExtents[axis] > 1e-6f)
                    smallestPositiveExtent = std::min(
                        smallestPositiveExtent, halfExtents[axis]);
            }
            motion.minimumHalfExtent = smallestPositiveExtent <
                std::numeric_limits<float>::max()
                    ? smallestPositiveExtent * 0.1f : 1e-3f;
        }
        if (body.inverseMass <= 0.0f || motion.minimumHalfExtent <= 1e-6f)
            continue;

        const glm::vec3 acceleration = gravity_ +
            (body.forceAccumulator + body.continuousForce) * body.inverseMass;
        motion.linearDisplacement = body.velocity * dt +
            0.5f * acceleration * dt * dt;

        const glm::vec3 angularAcceleration = body.inverseInertiaTensorWorld() *
            (body.torqueAccumulator + body.continuousTorque);
        const float angularTravel = body.collider.shape == BodyShape::Sphere
            ? 0.0f
            : glm::length(body.collider.boxHalfExtents) *
                (glm::length(body.angularVelocity) * dt +
                 0.5f * glm::length(angularAcceleration) * dt * dt);
        motion.angularTravel = angularTravel;
        motion.totalTravel = glm::length(motion.linearDisplacement) + angularTravel;

        const glm::vec3 displacement = motion.linearDisplacement;
        const glm::vec3 angularExpansion(angularTravel);
        motion.sweptBounds.minimum = glm::min(
            currentBounds.minimum, currentBounds.minimum + displacement) - angularExpansion;
        motion.sweptBounds.maximum = glm::max(
            currentBounds.maximum, currentBounds.maximum + displacement) + angularExpansion;
    }

    int substeps = 1;
    const auto includeRequiredSubsteps = [&substeps](float relativeTravel, float featureSize)
    {
        if (!std::isfinite(relativeTravel) || !std::isfinite(featureSize) ||
            relativeTravel <= 0.0f || featureSize <= 1e-6f)
            return;
        const float required = std::ceil(relativeTravel / (featureSize * 0.5f));
        if (!std::isfinite(required) || required >=
            static_cast<float>(kMaxAdaptiveSubsteps))
        {
            substeps = kMaxAdaptiveSubsteps;
            return;
        }
        substeps = std::max(substeps, std::max(1, static_cast<int>(required)));
    };

    // Pares dinâmicos/estáticos registrados como bodies. Somente trajetórias
    // cujos AABBs varridos se cruzam influenciam a subdivisão temporal.
    for (std::size_t i = 0; i < bodies_.size(); ++i)
    {
        const BodyMotionProxy& a = bodyMotionScratch_[i];
        if (!a.active)
            continue;
        for (std::size_t j = i + 1; j < bodies_.size(); ++j)
        {
            const BodyMotionProxy& b = bodyMotionScratch_[j];
            if (!b.active || bodies_[j].inverseMass + bodies_[i].inverseMass <= 0.0f ||
                !overlaps(a.sweptBounds, b.sweptBounds))
                continue;

            const float bFeature = b.minimumHalfExtent > 1e-6f
                ? b.minimumHalfExtent : a.minimumHalfExtent;
            const float featureSize = std::min(a.minimumHalfExtent, bFeature);
            const float relativeTravel = glm::length(
                a.linearDisplacement - b.linearDisplacement) +
                a.angularTravel + b.angularTravel;
            includeRequiredSubsteps(relativeTravel, featureSize);
        }
    }

    // Colisores estáticos separados dos bodies (mesa, plataformas etc.).
    for (std::size_t bodyIndex = 0; bodyIndex < bodies_.size(); ++bodyIndex)
    {
        const Body& body = bodies_[bodyIndex];
        const BodyMotionProxy& motion = bodyMotionScratch_[bodyIndex];
        if (!motion.active || body.inverseMass <= 0.0f || motion.totalTravel <= 0.0f)
            continue;
        for (const StaticBoxCollider& collider : staticBoxes_)
        {
            if (!collider.transform)
                continue;
            const glm::vec3 scale = glm::abs(collider.transform->scale());
            const float staticFeature = std::min(
                scale.x * collider.localHalfExtents.x,
                std::min(scale.y * collider.localHalfExtents.y,
                         scale.z * collider.localHalfExtents.z));
            if (staticFeature <= 1e-6f ||
                !overlaps(motion.sweptBounds, makeStaticBoxBounds(collider)))
                continue;
            includeRequiredSubsteps(
                motion.totalTravel, std::min(motion.minimumHalfExtent, staticFeature));
        }

        if (hasGroundPlane_ && motion.sweptBounds.minimum.y <= groundHeight_ + 1e-4f)
        {
            // Em um plano infinito, deslocamento horizontal não aumenta o risco
            // de tunneling contra o chão; só contam movimento normal e rotação.
            const float groundNormalTravel =
                std::fabs(motion.linearDisplacement.y) + motion.angularTravel;
            includeRequiredSubsteps(groundNormalTravel, motion.minimumHalfExtent);
        }
        else if (hasGroundBowl_)
        {
            AxisAlignedBounds bowlBounds;
            bowlBounds.minimum = glm::vec3(
                -groundBowlRadius_, -1e-3f, -groundBowlRadius_);
            bowlBounds.maximum = glm::vec3(
                groundBowlRadius_, groundBowlDepth_ + groundBowlLipHeight_ + 1e-3f,
                groundBowlRadius_);
            if (overlaps(motion.sweptBounds, bowlBounds))
                includeRequiredSubsteps(motion.totalTravel, motion.minimumHalfExtent);
        }
    }

    if (profilingEnabled_)
    {
        profilingWindow_.setupMilliseconds +=
            std::chrono::duration<double, std::milli>(Clock::now() - phaseStart).count();
        ++profilingWindow_.fixedSteps;
        profilingWindow_.substeps += static_cast<std::uint64_t>(substeps);
        profilingWindow_.maxSubstepsInFixedStep = std::max<std::uint64_t>(
            profilingWindow_.maxSubstepsInFixedStep,
            static_cast<std::uint64_t>(substeps));
    }

    const float substep = dt / static_cast<float>(substeps);
    for (int substepIndex = 0; substepIndex < substeps; ++substepIndex)
    {
        phaseStart = profilingEnabled_ ? Clock::now() : Clock::time_point{};
        for (Body& body : bodies_)
        {
            if (!body.transform || body.inverseMass == 0.0f)
                continue;

            const glm::vec3 acceleration = gravity_ +
                (body.forceAccumulator + body.continuousForce) * body.inverseMass;
            const glm::vec3 angularAcceleration = body.inverseInertiaTensorWorld() *
                (body.torqueAccumulator + body.continuousTorque);

            body.velocity += acceleration * substep;
            body.angularVelocity += angularAcceleration * substep;
            body.velocity *= std::exp(-body.linearDamping * substep);
            body.angularVelocity *= std::exp(-body.angularDamping * substep);
            body.transform->translate(body.velocity * substep);

            const float angularSpeed = glm::length(body.angularVelocity);
            if (angularSpeed > 1e-6f)
            {
                const glm::quat delta = glm::angleAxis(
                    angularSpeed * substep, body.angularVelocity / angularSpeed);
                body.transform->setOrientation(glm::normalize(
                    delta * body.transform->orientation()));
            }
        }
        if (profilingEnabled_)
        {
            profilingWindow_.integrationMilliseconds +=
                std::chrono::duration<double, std::milli>(Clock::now() - phaseStart).count();
        }

        // Detecta uma vez por subpasso e mantém o mesmo conjunto de contatos
        // durante as iterações do solver. Isso permite acumular impulsos por
        // ponto sem recalcular a geometria a cada iteração.
        std::vector<ContactConstraint>& contacts = contactsScratch_;
        contacts.reserve(std::max(contacts.capacity(), bodies_.size() * 4));
        phaseStart = profilingEnabled_ ? Clock::now() : Clock::time_point{};
        if (profilingEnabled_)
        {
            profileCollectionActive_ = true;
            meshCollisionScratch_.collectStats = true;
        }
        collectContactConstraints(contacts);
        if (profilingEnabled_)
        {
            profileCollectionActive_ = false;
            meshCollisionScratch_.collectStats = false;
            profilingWindow_.initialContactMilliseconds +=
                std::chrono::duration<double, std::milli>(Clock::now() - phaseStart).count();
            ++profilingWindow_.contactCollectionCalls;
            profilingWindow_.initialManifolds += contacts.size();
            profilingWindow_.maxContactManifolds = std::max<std::uint64_t>(
                profilingWindow_.maxContactManifolds, contacts.size());
            for (const ContactConstraint& contact : contacts)
                profilingWindow_.initialContactPoints += contact.manifold.pointCount;
        }

        // Contatos contra suportes estáticos são resolvidos primeiro. Em pilhas,
        // isso ancora os corpos inferiores antes de propagar as cargas para os
        // corpos dinâmicos apoiados neles nas iterações seguintes.
        phaseStart = profilingEnabled_ ? Clock::now() : Clock::time_point{};
        std::stable_partition(
            contacts.begin(), contacts.end(),
            [](const ContactConstraint& contact)
            {
                return !contact.bodyA || !contact.bodyB ||
                    contact.bodyA->inverseMass <= 0.0f ||
                    contact.bodyB->inverseMass <= 0.0f;
            });
        for (ContactConstraint& contact : contacts)
        {
            solver::prepareContactManifold(
                contact.bodyA,
                contact.bodyB,
                contact.manifold,
                contact.material);
            solver::prepareContactSolverCache(
                contact.bodyA, contact.bodyB,
                contact.manifold, contact.solverCache);
        }
        if (profilingEnabled_)
        {
            profilingWindow_.contactPreparationMilliseconds +=
                std::chrono::duration<double, std::milli>(Clock::now() - phaseStart).count();
        }

        phaseStart = profilingEnabled_ ? Clock::now() : Clock::time_point{};
        int velocityIterationsUsed = 0;
        if (contacts.empty())
            velocityIterationsUsed = 0;
        else
        for (int iteration = 0; iteration < kCollisionIterations; ++iteration)
        {
            float maximumImpulseChange = 0.0f;
            bool singlePointOnly = true;
            for (ContactConstraint& contact : contacts)
            {
                if (contact.manifold.pointCount != 1)
                    singlePointOnly = false;
                maximumImpulseChange = std::max(
                    maximumImpulseChange,
                    solver::solveContactVelocity(
                    contact.bodyA,
                    contact.bodyB,
                    contact.manifold,
                    contact.material,
                    contact.solverCache));
            }
            ++velocityIterationsUsed;
            if (singlePointOnly && iteration >= 7 &&
                maximumImpulseChange < 1e-5f)
                break;
        }
        if (profilingEnabled_)
        {
            profilingWindow_.velocitySolveMilliseconds +=
                std::chrono::duration<double, std::milli>(Clock::now() - phaseStart).count();
            profilingWindow_.velocityIterations += velocityIterationsUsed;
            profilingWindow_.velocityContactSolves +=
                static_cast<std::uint64_t>(contacts.size()) *
                static_cast<std::uint64_t>(velocityIterationsUsed);
        }

        // Projeção posicional separada dos impulsos. Reutilizamos os manifolds
        // coletados no início do subpasso: a correção é translacional, então a
        // profundidade pode ser atualizada exatamente pela mudança relativa de
        // posição ao longo da normal, sem reconstruir a geometria das malhas.
        if (!contacts.empty())
        {
            for (ContactConstraint& contact : contacts)
            {
                contact.lastPositionA = contact.bodyA && contact.bodyA->transform
                    ? contact.bodyA->transform->position() : glm::vec3(0.0f);
                contact.lastPositionB = contact.bodyB && contact.bodyB->transform
                    ? contact.bodyB->transform->position() : glm::vec3(0.0f);
            }

            for (int iteration = 0; iteration < kPositionIterations; ++iteration)
            {
                phaseStart = profilingEnabled_ ? Clock::now() : Clock::time_point{};
                float deepestPenetration = 0.0f;
                for (ContactConstraint& contact : contacts)
                {
                    const glm::vec3 currentPositionA =
                        contact.bodyA && contact.bodyA->transform
                            ? contact.bodyA->transform->position() : glm::vec3(0.0f);
                    const glm::vec3 currentPositionB =
                        contact.bodyB && contact.bodyB->transform
                            ? contact.bodyB->transform->position() : glm::vec3(0.0f);
                    const glm::vec3 relativeCorrection =
                        (currentPositionB - contact.lastPositionB) -
                        (currentPositionA - contact.lastPositionA);

                    // Os manifolds preservam suas normais e pontos durante a
                    // projeção. Como esta etapa não gira os corpos, a mudança
                    // de penetração pela translação é a projeção exata abaixo.
                    float contactDeepestPenetration = 0.0f;
                    for (std::size_t pointIndex = 0;
                         pointIndex < contact.manifold.pointCount; ++pointIndex)
                    {
                        ContactPoint& point = contact.manifold.points[pointIndex];
                        point.penetration = std::max(
                            0.0f, point.penetration -
                                glm::dot(relativeCorrection, point.normal));
                        contactDeepestPenetration = std::max(
                            contactDeepestPenetration, point.penetration);
                    }
                    deepestPenetration = std::max(
                        deepestPenetration, contactDeepestPenetration);

                    contact.lastPositionA = currentPositionA;
                    contact.lastPositionB = currentPositionB;
                    if (contactDeepestPenetration > 0.0002f)
                    {
                        const auto correctionStart = profilingEnabled_
                            ? Clock::now() : Clock::time_point{};
                        solver::correctContactPosition(
                            contact.bodyA, contact.bodyB, contact.manifold, 1.0f);
                        if (profilingEnabled_)
                        {
                            profilingWindow_.positionCorrectionMilliseconds +=
                                std::chrono::duration<double, std::milli>(
                                    Clock::now() - correctionStart).count();
                        }
                    }
                }

                if (profilingEnabled_)
                {
                    profilingWindow_.positionContactMilliseconds +=
                        std::chrono::duration<double, std::milli>(
                            Clock::now() - phaseStart).count();
                    ++profilingWindow_.positionPasses;
                    profilingWindow_.positionManifolds += contacts.size();
                    for (const ContactConstraint& contact : contacts)
                        profilingWindow_.positionContactPoints +=
                            contact.manifold.pointCount;
                }

                if (deepestPenetration <= 0.0002f)
                    break;
            }
        }

    }

    ++stepCount_;
    if (profilingEnabled_)
    {
        const double elapsed = std::chrono::duration<double, std::milli>(
            Clock::now() - stepStart).count();
        profilingWindow_.stepMilliseconds += elapsed;
        profilingWindow_.maxStepMilliseconds = std::max(
            profilingWindow_.maxStepMilliseconds, elapsed);
    }
}

void PhysicsWorld::reset()
{
    for (Body& body : bodies_)
    {
        if (!body.transform)
            continue;

        body.transform->move(body.startPosition);
        body.transform->setOrientation(body.startOrientation);
        body.velocity = body.startVelocity;
        body.angularVelocity = body.startAngularVelocity;
        body.forceAccumulator = glm::vec3(0.0f);
        body.torqueAccumulator = glm::vec3(0.0f);
        body.continuousForce = glm::vec3(0.0f);
        body.continuousTorque = glm::vec3(0.0f);
        body.synchronizeColliderAndInertia();
    }

    accumulator_ = 0.0;
}

void PhysicsWorld::captureInitialPose()
{
    for (Body& body : bodies_)
    {
        if (!body.transform)
            continue;

        body.synchronizeColliderAndInertia();
        body.startPosition = body.transform->position();
        body.startOrientation = body.transform->orientation();
    }
}

void PhysicsWorld::clear()
{
    bodies_.clear();
    staticBoxes_.clear();
    contactsScratch_.clear();
    bodyBoundsByIndexScratch_.clear();
    bodySweepScratch_.clear();
    staticBoxBoundsScratch_.clear();
    bodyPairScratch_.clear();
    bodyMotionScratch_.clear();
    meshCollisionScratch_ = collision::MeshCollisionScratch{};
    clearGroundPlane();
    accumulator_ = 0.0;
}

std::size_t PhysicsWorld::bodyCount() const
{
    return bodies_.size();
}

unsigned long long PhysicsWorld::stepCount() const
{
    return stepCount_;
}

bool PhysicsWorld::hasDynamicBody(const Transform& transform) const
{
    const Body* body = findBody(transform);
    return body && body->inverseMass > 0.0f;
}

glm::vec3 PhysicsWorld::linearVelocity(const Transform& transform) const
{
    const Body* body = findBody(transform);
    return body ? body->velocity : glm::vec3(0.0f);
}

glm::vec3 PhysicsWorld::angularVelocity(const Transform& transform) const
{
    const Body* body = findBody(transform);
    return body ? body->angularVelocity : glm::vec3(0.0f);
}

} // namespace rotor::physics
