#include "physics_world.h"

#include "engine/physics/collision/collision_geometry.h"
#include "engine/physics/solver/contact_solver.h"

#include <algorithm>
#include <cmath>

namespace rotor::physics
{
using collision::OrientedBox;

namespace
{
void setSinglePointManifold(
    ContactManifold& manifold,
    const ContactPoint& point)
{
    manifold.clear();
    manifold.points[0] = point;
    manifold.pointCount = 1;
}

OrientedBox makeBodyBoundsBox(const RigidBody& body)
{
    if (!body.transform)
        return OrientedBox{};

    const glm::quat orientation = body.transform->orientation();
    glm::vec3 center = body.transform->position();
    if (body.collider.shape == ColliderShape::TriangleMesh)
    {
        center += orientation * body.collider.boxCenter;
    }

    return collision::makeOrientedBox(
        center, orientation, body.collider.boxHalfExtents);
}

PhysicsMaterial groundMaterialFor(
    const RigidBody& body,
    float staticFriction,
    float dynamicFriction)
{
    PhysicsMaterial material;
    material.restitution = body.material.restitution;
    material.staticFriction = staticFriction;
    material.dynamicFriction = dynamicFriction;
    return rotor::physics::solver::combineMaterials(body.material, material);
}
} // namespace

void PhysicsWorld::collectContactConstraints(
    std::vector<ContactConstraint>& contacts)
{
    contacts.clear();
    meshCollisionScratch_.collectStats = profilingEnabled_ && profileCollectionActive_;

    // Broadphase sweep-and-prune: ordena os corpos pelo limite mínimo em X e
    // só envia ao narrowphase pares cujas AABBs se sobrepõem nos três eixos.
    bodyBoundsByIndexScratch_.resize(bodies_.size());
    bodySweepScratch_.clear();
    bodySweepScratch_.reserve(std::max(bodySweepScratch_.capacity(), bodies_.size()));
    std::size_t staticBodyCount = 0;
    for (std::size_t index = 0; index < bodies_.size(); ++index)
    {
        const Body& body = bodies_[index];
        if (!body.transform)
        {
            bodyBoundsByIndexScratch_[index] = AxisAlignedBounds{};
            continue;
        }
        bodyBoundsByIndexScratch_[index] = makeBodyBounds(body);
        bodySweepScratch_.push_back(IndexedBounds{
            index, bodyBoundsByIndexScratch_[index]});
        if (body.inverseMass <= 0.0f)
            ++staticBodyCount;
    }
    std::sort(bodySweepScratch_.begin(), bodySweepScratch_.end(),
        [](const IndexedBounds& a, const IndexedBounds& b)
        {
            if (a.bounds.minimum.x != b.bounds.minimum.x)
                return a.bounds.minimum.x < b.bounds.minimum.x;
            return a.index < b.index;
        });

    bodyPairScratch_.clear();
    for (std::size_t i = 0; i < bodySweepScratch_.size(); ++i)
    {
        const IndexedBounds& a = bodySweepScratch_[i];
        for (std::size_t j = i + 1; j < bodySweepScratch_.size(); ++j)
        {
            const IndexedBounds& b = bodySweepScratch_[j];
            if (b.bounds.minimum.x > a.bounds.maximum.x)
                break;
            if (a.bounds.maximum.y < b.bounds.minimum.y ||
                a.bounds.minimum.y > b.bounds.maximum.y ||
                a.bounds.maximum.z < b.bounds.minimum.z ||
                a.bounds.minimum.z > b.bounds.maximum.z)
                continue;

            const Body& bodyA = bodies_[a.index];
            const Body& bodyB = bodies_[b.index];
            if (bodyA.inverseMass + bodyB.inverseMass <= 0.0f)
                continue;
            bodyPairScratch_.push_back(BodyPair{
                std::min(a.index, b.index), std::max(a.index, b.index)});
        }
    }
    // Mantém a ordem estável de resolução usada pelo antigo loop i/j.
    std::sort(bodyPairScratch_.begin(), bodyPairScratch_.end(),
        [](const BodyPair& a, const BodyPair& b)
        {
            return a.a != b.a ? a.a < b.a : a.b < b.b;
        });

    staticBoxBoundsScratch_.clear();
    staticBoxBoundsScratch_.reserve(std::max(
        staticBoxBoundsScratch_.capacity(), staticBoxes_.size()));
    for (std::size_t index = 0; index < staticBoxes_.size(); ++index)
    {
        const StaticBoxCollider& collider = staticBoxes_[index];
        if (!collider.transform)
            continue;
        const glm::vec3 scale = glm::abs(collider.transform->scale());
        if (scale.x <= 1e-6f || scale.y <= 1e-6f || scale.z <= 1e-6f)
            continue;
        staticBoxBoundsScratch_.push_back(IndexedBounds{
            index, makeStaticBoxBounds(collider)});
    }

    if (profilingEnabled_ && profileCollectionActive_)
    {
        const std::uint64_t bodyCount = static_cast<std::uint64_t>(bodySweepScratch_.size());
        const std::uint64_t allPairs = bodyCount * (bodyCount > 0 ? bodyCount - 1 : 0) / 2;
        const std::uint64_t staticCount = static_cast<std::uint64_t>(staticBodyCount);
        const std::uint64_t staticPairs = staticCount *
            (staticCount > 0 ? staticCount - 1 : 0) / 2;
        const std::uint64_t possible = allPairs - std::min(allPairs, staticPairs);
        profilingWindow_.bodyPairPossible += possible;
        profilingWindow_.bodyPairAabbRejected += possible > bodyPairScratch_.size()
            ? possible - bodyPairScratch_.size() : 0;
    }

    // 1. Contatos entre pares selecionados pelo broadphase.
    for (const BodyPair& pair : bodyPairScratch_)
    {
        Body& bodyA = bodies_[pair.a];
        Body& bodyB = bodies_[pair.b];
        if (profilingEnabled_ && profileCollectionActive_)
            ++profilingWindow_.bodyPairTests;
        if (!bodyA.transform || !bodyB.transform)
            continue;

        ContactManifold manifold;
        bool collided = false;

        if (bodyA.collider.shape == BodyShape::Sphere &&
            bodyB.collider.shape == BodyShape::Sphere)
        {
            ContactPoint point;
            collided = collision::sphereSphereContact(
                bodyA.transform->position(), bodyA.collider.sphereRadius,
                bodyB.transform->position(), bodyB.collider.sphereRadius,
                point);
            if (collided)
                setSinglePointManifold(manifold, point);
        }
        else if (bodyA.collider.shape != BodyShape::Sphere &&
                 bodyB.collider.shape != BodyShape::Sphere)
        {
            // Meshes continuam usando OBB nos pares dinâmicos; o broadphase
            // agora evita até chegar a esta consulta quando os limites separam.
            const OrientedBox boxA = makeBodyBoundsBox(bodyA);
            const OrientedBox boxB = makeBodyBoundsBox(bodyB);
            collided = collision::intersectOrientedBoxes(boxA, boxB, manifold);
        }
        else
        {
            const Body* sphereBody = bodyA.collider.shape == BodyShape::Sphere
                ? &bodyA : &bodyB;
            const Body* otherBody = sphereBody == &bodyA ? &bodyB : &bodyA;
            const OrientedBox box = makeBodyBoundsBox(*otherBody);
            ContactPoint point;
            collided = collision::sphereOrientedBoxContact(
                sphereBody->transform->position(),
                sphereBody->collider.sphereRadius, box, point);
            if (collided)
            {
                // A consulta esfera-caixa produz normal caixa -> esfera.
                // O solver de pares espera normal de A para B.
                if (sphereBody == &bodyA)
                {
                    point.normal = -point.normal;
                    std::swap(point.pointA, point.pointB);
                }
                setSinglePointManifold(manifold, point);
            }
        }

        if (!collided || manifold.empty())
            continue;

        contacts.push_back(ContactConstraint{
            &bodyA,
            &bodyB,
            manifold,
            solver::combineMaterials(bodyA.material, bodyB.material),
            solver::ContactSolverCache{}});
        if (profilingEnabled_ && profileCollectionActive_)
            ++profilingWindow_.bodyPairHits;
    }

    // 2. Contatos contra caixas estáticas, como o tampo da mesa.
    for (std::size_t bodyIndex = 0; bodyIndex < bodies_.size(); ++bodyIndex)
    {
        Body& body = bodies_[bodyIndex];
        if (!body.transform || body.inverseMass <= 0.0f)
            continue;

        for (const IndexedBounds& staticEntry : staticBoxBoundsScratch_)
        {
            if (profilingEnabled_ && profileCollectionActive_)
                ++profilingWindow_.staticBoxTests;
            if (!overlaps(bodyBoundsByIndexScratch_[bodyIndex], staticEntry.bounds))
            {
                if (profilingEnabled_ && profileCollectionActive_)
                    ++profilingWindow_.staticBoxAabbRejected;
                continue;
            }
            const StaticBoxCollider& collider = staticBoxes_[staticEntry.index];
            if (!collider.transform)
                continue;

            const glm::vec3 scale = collider.transform->scale();
            const glm::vec3 absoluteScale = glm::abs(scale);
            if (absoluteScale.x <= 1e-6f || absoluteScale.y <= 1e-6f ||
                absoluteScale.z <= 1e-6f)
                continue;

            const glm::quat orientation = collider.transform->orientation();
            const glm::vec3 boxCenter = collider.transform->position() +
                orientation * (scale * collider.localCenter);
            const OrientedBox staticBox = collision::makeOrientedBox(
                boxCenter, orientation, absoluteScale * collider.localHalfExtents);

            ContactManifold manifold;
            bool collided = false;
            if (body.collider.shape == BodyShape::Sphere)
            {
                ContactPoint point;
                collided = collision::sphereOrientedBoxContact(
                    body.transform->position(), body.collider.sphereRadius,
                    staticBox, point);
                if (collided)
                    setSinglePointManifold(manifold, point);
            }
            else if (body.collider.shape == BodyShape::TriangleMesh)
            {
                collided = collision::triangleMeshOrientedBoxContacts(
                    body, staticBox, manifold, &meshCollisionScratch_);
            }
            else
            {
                const OrientedBox dynamicBox = collision::makeOrientedBox(
                    body.transform->position(), body.transform->orientation(),
                    body.collider.boxHalfExtents);
                collided = collision::intersectOrientedBoxes(
                    staticBox, dynamicBox, manifold);
            }

            if (!collided || manifold.empty())
                continue;

            contacts.push_back(ContactConstraint{
                nullptr,
                &body,
                manifold,
                solver::combineMaterials(body.material, collider.material),
                solver::ContactSolverCache{}});
        }
    }

    // 3. Contatos com o chão plano ou a bacia.
    if (!hasGroundPlane_ && !hasGroundBowl_)
        return;

    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    for (std::size_t bodyIndex = 0; bodyIndex < bodies_.size(); ++bodyIndex)
    {
        Body& body = bodies_[bodyIndex];
        if (!body.transform || body.inverseMass <= 0.0f)
            continue;
        if (profilingEnabled_ && profileCollectionActive_)
            ++profilingWindow_.groundBodyTests;

        if (hasGroundPlane_ &&
            bodyBoundsByIndexScratch_[bodyIndex].minimum.y > groundHeight_ + 1e-4f)
        {
            if (profilingEnabled_ && profileCollectionActive_)
                ++profilingWindow_.groundAabbRejected;
            continue;
        }

        ContactManifold manifold;
        if (hasGroundPlane_ && body.collider.shape == BodyShape::Box)
        {
            const OrientedBox box = collision::makeOrientedBox(
                body.transform->position(), body.transform->orientation(),
                body.collider.boxHalfExtents);
            collision::boxPlaneContacts(
                box, worldUp, groundHeight_, manifold);
        }
        else if (hasGroundPlane_ && body.collider.shape == BodyShape::TriangleMesh)
        {
            collision::triangleMeshPlaneContacts(
                body, worldUp, groundHeight_, manifold, &meshCollisionScratch_);
        }
        else if (hasGroundPlane_)
        {
            const glm::vec3 position = body.transform->position();
            const float signedDistance = position.y - groundHeight_;
            const float radius = collision::supportRadius(body, worldUp);
            const float penetration = radius - signedDistance;
            if (penetration > 0.0f)
            {
                ContactPoint point;
                point.normal = worldUp;
                point.penetration = penetration;
                point.pointB = collision::supportPoint(body, -worldUp);
                const float bodyPointDistance = point.pointB.y - groundHeight_;
                point.pointA = point.pointB - worldUp * bodyPointDistance;
                point.point = (point.pointA + point.pointB) * 0.5f;
                point.hasSeparatePoints = true;
                setSinglePointManifold(manifold, point);
            }
        }
        else
        {
            const collision::BowlSurfaceSample surface =
                collision::findClosestBowlSurface(
                    body.transform->position(),
                    groundBowlRadius_,
                    groundBowlDepth_,
                    groundBowlLipHeight_);
            const float support = collision::supportRadius(
                body, surface.normal, &meshCollisionScratch_);
            const float penetration = support - surface.signedDistance;
            if (penetration > 0.0f)
            {
                ContactPoint point;
                point.normal = surface.normal;
                point.penetration = penetration;
                point.pointB = collision::supportPoint(body, -surface.normal);
                const float bodyPointDistance = glm::dot(
                    point.pointB - surface.surfacePoint, surface.normal);
                point.pointA = point.pointB - surface.normal * bodyPointDistance;
                point.point = (point.pointA + point.pointB) * 0.5f;
                point.hasSeparatePoints = true;
                setSinglePointManifold(manifold, point);
            }
        }

        if (manifold.empty())
            continue;

        contacts.push_back(ContactConstraint{
            nullptr,
            &body,
            manifold,
            groundMaterialFor(
                body, groundStaticFriction_, groundDynamicFriction_),
            solver::ContactSolverCache{}});
    }
}

void PhysicsWorld::stabilizeInitialOverlaps(int iterations)
{
    const int boundedIterations = std::clamp(iterations, 0, 128);
    std::vector<ContactConstraint>& contacts = contactsScratch_;
    contacts.reserve(std::max(contacts.capacity(), bodies_.size() * 4));

    // Apenas corrige posições. Não aplica impulsos nem cria velocidades iniciais.
    // O estado corrigido passa a ser a pose de reset capturada pela aplicação.
    for (int iteration = 0; iteration < boundedIterations; ++iteration)
    {
        collectContactConstraints(contacts);
        if (contacts.empty())
            break;

        // Corrige primeiro corpos apoiados em superfícies estáticas, antes de
        // propagar a correção aos pares dinâmicos empilhados acima deles.
        std::stable_partition(
            contacts.begin(), contacts.end(),
            [](const ContactConstraint& constraint)
            {
                return !constraint.bodyA || !constraint.bodyB ||
                    constraint.bodyA->inverseMass <= 0.0f ||
                    constraint.bodyB->inverseMass <= 0.0f;
            });

        float deepestPenetration = 0.0f;
        for (const ContactConstraint& constraint : contacts)
        {
            for (std::size_t i = 0; i < constraint.manifold.pointCount; ++i)
            {
                deepestPenetration = std::max(
                    deepestPenetration,
                    constraint.manifold.points[i].penetration);
            }

            solver::correctContactPosition(
                constraint.bodyA,
                constraint.bodyB,
                constraint.manifold,
                1.0f);
        }

        if (deepestPenetration <= 0.0002f)
            break;
    }
}

} // namespace rotor::physics
