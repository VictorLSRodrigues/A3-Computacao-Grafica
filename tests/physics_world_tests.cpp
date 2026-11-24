#include "engine/physics/physics_world.h"
#include "engine/graphics/geometry/shapes.h"

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
bool expect(bool condition, const char* message)
{
    if (!condition)
        std::cerr << "FAIL: " << message << '\n';
    return condition;
}
}

int main()
{
    bool passed = true;

    PhysicsWorld bouncingWorld;
    bouncingWorld.setGravity(glm::vec3(0.0f));
    bouncingWorld.setGroundPlane(0.0f);

    Transform bouncingSphere;
    bouncingSphere.move(glm::vec3(0.0f, 0.51f, 0.0f));
    bouncingWorld.addBody(
        bouncingSphere, glm::vec3(0.0f, -2.0f, 0.0f), 0.5f, 0.8f);
    bouncingWorld.update(1.0f / 120.0f);

    passed &= expect(
        std::fabs(bouncingSphere.position().y - 0.5f) < 1e-5f,
        "sphere is moved out of the ground after contact");

    bouncingWorld.update(1.0f / 120.0f);
    passed &= expect(
        bouncingSphere.position().y > 0.5f,
        "positive restitution makes the sphere bounce");

    bouncingWorld.reset();
    passed &= expect(
        std::fabs(bouncingSphere.position().y - 0.51f) < 1e-5f,
        "reset restores the sphere's starting position");

    PhysicsWorld restingWorld;
    restingWorld.setGroundPlane(0.0f);
    Transform restingSphere;
    restingSphere.move(glm::vec3(0.0f, 1.0f, 0.0f));
    restingWorld.addBody(restingSphere, glm::vec3(0.0f), 0.5f, 0.0f);

    for (int i = 0; i < 240; ++i)
        restingWorld.update(1.0f / 120.0f);

    passed &= expect(
        std::fabs(restingSphere.position().y - 0.5f) < 1e-5f,
        "zero restitution prevents penetration and leaves the sphere resting");

    PhysicsWorld pointWorld;
    pointWorld.setGravity(glm::vec3(0.0f));
    pointWorld.setGroundPlane(0.0f);
    Transform pointBody;
    pointBody.move(glm::vec3(0.0f, 0.01f, 0.0f));
    pointWorld.addBody(pointBody, glm::vec3(0.0f, -2.0f, 0.0f));
    pointWorld.update(1.0f / 120.0f);
    passed &= expect(
        pointBody.position().y < 0.01f,
        "bodies without a sphere radius keep their previous non-colliding behavior");

    PhysicsWorld massWorld;
    massWorld.setGravity(glm::vec3(0.0f));
    Transform lightSphere;
    Transform heavySphere;
    lightSphere.move(glm::vec3(-0.5f, 0.0f, 0.0f));
    heavySphere.move(glm::vec3(0.5f, 0.0f, 0.0f));
    passed &= expect(
        massWorld.addBody(
            lightSphere, glm::vec3(1.0f, 0.0f, 0.0f), 0.5f, 1.0f, 1.0f),
        "positive-mass dynamic body is accepted");
    passed &= expect(
        massWorld.addBody(
            heavySphere, glm::vec3(0.0f), 0.5f, 1.0f, 3.0f),
        "body with a different positive mass is accepted");
    massWorld.update(1.0f / 120.0f);
    massWorld.update(1.0f / 120.0f);
    passed &= expect(
        lightSphere.position().x < -0.5f && heavySphere.position().x > 0.5f,
        "elastic sphere collision transfers momentum according to body mass");

    PhysicsWorld staticWorld;
    Transform fixedSphere;
    Transform movingSphere;
    fixedSphere.move(glm::vec3(0.0f));
    movingSphere.move(glm::vec3(-0.9f, 0.0f, 0.0f));
    staticWorld.addStaticBody(fixedSphere, 0.5f, 1.0f);
    passed &= expect(
        staticWorld.addBody(
            movingSphere, glm::vec3(1.0f, 0.0f, 0.0f), 0.5f, 1.0f),
        "dynamic sphere is accepted beside a static sphere");
    staticWorld.update(1.0f / 120.0f);
    passed &= expect(
        glm::length(fixedSphere.position()) < 1e-5f,
        "static body is not moved by gravity or collision response");
    staticWorld.update(1.0f / 120.0f);
    passed &= expect(
        movingSphere.position().x < -1.0f,
        "dynamic sphere rebounds from a static sphere");

    PhysicsWorld sphereFrictionWorld;
    sphereFrictionWorld.setGravity(glm::vec3(0.0f));
    Transform sphereFrictionObstacle;
    Transform sphereFrictionBody;
    sphereFrictionBody.move(glm::vec3(-0.9f, -1.0f / 120.0f, 0.0f));
    sphereFrictionWorld.addStaticBody(
        sphereFrictionObstacle, 0.5f, 0.0f, 1.0f, 1.0f);
    sphereFrictionWorld.addBody(
        sphereFrictionBody,
        glm::vec3(1.0f, 1.0f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        1.0f,
        1.0f);
    sphereFrictionWorld.update(1.0f / 120.0f);
    sphereFrictionWorld.update(1.0f / 120.0f);
    const glm::vec3 pairNormal = glm::normalize(
        sphereFrictionBody.position() - sphereFrictionObstacle.position());
    const glm::vec3 pairContactArm = -pairNormal * 0.5f;
    const glm::vec3 pairContactVelocity =
        sphereFrictionWorld.linearVelocity(sphereFrictionBody) +
        glm::cross(
            sphereFrictionWorld.angularVelocity(sphereFrictionBody),
            pairContactArm);
    const glm::vec3 pairTangentialVelocity = pairContactVelocity -
        glm::dot(pairContactVelocity, pairNormal) * pairNormal;
    passed &= expect(
        glm::length(pairTangentialVelocity) < 1e-4f,
        "static friction removes tangential relative velocity at sphere contact");
    passed &= expect(
        glm::length(sphereFrictionWorld.angularVelocity(sphereFrictionBody)) > 0.1f,
        "tangential sphere contact generates angular velocity");

    PhysicsWorld slidingWorld;
    slidingWorld.setGroundPlane(0.0f, 0.8f, 0.6f);
    Transform slidingSphere;
    slidingSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    slidingWorld.addBody(
        slidingSphere,
        glm::vec3(3.0f, 0.0f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        0.8f,
        0.6f);

    PhysicsWorld frictionlessWorld;
    frictionlessWorld.setGroundPlane(0.0f, 0.0f, 0.0f);
    Transform frictionlessSphere;
    frictionlessSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    frictionlessWorld.addBody(
        frictionlessSphere,
        glm::vec3(3.0f, 0.0f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        0.0f,
        0.0f);

    for (int i = 0; i < 240; ++i)
    {
        slidingWorld.update(1.0f / 120.0f);
        frictionlessWorld.update(1.0f / 120.0f);
    }

    const glm::vec3 rollingLinearVelocity =
        slidingWorld.linearVelocity(slidingSphere);
    const glm::vec3 rollingAngularVelocity =
        slidingWorld.angularVelocity(slidingSphere);
    const glm::vec3 rollingContactArm(0.0f, -0.5f, 0.0f);
    const glm::vec3 rollingContactVelocity = rollingLinearVelocity +
        glm::cross(rollingAngularVelocity, rollingContactArm);
    passed &= expect(
        slidingSphere.position().x < frictionlessSphere.position().x * 0.85f,
        "ground friction reduces the travel distance compared with a frictionless sphere");
    passed &= expect(
        std::fabs(rollingContactVelocity.x) < 0.05f &&
            std::fabs(rollingContactVelocity.z) < 0.05f,
        "ground friction transitions a sliding sphere into rolling without slipping");
    passed &= expect(
        std::fabs(rollingAngularVelocity.z) > 1.0f,
        "ground friction generates the angular velocity required for rolling");

    const glm::quat rollingOrientation = slidingSphere.orientation();
    passed &= expect(
        std::fabs(glm::dot(rollingOrientation, rollingOrientation) - 1.0f) < 1e-4f,
        "integrated orientation remains a normalized quaternion");
    slidingWorld.reset();
    passed &= expect(
        glm::length(slidingWorld.angularVelocity(slidingSphere)) < 1e-6f &&
            glm::length(glm::vec3(slidingSphere.orientation().x,
                                  slidingSphere.orientation().y,
                                  slidingSphere.orientation().z)) < 1e-6f,
        "reset restores the initial angular velocity and orientation");

    PhysicsWorld staticFrictionWorld;
    staticFrictionWorld.setGroundPlane(0.0f, 0.8f, 0.6f);
    Transform barelySlidingSphere;
    barelySlidingSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    staticFrictionWorld.addBody(
        barelySlidingSphere,
        glm::vec3(0.02f, 0.0f, 0.0f),
        0.5f,
        0.0f,
        1.0f,
        0.8f,
        0.6f);
    for (int i = 0; i < 30; ++i)
        staticFrictionWorld.update(1.0f / 120.0f);
    passed &= expect(
        barelySlidingSphere.position().x < 0.005f,
        "static ground friction prevents small persistent sliding");

    PhysicsWorld overlapWorld;
    overlapWorld.setGravity(glm::vec3(0.0f));
    Transform overlappingA;
    Transform overlappingB;
    overlapWorld.addBody(overlappingA, glm::vec3(0.0f), 0.5f);
    overlapWorld.addBody(overlappingB, glm::vec3(0.0f), 0.5f);
    overlapWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::isfinite(glm::length(overlappingB.position() - overlappingA.position())) &&
            glm::length(overlappingB.position() - overlappingA.position()) > 0.99f,
        "coincident spheres are separated without producing invalid positions");

    PhysicsWorld angularImpulseWorld;
    angularImpulseWorld.setGravity(glm::vec3(0.0f));
    Transform angularImpulseBody;
    angularImpulseBody.move(glm::vec3(0.0f, 1.0f, 0.0f));
    angularImpulseWorld.addBody(
        angularImpulseBody, glm::vec3(0.0f), 0.5f, 0.0f, 2.0f);
    const glm::vec3 positionBeforeAngularImpulse = angularImpulseBody.position();
    passed &= expect(
        angularImpulseWorld.applyAngularImpulse(
            angularImpulseBody, glm::vec3(0.0f, 0.8f, 0.0f)),
        "angular impulse is accepted for a dynamic sphere");
    passed &= expect(
        glm::length(angularImpulseWorld.linearVelocity(angularImpulseBody)) < 1e-6f &&
            angularImpulseWorld.angularVelocity(angularImpulseBody).y > 0.0f,
        "angular impulse changes spin without changing linear velocity");
    angularImpulseWorld.update(1.0f / 120.0f);
    passed &= expect(
        glm::length(angularImpulseBody.position() - positionBeforeAngularImpulse) < 1e-6f &&
            std::fabs(glm::dot(angularImpulseBody.orientation(),
                               glm::quat(1.0f, 0.0f, 0.0f, 0.0f))) < 0.99999f,
        "angular impulse rotates the body in place");

    PhysicsWorld axisForceWorld;
    axisForceWorld.setGravity(glm::vec3(0.0f));
    Transform axisForceBody;
    axisForceWorld.addBody(axisForceBody, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);
    axisForceWorld.applyForce(axisForceBody, glm::vec3(0.0f, 0.0f, 12.0f));
    axisForceWorld.update(1.0f / 120.0f);
    const glm::vec3 forceVelocity = axisForceWorld.linearVelocity(axisForceBody);
    passed &= expect(
        forceVelocity.z > 0.0f && std::fabs(forceVelocity.x) < 1e-6f &&
            std::fabs(forceVelocity.y) < 1e-6f,
        "a force on the Z axis produces translation along Z only");

    PhysicsWorld showcaseWorld;
    showcaseWorld.setGroundPlane(0.0f, 0.95f, 0.78f);
    Transform orangeShowcaseSphere;
    Transform cyanShowcaseSphere;
    orangeShowcaseSphere.move(glm::vec3(-2.7f, 0.5f, 0.0f));
    cyanShowcaseSphere.move(glm::vec3(2.7f, 0.5f, 0.0f));
    showcaseWorld.addBody(
        orangeShowcaseSphere, glm::vec3(3.2f, 0.0f, 0.0f),
        0.5f, 0.35f, 1.0f, 0.95f, 0.78f);
    showcaseWorld.addBody(
        cyanShowcaseSphere, glm::vec3(-2.4f, 0.0f, 0.0f),
        0.5f, 0.35f, 2.0f, 0.95f, 0.78f);
    for (int i = 0; i < 180; ++i)
        showcaseWorld.update(1.0f / 120.0f);

    const float showcaseDistance = glm::length(
        cyanShowcaseSphere.position() - orangeShowcaseSphere.position());
    passed &= expect(
        std::fabs(orangeShowcaseSphere.position().y - 0.5f) < 1e-4f &&
            std::fabs(cyanShowcaseSphere.position().y - 0.5f) < 1e-4f,
        "Dynamis rolling demonstration keeps both spheres supported by the floor");
    passed &= expect(
        showcaseDistance >= 0.999f,
        "Dynamis rolling demonstration resolves the head-on sphere collision");
    passed &= expect(
        std::fabs(showcaseWorld.angularVelocity(orangeShowcaseSphere).z) > 0.5f &&
            std::fabs(showcaseWorld.angularVelocity(cyanShowcaseSphere).z) > 0.5f,
        "both spheres acquire visible spin during the rolling collision demonstration");

    Transform orientationTransform;
    orientationTransform.setRotation(glm::vec3(25.0f, 40.0f, 15.0f));
    const glm::mat4 originalOrientationMatrix = orientationTransform.matrix();
    orientationTransform.setOrientation(orientationTransform.orientation());
    const glm::mat4 restoredOrientationMatrix = orientationTransform.matrix();
    float orientationMatrixError = 0.0f;
    for (int column = 0; column < 4; ++column)
    {
        for (int row = 0; row < 4; ++row)
        {
            orientationMatrixError += std::fabs(
                originalOrientationMatrix[column][row] -
                restoredOrientationMatrix[column][row]);
        }
    }
    passed &= expect(
        orientationMatrixError < 1e-4f,
        "quaternion orientation preserves the existing Y-X-Z Euler convention");

    // O controle WASD aplica a mesma força aos corpos selecionados. A massa
    // determina a aceleração, enquanto o contato com o piso produz o rolamento.
    PhysicsWorld keyboardControlWorld;
    keyboardControlWorld.setGroundPlane(0.0f, 0.95f, 0.78f);
    Transform lightControlledSphere;
    Transform heavyControlledSphere;
    lightControlledSphere.move(glm::vec3(-2.0f, 0.5f, 0.0f));
    heavyControlledSphere.move(glm::vec3(2.0f, 0.5f, 0.0f));
    keyboardControlWorld.addBody(
        lightControlledSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    keyboardControlWorld.addBody(
        heavyControlledSphere, glm::vec3(0.0f), 0.5f, 0.0f, 2.0f, 0.95f, 0.78f);
    for (int i = 0; i < 120; ++i)
    {
        keyboardControlWorld.applyForce(
            lightControlledSphere, glm::vec3(12.0f, 0.0f, 0.0f));
        keyboardControlWorld.applyForce(
            heavyControlledSphere, glm::vec3(12.0f, 0.0f, 0.0f));
        keyboardControlWorld.update(1.0f / 120.0f);
    }
    const float lightControlSpeed =
        keyboardControlWorld.linearVelocity(lightControlledSphere).x;
    const float heavyControlSpeed =
        keyboardControlWorld.linearVelocity(heavyControlledSphere).x;
    passed &= expect(
        lightControlledSphere.position().x > -2.0f &&
            heavyControlledSphere.position().x > 2.0f &&
            lightControlSpeed > 0.0f && heavyControlSpeed > 0.0f,
        "continuous WASD-style force moves a selected body without teleporting it");
    passed &= expect(
        lightControlSpeed > heavyControlSpeed * 1.9f &&
            lightControlSpeed < heavyControlSpeed * 2.1f,
        "the same WASD force accelerates a 1 kg sphere more than a 2 kg sphere");
    passed &= expect(
        keyboardControlWorld.angularVelocity(lightControlledSphere).z < 0.0f &&
            keyboardControlWorld.angularVelocity(heavyControlledSphere).z < 0.0f,
        "WASD force at sphere center produces natural rolling through ground friction");

    PhysicsWorld bodyQueryWorld;
    Transform queriedDynamicBody;
    Transform queriedStaticBody;
    Transform queriedUnregisteredBody;
    bodyQueryWorld.addBody(
        queriedDynamicBody, glm::vec3(0.0f), 0.5f, 0.0f, 2.5f);
    bodyQueryWorld.addStaticBody(queriedStaticBody, 0.5f);
    passed &= expect(
        bodyQueryWorld.hasDynamicBody(queriedDynamicBody) &&
            !bodyQueryWorld.hasDynamicBody(queriedStaticBody) &&
            !bodyQueryWorld.hasDynamicBody(queriedUnregisteredBody),
        "dynamic body lookup distinguishes dynamic, static, and unregistered transforms");
    PhysicsWorld invalidMassWorld;
    Transform invalidMassBody;
    passed &= expect(
        !invalidMassWorld.addBody(
            invalidMassBody, glm::vec3(0.0f), 0.5f, 0.0f, 0.0f) &&
            invalidMassWorld.bodyCount() == 0,
        "zero mass is rejected instead of creating an invalid dynamic body");


    // Força contínua: F = m*a, e a força é consumida após os subpassos do update.
    PhysicsWorld forceWorld;
    forceWorld.setGravity(glm::vec3(0.0f));
    Transform forceBody;
    forceWorld.addBody(forceBody, glm::vec3(0.0f), 0.5f, 0.0f, 2.0f);
    passed &= expect(
        forceWorld.applyForce(forceBody, glm::vec3(4.0f, 0.0f, 0.0f)),
        "registered dynamic body accepts an external force");
    forceWorld.update(2.0f / 120.0f);
    const float velocityAfterForce = forceWorld.linearVelocity(forceBody).x;
    passed &= expect(
        std::fabs(velocityAfterForce - (4.0f / 2.0f) * (2.0f / 120.0f)) < 1e-5f,
        "external force produces acceleration according to mass across all substeps");
    forceWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(forceWorld.linearVelocity(forceBody).x - velocityAfterForce) < 1e-6f,
        "accumulated force is cleared after the update consumes it");

    // Uma força aplicada antes de haver um passo fixo permanece pendente.
    PhysicsWorld deferredForceWorld;
    deferredForceWorld.setGravity(glm::vec3(0.0f));
    Transform deferredForceBody;
    deferredForceWorld.addBody(deferredForceBody, glm::vec3(0.0f), 0.5f, 0.0f);
    deferredForceWorld.applyForce(deferredForceBody, glm::vec3(6.0f, 0.0f, 0.0f));
    deferredForceWorld.update(1.0f / 240.0f);
    passed &= expect(
        glm::length(deferredForceWorld.linearVelocity(deferredForceBody)) < 1e-6f,
        "force waits when the accumulator has not reached a fixed step");
    deferredForceWorld.update(1.0f / 240.0f);
    passed &= expect(
        std::fabs(deferredForceWorld.linearVelocity(deferredForceBody).x - 6.0f / 120.0f) < 1e-5f,
        "deferred force is applied when the next fixed step becomes available");

    // Força contínua é definida uma vez por frame e lida em cada passo fixo.
    // O resultado deve independer da frequência de renderização.
    const auto simulateContinuousForce = [](int renderFps)
    {
        PhysicsWorld world;
        world.setGravity(glm::vec3(0.0f));
        Transform body;
        world.addBody(body, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);

        const float frameTime = 1.0f / static_cast<float>(renderFps);
        for (int frame = 0; frame < renderFps; ++frame)
        {
            world.setContinuousForce(body, glm::vec3(12.0f, 0.0f, 0.0f));
            world.update(frameTime);
        }
        return world.linearVelocity(body).x;
    };
    const float forceAt60Fps = simulateContinuousForce(60);
    const float forceAt144Fps = simulateContinuousForce(144);
    const float forceAt240Fps = simulateContinuousForce(240);
    passed &= expect(
        std::fabs(forceAt60Fps - 12.0f) < 0.03f &&
            std::fabs(forceAt144Fps - 12.0f) < 0.03f &&
            std::fabs(forceAt240Fps - 12.0f) < 0.03f,
        "continuous force produces the same acceleration at 60, 144, and 240 render FPS");

    PhysicsWorld releasedForceWorld;
    releasedForceWorld.setGravity(glm::vec3(0.0f));
    Transform releasedForceBody;
    releasedForceWorld.addBody(releasedForceBody, glm::vec3(0.0f), 0.5f, 0.0f);
    releasedForceWorld.setContinuousForce(
        releasedForceBody, glm::vec3(10.0f, 0.0f, 0.0f));
    releasedForceWorld.update(1.0f / 120.0f);
    const float velocityBeforeRelease =
        releasedForceWorld.linearVelocity(releasedForceBody).x;
    releasedForceWorld.setContinuousForce(releasedForceBody, glm::vec3(0.0f));
    releasedForceWorld.update(1.0f / 120.0f);
    passed &= expect(
        velocityBeforeRelease > 0.0f &&
            std::fabs(releasedForceWorld.linearVelocity(releasedForceBody).x -
                      velocityBeforeRelease) < 1e-6f,
        "releasing a continuous force stops acceleration but preserves inertia");

    // O colisor esférico segue a maior escala absoluta do Transform.
    PhysicsWorld scaledColliderWorld;
    scaledColliderWorld.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    scaledColliderWorld.setGroundPlane(0.0f);
    Transform scaledColliderBody;
    scaledColliderBody.move(glm::vec3(0.0f, 0.5f, 0.0f));
    scaledColliderWorld.addBody(
        scaledColliderBody, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);
    scaledColliderBody.setScale(glm::vec3(1.0f, 2.0f, 0.75f));
    scaledColliderWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(scaledColliderBody.position().y - 1.0f) < 1e-4f,
        "sphere collider expands with the largest absolute Transform scale component");

    PhysicsWorld scaledInertiaWorld;
    scaledInertiaWorld.setGravity(glm::vec3(0.0f));
    Transform scaledInertiaBody;
    scaledInertiaWorld.addBody(
        scaledInertiaBody, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);
    scaledInertiaBody.setScale(glm::vec3(2.0f));
    scaledInertiaWorld.applyAngularImpulse(
        scaledInertiaBody, glm::vec3(0.0f, 0.0f, 1.0f));
    passed &= expect(
        std::fabs(scaledInertiaWorld.angularVelocity(scaledInertiaBody).z - 2.5f) < 1e-4f,
        "sphere moment of inertia is recalculated after its Transform scale changes");

    // Subpassos adaptativos impedem que uma esfera rápida atravesse outro corpo.
    PhysicsWorld fastCollisionWorld;
    fastCollisionWorld.setGravity(glm::vec3(0.0f));
    Transform fastStaticSphere;
    Transform fastMovingSphere;
    fastStaticSphere.move(glm::vec3(0.0f));
    fastMovingSphere.move(glm::vec3(-2.0f, 0.0f, 0.0f));
    fastCollisionWorld.addStaticBody(
        fastStaticSphere, 0.5f, 1.0f, 0.0f, 0.0f);
    fastCollisionWorld.addBody(
        fastMovingSphere, glm::vec3(300.0f, 0.0f, 0.0f),
        0.5f, 1.0f, 1.0f, 0.0f, 0.0f);
    fastCollisionWorld.update(1.0f / 120.0f);
    passed &= expect(
        fastCollisionWorld.linearVelocity(fastMovingSphere).x < 0.0f &&
            fastMovingSphere.position().x < -1.0f,
        "adaptive substeps prevent a high-speed sphere from tunneling through a static sphere");

    const auto simulateContinuousTorque = [](int renderFps)
    {
        PhysicsWorld world;
        world.setGravity(glm::vec3(0.0f));
        Transform body;
        world.addBody(body, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);

        const float frameTime = 1.0f / static_cast<float>(renderFps);
        for (int frame = 0; frame < renderFps; ++frame)
        {
            world.setContinuousTorque(body, glm::vec3(0.0f, 0.0f, 0.1f));
            world.update(frameTime);
        }
        return world.angularVelocity(body).z;
    };
    const float torqueAt60Fps = simulateContinuousTorque(60);
    const float torqueAt240Fps = simulateContinuousTorque(240);
    passed &= expect(
        std::fabs(torqueAt60Fps - 1.0f) < 0.01f &&
            std::fabs(torqueAt240Fps - 1.0f) < 0.01f,
        "continuous torque produces the same angular acceleration at different render FPS");

    PhysicsWorld torqueWorld;
    torqueWorld.setGravity(glm::vec3(0.0f));
    Transform torqueBody;
    torqueWorld.addBody(torqueBody, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f);
    passed &= expect(
        torqueWorld.applyTorque(torqueBody, glm::vec3(0.0f, 0.0f, 1.0f)),
        "dynamic sphere accepts external torque");
    torqueWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(torqueWorld.angularVelocity(torqueBody).z - (1.0f / 0.1f) / 120.0f) < 1e-5f,
        "torque produces angular acceleration using the sphere moment of inertia");
    passed &= expect(
        glm::length(torqueWorld.linearVelocity(torqueBody)) < 1e-6f,
        "pure torque does not introduce linear velocity");

    PhysicsWorld impulseWorld;
    impulseWorld.setGravity(glm::vec3(0.0f));
    Transform impulseBody;
    impulseWorld.addBody(impulseBody, glm::vec3(0.0f), 0.5f, 0.0f, 2.0f);
    passed &= expect(
        impulseWorld.applyImpulseAtPoint(
            impulseBody,
            glm::vec3(2.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.25f, 0.0f)),
        "off-center impulse is accepted");
    passed &= expect(
        std::fabs(impulseWorld.linearVelocity(impulseBody).x - 1.0f) < 1e-5f &&
            std::fabs(impulseWorld.angularVelocity(impulseBody).z + 2.5f) < 1e-5f,
        "off-center impulse changes both linear and angular velocity immediately");

    PhysicsWorld forceAtPointWorld;
    forceAtPointWorld.setGravity(glm::vec3(0.0f));
    Transform forceAtPointBody;
    forceAtPointWorld.addBody(forceAtPointBody, glm::vec3(0.0f), 0.5f, 0.0f);
    passed &= expect(
        forceAtPointWorld.applyForceAtPoint(
            forceAtPointBody,
            glm::vec3(4.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.25f, 0.0f)),
        "force applied at a world-space point is accepted");
    forceAtPointWorld.update(1.0f / 120.0f);
    passed &= expect(
        forceAtPointWorld.linearVelocity(forceAtPointBody).x > 0.03f &&
            forceAtPointWorld.angularVelocity(forceAtPointBody).z < -0.08f,
        "off-center force generates both translation and torque");

    PhysicsWorld guardedForceWorld;
    Transform staticForceBody;
    guardedForceWorld.addStaticBody(staticForceBody, 0.5f);
    passed &= expect(
        !guardedForceWorld.applyForce(staticForceBody, glm::vec3(10.0f, 0.0f, 0.0f)) &&
            !guardedForceWorld.applyImpulse(staticForceBody, glm::vec3(10.0f, 0.0f, 0.0f)) &&
            !guardedForceWorld.applyTorque(staticForceBody, glm::vec3(0.0f, 0.0f, 10.0f)),
        "static bodies reject external force, impulse, and torque");
    Transform unregisteredBody;
    passed &= expect(
        !guardedForceWorld.applyForce(unregisteredBody, glm::vec3(1.0f, 0.0f, 0.0f)),
        "unregistered transforms reject external forces");
    passed &= expect(
        !guardedForceWorld.applyImpulse(
            staticForceBody,
            glm::vec3(std::numeric_limits<float>::infinity(), 0.0f, 0.0f)),
        "non-finite impulses are rejected");

    PhysicsWorld fastGroundWorld;
    fastGroundWorld.setGravity(glm::vec3(0.0f));
    fastGroundWorld.setGroundPlane(0.0f, 0.0f, 0.0f);
    Transform fastFallingSphere;
    fastFallingSphere.move(glm::vec3(0.0f, 2.0f, 0.0f));
    fastGroundWorld.addBody(
        fastFallingSphere, glm::vec3(0.0f, -300.0f, 0.0f),
        0.5f, 1.0f, 1.0f, 0.0f, 0.0f);
    fastGroundWorld.update(1.0f / 120.0f);
    passed &= expect(
        fastFallingSphere.position().y > 0.5f &&
            fastGroundWorld.linearVelocity(fastFallingSphere).y > 0.0f,
        "adaptive substeps prevent a high-speed sphere from tunneling through the ground plane");

    std::vector<Vertex> bowlVertices;
    std::vector<unsigned int> bowlIndices;
    passed &= expect(
        shapes::bowl(10.0f, 4.0f, 1.0f, 16, 32,
                     bowlVertices, bowlIndices),
        "procedural bowl geometry accepts valid dimensions and subdivisions");
    passed &= expect(
        bowlVertices.size() == 1 + 16 * 33 &&
            bowlIndices.size() == 32 * 3 + 15 * 32 * 6,
        "bowl geometry creates a single center vertex and indexed radial rings");
    passed &= expect(
        std::fabs(bowlVertices.front().position.y) < 1e-6f &&
            std::fabs(bowlVertices.back().position.y - 5.0f) < 1e-4f,
        "bowl profile reaches the requested height at its outer rim");
    bool bowlNormalsValid = true;
    for (const Vertex& vertex : bowlVertices)
    {
        const float normalLength = glm::length(vertex.normal);
        if (!std::isfinite(normalLength) ||
            std::fabs(normalLength - 1.0f) > 1e-4f ||
            vertex.normal.y <= 0.0f)
        {
            bowlNormalsValid = false;
            break;
        }
    }
    passed &= expect(
        bowlNormalsValid,
        "bowl vertices have normalized upward-facing surface normals");
    bool bowlWindingValid = true;
    for (std::size_t index = 0; index + 2 < bowlIndices.size(); index += 3)
    {
        const Vertex& a = bowlVertices[bowlIndices[index + 0]];
        const Vertex& b = bowlVertices[bowlIndices[index + 1]];
        const Vertex& c = bowlVertices[bowlIndices[index + 2]];
        const glm::vec3 faceNormal = glm::cross(
            b.position - a.position, c.position - a.position);
        const glm::vec3 averageNormal = a.normal + b.normal + c.normal;
        if (glm::dot(faceNormal, averageNormal) <= 0.0f)
        {
            bowlWindingValid = false;
            break;
        }
    }
    passed &= expect(
        bowlWindingValid,
        "bowl triangles face the inside of the basin consistently with their normals");
    passed &= expect(
        !shapes::bowl(0.0f, 4.0f, 1.0f, 16, 32,
                      bowlVertices, bowlIndices),
        "bowl geometry rejects invalid dimensions");

    PhysicsWorld bowlWorld;
    bowlWorld.setGroundBowl(10.0f, 4.0f, 1.0f, 0.95f, 0.78f);
    Transform bowlCenterSphere;
    bowlCenterSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    bowlWorld.addBody(
        bowlCenterSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    for (int i = 0; i < 120; ++i)
        bowlWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(bowlCenterSphere.position().y - 0.5f) < 1e-4f &&
            std::fabs(bowlCenterSphere.position().x) < 1e-4f &&
            std::fabs(bowlCenterSphere.position().z) < 1e-4f,
        "sphere remains at rest at the lowest point of the bowl");

    PhysicsWorld longTermBowlWorld;
    longTermBowlWorld.setGroundBowl(10.0f, 4.0f, 1.0f, 0.95f, 0.78f);
    Transform longTermRestingSphere;
    longTermRestingSphere.move(glm::vec3(0.0f, 0.5f, 0.0f));
    longTermBowlWorld.addBody(
        longTermRestingSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    for (int i = 0; i < 1200; ++i)
        longTermBowlWorld.update(1.0f / 120.0f);
    passed &= expect(
        std::fabs(longTermRestingSphere.position().y - 0.5f) < 1e-4f &&
            glm::length(longTermBowlWorld.linearVelocity(longTermRestingSphere)) < 1e-5f &&
            glm::length(longTermBowlWorld.angularVelocity(longTermRestingSphere)) < 1e-5f,
        "a resting sphere remains stable at the bowl bottom over ten simulated seconds");

    PhysicsWorld bowlRollingWorld;
    bowlRollingWorld.setGroundBowl(10.0f, 4.0f, 1.0f, 0.95f, 0.78f);
    Transform bowlSideSphere;
    // Centro da esfera = ponto da superfície (r=5) + normal * raio.
    const float bowlSlope = 0.4f;
    const glm::vec3 bowlNormal = glm::normalize(
        glm::vec3(-bowlSlope, 1.0f, 0.0f));
    const glm::vec3 bowlSurfacePoint(5.0f, 1.0f, 0.0f);
    bowlSideSphere.move(bowlSurfacePoint + bowlNormal * 0.5f);
    bowlRollingWorld.addBody(
        bowlSideSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    for (int i = 0; i < 60; ++i)
        bowlRollingWorld.update(1.0f / 120.0f);
    passed &= expect(
        bowlSideSphere.position().x < bowlSurfacePoint.x - 0.05f,
        "gravity accelerates a sphere down the inclined bowl toward its center");
    passed &= expect(
        glm::length(bowlRollingWorld.angularVelocity(bowlSideSphere)) > 0.1f,
        "friction against the inclined bowl creates rolling angular velocity");

    PhysicsWorld longRunBowlWorld;
    longRunBowlWorld.setGroundBowl(18.0f, 4.5f, 2.5f, 0.95f, 0.78f);
    Transform longRunSlopeSphere;
    const float initialBowlSlope = 2.0f * 4.5f * 5.0f / (18.0f * 18.0f);
    const glm::vec3 initialBowlNormal = glm::normalize(
        glm::vec3(-initialBowlSlope, 1.0f, 0.0f));
    const glm::vec3 initialBowlSurfacePoint(
        5.0f, 4.5f * 25.0f / (18.0f * 18.0f), 0.0f);
    longRunSlopeSphere.move(initialBowlSurfacePoint + initialBowlNormal * 0.5f);
    longRunBowlWorld.addBody(
        longRunSlopeSphere, glm::vec3(0.0f), 0.5f, 0.0f, 1.0f, 0.95f, 0.78f);
    for (int i = 0; i < 1200; ++i)
        longRunBowlWorld.update(1.0f / 120.0f);
    const glm::vec3 longRunPosition = longRunSlopeSphere.position();
    const float longRunRadialDistance = std::sqrt(
        longRunPosition.x * longRunPosition.x +
        longRunPosition.z * longRunPosition.z);
    const bool longRunFinite =
        std::isfinite(longRunPosition.x) && std::isfinite(longRunPosition.y) &&
        std::isfinite(longRunPosition.z) &&
        std::isfinite(glm::length(longRunBowlWorld.linearVelocity(longRunSlopeSphere))) &&
        std::isfinite(glm::length(longRunBowlWorld.angularVelocity(longRunSlopeSphere)));
    passed &= expect(
        longRunFinite && longRunRadialDistance < 18.0f &&
            longRunPosition.y > 0.0f,
        "a sphere rolling along the curved bowl remains finite and contained over ten seconds");

    return passed ? 0 : 1;
}
