#include "renderer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/graphics/geometry/shapes.h"

namespace
{
using FrustumPlanes = std::array<glm::vec4, 6>;

bool finite(const glm::vec3& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
        std::isfinite(value.z);
}

bool extractFrustumPlanes(
    const glm::mat4& viewProjection,
    FrustumPlanes& planes)
{
    glm::vec4 rows[4];
    for (int row = 0; row < 4; ++row)
    {
        rows[row] = glm::vec4(
            viewProjection[0][row],
            viewProjection[1][row],
            viewProjection[2][row],
            viewProjection[3][row]);
    }

    planes = {{
        rows[3] + rows[0], rows[3] - rows[0],
        rows[3] + rows[1], rows[3] - rows[1],
        rows[3] + rows[2], rows[3] - rows[2]}};

    for (glm::vec4& plane : planes)
    {
        const float normalLength = glm::length(glm::vec3(plane));
        if (!std::isfinite(normalLength) || normalLength <= 1e-7f ||
            !std::isfinite(plane.w))
            return false;
        plane /= normalLength;
    }
    return true;
}

bool isInsideFrustum(
    const BoundingBox& bounds,
    const glm::mat4& modelMatrix,
    const FrustumPlanes& planes)
{
    if (!bounds.valid || !finite(bounds.min) || !finite(bounds.max))
        return true;

    const glm::vec3 localCenter = (bounds.min + bounds.max) * 0.5f;
    const glm::vec3 localHalfExtents =
        glm::max((bounds.max - bounds.min) * 0.5f, glm::vec3(0.0f));
    const glm::vec3 center(
        modelMatrix * glm::vec4(localCenter, 1.0f));
    const glm::mat3 basis(modelMatrix);
    const glm::vec3 axisX = basis[0];
    const glm::vec3 axisY = basis[1];
    const glm::vec3 axisZ = basis[2];
    if (!finite(center) || !finite(axisX) ||
        !finite(axisY) || !finite(axisZ) || !finite(localHalfExtents))
        return true;

    for (const glm::vec4& plane : planes)
    {
        const glm::vec3 normal(plane);
        const float projectedRadius =
            std::fabs(glm::dot(normal, axisX)) * localHalfExtents.x +
            std::fabs(glm::dot(normal, axisY)) * localHalfExtents.y +
            std::fabs(glm::dot(normal, axisZ)) * localHalfExtents.z;
        const float distance = glm::dot(normal, center) + plane.w;
        const float margin = 1e-4f *
            (1.0f + glm::length(center) + projectedRadius);
        if (distance + projectedRadius < -margin)
            return false;
    }
    return true;
}

void setFrontFaceForTransform(const glm::mat4& matrix)
{
    const float determinant = glm::determinant(glm::mat3(matrix));
    glFrontFace(std::isfinite(determinant) && determinant < 0.0f
        ? GL_CW : GL_CCW);
}

std::vector<float> buildDirectionalLightGizmoVertices()
{
    std::vector<float> vertices;
    vertices.reserve(32 * 6 + 8 * 6 + 18);

    const auto addLine = [&vertices](const glm::vec3& a, const glm::vec3& b)
    {
        vertices.insert(vertices.end(), {
            a.x, a.y, a.z,
            b.x, b.y, b.z});
    };

    // Anel do sol, no plano perpendicular à direção dos raios.
    constexpr int circleSegments = 32;
    constexpr float pi = 3.14159265358979323846f;
    constexpr float sunRadius = 0.28f;
    for (int i = 0; i < circleSegments; ++i)
    {
        const float a0 = 2.0f * pi * static_cast<float>(i) /
            static_cast<float>(circleSegments);
        const float a1 = 2.0f * pi * static_cast<float>(i + 1) /
            static_cast<float>(circleSegments);
        addLine(
            {std::cos(a0) * sunRadius, std::sin(a0) * sunRadius, 0.0f},
            {std::cos(a1) * sunRadius, std::sin(a1) * sunRadius, 0.0f});
    }

    // Raios ao redor do sol.
    constexpr int rayCount = 8;
    for (int i = 0; i < rayCount; ++i)
    {
        const float angle = 2.0f * pi * static_cast<float>(i) /
            static_cast<float>(rayCount);
        const glm::vec2 direction(std::cos(angle), std::sin(angle));
        addLine(
            {direction.x * 0.37f, direction.y * 0.37f, 0.0f},
            {direction.x * 0.53f, direction.y * 0.53f, 0.0f});
    }

    // Seta saindo do sol na direção em que a luz viaja.
    addLine({0.0f, 0.0f, 0.18f}, {0.0f, 0.0f, 1.48f});
    addLine({0.0f, 0.0f, 1.78f}, {0.19f, 0.0f, 1.48f});
    addLine({0.0f, 0.0f, 1.78f}, {-0.19f, 0.0f, 1.48f});

    return vertices;
}
} // namespace

Renderer::~Renderer()
{
    if (shadowFramebuffer != 0)
        glDeleteFramebuffers(1, &shadowFramebuffer);
    if (shadowDepthTexture != 0)
        glDeleteTextures(1, &shadowDepthTexture);

    const GLuint framebuffers[] = {
        ssaoFramebuffer, ssaoOutputFramebuffer, ssaoBlurFramebuffer};
    glDeleteFramebuffers(3, framebuffers);

    const GLuint textures[] = {
        ssaoNormalTexture, ssaoDepthTexture, ssaoNoiseTexture,
        ssaoOutputTexture, ssaoBlurTexture};
    glDeleteTextures(5, textures);

    if (fullscreenVao != 0)
        glDeleteVertexArrays(1, &fullscreenVao);
}

bool Renderer::createShadowResources()
{
    if (shadowShader.ID == 0)
        return false;

    GLint previousTexture = 0;
    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    glGenTextures(1, &shadowDepthTexture);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        kShadowMapResolution,
        kShadowMapResolution,
        0,
        GL_DEPTH_COMPONENT,
        GL_FLOAT,
        nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    const GLfloat borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glGenFramebuffers(1, &shadowFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        shadowDepthTexture,
        0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "Shadow Mapping desativado: framebuffer incompleto (0x"
                  << std::hex << status << std::dec << ")." << std::endl;
        if (shadowFramebuffer != 0)
            glDeleteFramebuffers(1, &shadowFramebuffer);
        if (shadowDepthTexture != 0)
            glDeleteTextures(1, &shadowDepthTexture);
        shadowFramebuffer = 0;
        shadowDepthTexture = 0;
        return false;
    }

    return true;
}

bool Renderer::buildDirectionalLightSpaceMatrix(
    const Scene &scene,
    const Light &light,
    glm::mat4 &lightSpaceMatrix) const
{
    glm::vec3 lightDirection = light.direction();
    const float directionLength = glm::length(lightDirection);
    if (!finite(lightDirection) || !std::isfinite(directionLength) ||
        directionLength < 1e-5f)
        return false;
    lightDirection /= directionLength;

    const float infinity = std::numeric_limits<float>::infinity();
    glm::vec3 worldMin(infinity);
    glm::vec3 worldMax(-infinity);
    bool hasBounds = false;

    for (const Scene::Object &object : scene.getObjects())
    {
        if (!object.visible)
            continue;

        const glm::mat4 modelMatrix = object.transform.matrix();
        const BoundingBox *bounds = object.boundingBox();
        if (!bounds || !bounds->valid || !finite(bounds->min) || !finite(bounds->max))
        {
            const glm::vec3 position = object.transform.position();
            if (!finite(position))
                continue;
            worldMin = glm::min(worldMin, position);
            worldMax = glm::max(worldMax, position);
            hasBounds = true;
            continue;
        }

        for (int corner = 0; corner < 8; ++corner)
        {
            const glm::vec3 localPoint(
                (corner & 1) ? bounds->max.x : bounds->min.x,
                (corner & 2) ? bounds->max.y : bounds->min.y,
                (corner & 4) ? bounds->max.z : bounds->min.z);
            const glm::vec4 transformed =
                modelMatrix * glm::vec4(localPoint, 1.0f);
            if (!std::isfinite(transformed.x) || !std::isfinite(transformed.y) ||
                !std::isfinite(transformed.z) || std::fabs(transformed.w) < 1e-6f)
                continue;

            const glm::vec3 point = glm::vec3(transformed) / transformed.w;
            if (!finite(point))
                continue;
            worldMin = glm::min(worldMin, point);
            worldMax = glm::max(worldMax, point);
            hasBounds = true;
        }
    }

    if (!hasBounds)
    {
        worldMin = glm::vec3(-4.0f);
        worldMax = glm::vec3(4.0f);
    }

    const glm::vec3 center = (worldMin + worldMax) * 0.5f;
    float radius = glm::length(worldMax - worldMin) * 0.5f;
    if (!finite(center) || !std::isfinite(radius))
        return false;
    radius = std::max(4.0f, radius * 1.15f + 0.5f);

    const glm::vec3 worldUp = std::fabs(glm::dot(lightDirection, glm::vec3(0, 1, 0))) > 0.96f
        ? glm::vec3(0, 0, 1)
        : glm::vec3(0, 1, 0);
    const glm::vec3 eye = center - lightDirection * (radius * 2.0f + 1.0f);
    const glm::mat4 lightView = glm::lookAt(eye, center, worldUp);
    const glm::mat4 lightProjection = glm::ortho(
        -radius, radius, -radius, radius, 0.1f, radius * 4.0f + 4.0f);
    lightSpaceMatrix = lightProjection * lightView;
    return true;
}

void Renderer::drawShadowScene(const Scene &scene)
{
    shadowShader.use();

    for (const Scene::Object &object : scene.getObjects())
    {
        if (!object.visible)
            continue;

        const glm::mat4 modelMatrix = object.transform.matrix();
        if (object.model)
        {
            object.model->drawDepth(shadowShader, modelMatrix);
        }
        else if (object.mesh)
        {
            setFrontFaceForTransform(modelMatrix);
            shadowShader.setMat4("model", modelMatrix);
            object.mesh->drawDepth();
        }
        glFrontFace(GL_CCW);
    }
}

void Renderer::renderShadowMap(
    const Scene &scene,
    const glm::mat4 &lightSpaceMatrix)
{
    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousViewport[4] = {0, 0, 0, 0};
    GLint previousDepthFunction = GL_LESS;
    GLboolean previousDepthWriteMask = GL_TRUE;
    GLfloat previousPolygonFactor = 0.0f;
    GLfloat previousPolygonUnits = 0.0f;
    const GLboolean depthTestWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean polygonOffsetWasEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthWriteMask);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &previousPolygonFactor);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &previousPolygonUnits);

    glViewport(0, 0, kShadowMapResolution, kShadowMapResolution);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    shadowShader.use();
    shadowShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    drawShadowScene(scene);

    if (polygonOffsetWasEnabled == GL_TRUE)
        glEnable(GL_POLYGON_OFFSET_FILL);
    else
        glDisable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(
        previousPolygonFactor,
        previousPolygonUnits);
    glDepthMask(previousDepthWriteMask);
    glDepthFunc(static_cast<GLenum>(previousDepthFunction));
    if (depthTestWasEnabled == GL_TRUE)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (cullFaceWasEnabled == GL_TRUE)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previousDrawFramebuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));
    glViewport(
        previousViewport[0], previousViewport[1],
        previousViewport[2], previousViewport[3]);
}

bool Renderer::loadSsaoShaders()
{
    const bool geometryReady = ssaoGeometryShader.createFromFiles(
        ASSETS_DIR "engine/shaders/ssao_geometry.vert",
        ASSETS_DIR "engine/shaders/ssao_geometry.frag");
    const bool aoReady = ssaoShader.createFromFiles(
        ASSETS_DIR "engine/shaders/fullscreen_triangle.vert",
        ASSETS_DIR "engine/shaders/ssao.frag");
    const bool blurReady = ssaoBlurShader.createFromFiles(
        ASSETS_DIR "engine/shaders/fullscreen_triangle.vert",
        ASSETS_DIR "engine/shaders/ssao_blur.frag");

    if (!geometryReady || !aoReady || !blurReady)
    {
        ssaoGeometryShader.destroy();
        ssaoShader.destroy();
        ssaoBlurShader.destroy();
        return false;
    }

    ssaoShader.use();
    ssaoShader.setInt("normalTexture", 0);
    ssaoShader.setInt("depthTexture", 1);
    ssaoShader.setInt("noiseTexture", 2);

    ssaoBlurShader.use();
    ssaoBlurShader.setInt("inputOcclusion", 0);
    ssaoBlurShader.setInt("depthTexture", 1);
    ssaoBlurShader.setInt("normalTexture", 2);
    return true;
}

bool Renderer::createSsaoResources()
{
    if (ssaoGeometryShader.ID == 0 || ssaoShader.ID == 0 ||
        ssaoBlurShader.ID == 0)
        return false;

    glGenFramebuffers(1, &ssaoFramebuffer);
    glGenFramebuffers(1, &ssaoOutputFramebuffer);
    glGenFramebuffers(1, &ssaoBlurFramebuffer);

    glGenTextures(1, &ssaoNormalTexture);
    glGenTextures(1, &ssaoDepthTexture);
    glGenTextures(1, &ssaoNoiseTexture);
    glGenTextures(1, &ssaoOutputTexture);
    glGenTextures(1, &ssaoBlurTexture);
    glGenVertexArrays(1, &fullscreenVao);

    if (ssaoFramebuffer == 0 || ssaoOutputFramebuffer == 0 ||
        ssaoBlurFramebuffer == 0 || ssaoNormalTexture == 0 ||
        ssaoDepthTexture == 0 || ssaoNoiseTexture == 0 ||
        ssaoOutputTexture == 0 || ssaoBlurTexture == 0 ||
        fullscreenVao == 0)
        return false;

    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);

    // Ruido de rotacao 4x4: quebra padroes visiveis sem depender de rand() global.
    std::array<float, 4 * 4 * 3> noise{};
    constexpr float goldenAngle = 2.39996322972865332f;
    for (int i = 0; i < 16; ++i)
    {
        const float angle = static_cast<float>(i) * goldenAngle;
        noise[static_cast<std::size_t>(i) * 3 + 0] = std::cos(angle);
        noise[static_cast<std::size_t>(i) * 3 + 1] = std::sin(angle);
        noise[static_cast<std::size_t>(i) * 3 + 2] = 0.0f;
    }

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0,
        GL_RGB, GL_FLOAT, noise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    return true;
}

bool Renderer::resizeSsaoResources(int width, int height)
{
    if (width <= 0 || height <= 0)
        return false;

    const int targetWidth = std::max(1, (width + 1) / 2);
    const int targetHeight = std::max(1, (height + 1) / 2);
    if (ssaoWidth == targetWidth && ssaoHeight == targetHeight)
        return true;

    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousTexture = 0;
    GLint previousActiveTexture = GL_TEXTURE0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    const auto allocateTexture = [](GLuint texture, GLint internalFormat,
                                    GLenum format, GLenum type, GLint filter,
                                    int textureWidth, int textureHeight)
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, internalFormat,
            textureWidth, textureHeight, 0, format, type, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    };

    allocateTexture(
        ssaoNormalTexture, GL_RGB16F, GL_RGB, GL_FLOAT,
        GL_NEAREST, targetWidth, targetHeight);
    allocateTexture(
        ssaoDepthTexture, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT,
        GL_NEAREST, targetWidth, targetHeight);
    allocateTexture(
        ssaoOutputTexture, GL_R16F, GL_RED, GL_FLOAT,
        GL_LINEAR, targetWidth, targetHeight);
    allocateTexture(
        ssaoBlurTexture, GL_R16F, GL_RED, GL_FLOAT,
        GL_LINEAR, targetWidth, targetHeight);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        ssaoNormalTexture, 0);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
        ssaoDepthTexture, 0);
    const GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    const GLenum geometryStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoOutputFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        ssaoOutputTexture, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    const GLenum outputStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFramebuffer);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
        ssaoBlurTexture, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    const GLenum blurStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                      static_cast<GLuint>(previousDrawFramebuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER,
                      static_cast<GLuint>(previousReadFramebuffer));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glActiveTexture(static_cast<GLenum>(previousActiveTexture));

    if (geometryStatus != GL_FRAMEBUFFER_COMPLETE ||
        outputStatus != GL_FRAMEBUFFER_COMPLETE ||
        blurStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cout << "SSAO desativado: framebuffer incompleto (geometry=0x"
                  << std::hex << geometryStatus << ", output=0x" << outputStatus
                  << ", blur=0x" << blurStatus << std::dec << ")." << std::endl;
        ssaoWidth = 0;
        ssaoHeight = 0;
        ssaoReady = false;
        return false;
    }

    ssaoWidth = targetWidth;
    ssaoHeight = targetHeight;
    return true;
}

void Renderer::drawSsaoGeometryScene(const Scene &scene)
{
    ssaoGeometryShader.use();
    for (const Scene::Object &object : scene.getObjects())
    {
        if (!object.visible)
            continue;

        const glm::mat4 modelMatrix = object.transform.matrix();
        if (object.model)
        {
            object.model->drawDepth(ssaoGeometryShader, modelMatrix);
        }
        else if (object.mesh)
        {
            setFrontFaceForTransform(modelMatrix);
            ssaoGeometryShader.setMat4("model", modelMatrix);
            object.mesh->drawDepth();
        }
        glFrontFace(GL_CCW);
    }
}

bool Renderer::renderSsao(
    const Scene &scene,
    const Camera &camera,
    int viewportWidth,
    int viewportHeight)
{
    if (!ssaoReady || !resizeSsaoResources(viewportWidth, viewportHeight))
        return false;

    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousViewport[4] = {0, 0, 0, 0};
    GLint previousDepthFunction = GL_LESS;
    GLint previousActiveTexture = GL_TEXTURE0;
    GLint previousVertexArray = 0;
    GLboolean previousDepthMask = GL_TRUE;
    GLboolean previousColorMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    const GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean cullWasEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    const GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVertexArray);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
    glGetBooleanv(GL_COLOR_WRITEMASK, previousColorMask);

    glDisable(GL_SCISSOR_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFramebuffer);
    glViewport(0, 0, ssaoWidth, ssaoHeight);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ssaoGeometryShader.use();
    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = camera.projectionMatrix();
    ssaoGeometryShader.setMat4("view", view);
    ssaoGeometryShader.setMat4("projection", projection);
    drawSsaoGeometryScene(scene);

    // Passo SSAO em meia resolucao: reduz custo de amostragem sem perder contato visual.
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoOutputFramebuffer);
    glViewport(0, 0, ssaoWidth, ssaoHeight);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader.use();
    ssaoShader.setMat4("projection", projection);
    ssaoShader.setMat4("inverseProjection", glm::inverse(projection));
    ssaoShader.setVec2(
        "noiseScale",
        static_cast<float>(ssaoWidth) / 4.0f,
        static_cast<float>(ssaoHeight) / 4.0f);
    ssaoShader.setFloat("radius", kSsaoRadius);
    ssaoShader.setFloat("bias", kSsaoBias);
    ssaoShader.setFloat("strength", 1.10f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoNormalTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ssaoDepthTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoNoiseTexture);
    glBindVertexArray(fullscreenVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Blur bilateral preserva bordas de profundidade e normais.
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFramebuffer);
    glViewport(0, 0, ssaoWidth, ssaoHeight);
    glClear(GL_COLOR_BUFFER_BIT);
    ssaoBlurShader.use();
    ssaoBlurShader.setMat4("inverseProjection", glm::inverse(projection));
    ssaoBlurShader.setVec2(
        "texelSize",
        1.0f / static_cast<float>(ssaoWidth),
        1.0f / static_cast<float>(ssaoHeight));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ssaoOutputTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, ssaoDepthTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, ssaoNormalTexture);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(static_cast<GLuint>(previousVertexArray));
    glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    glDepthMask(previousDepthMask);
    glColorMask(
        previousColorMask[0], previousColorMask[1],
        previousColorMask[2], previousColorMask[3]);
    glDepthFunc(static_cast<GLenum>(previousDepthFunction));
    if (depthWasEnabled == GL_TRUE)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (cullWasEnabled == GL_TRUE)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    if (blendWasEnabled == GL_TRUE)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (scissorWasEnabled == GL_TRUE)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                       static_cast<GLuint>(previousDrawFramebuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER,
                       static_cast<GLuint>(previousReadFramebuffer));
    glViewport(
        previousViewport[0], previousViewport[1],
        previousViewport[2], previousViewport[3]);
    return true;
}

bool Renderer::init()
{
    if (!defaultTexture.load(ASSETS_DIR "engine/textures/default.png"))
    {
        std::cout << "Falha ao carregar a textura padrao!" << std::endl;
        return false;
    }

    if (!loadShaders())
        return false;

    shadowMapReady = createShadowResources();
    if (!shadowMapReady)
        std::cout << "Shadow Mapping indisponivel; cena continuara sem sombras." << std::endl;

    ssaoReady = loadSsaoShaders() && createSsaoResources();
    if (!ssaoReady)
        std::cout << "SSAO indisponivel; cena continuara sem oclusao ambiente em tela." << std::endl;

    if (!lightMarker.createIndexed(
            shapes::cube(),
            shapes::cubeIndices(),
            3,
            GL_TRIANGLES,
            {3}))
    {
        std::cout << "Falha ao criar a malha do marcador da luz." << std::endl;
        return false;
    }

    const std::vector<float> directionalGizmoVertices =
        buildDirectionalLightGizmoVertices();
    if (!directionalLightGizmo.create(
            directionalGizmoVertices, 3, GL_LINES, {3}))
    {
        std::cout << "Falha ao criar o gizmo da luz direcional." << std::endl;
        return false;
    }

    rainScenario.init();

    if (!rainRenderer.init())
        return false;

    // O céu é opcional: sem ele, a cena usa a cor de fundo lisa.
    if (!skybox.load(ASSETS_DIR "engine/skybox/"))
        std::cout << "Ceu desativado: nao consegui carregar a pasta assets/skybox." << std::endl;

    // Objetos próximos escondem os que estão atrás; faces voltadas para longe da câmera são descartadas.
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    return true;
}

bool Renderer::loadShaders()
{
    if (!sceneShader.createFromFiles(
            ASSETS_DIR "engine/shaders/default.vert",
            ASSETS_DIR "engine/shaders/default.frag"))
    {
        std::cout << "Falha ao criar o shader da cena!" << std::endl;
        return false;
    }

    sceneShader.use();
    sceneShader.setInt("tex0", 0);
    sceneShader.setInt("specular0", 1);

    if (!lightShader.createFromFiles(
            ASSETS_DIR "engine/shaders/light.vert",
            ASSETS_DIR "engine/shaders/light.frag"))
    {
        std::cout << "Falha ao criar o shader da luz!" << std::endl;
        return false;
    }

    if (!shadowShader.createFromFiles(
            ASSETS_DIR "engine/shaders/shadow_depth.vert",
            ASSETS_DIR "engine/shaders/shadow_depth.frag"))
    {
        std::cout << "Falha ao criar shader de profundidade; sombras desativadas." << std::endl;
    }

    sceneShader.use();
    sceneShader.setInt("shadowMap", 6);
    return true;
}

void Renderer::resetSettings()
{
    showSkybox = kDefaultShowSkybox;
    showLightMarker = kDefaultShowLightMarker;
    shadowsEnabled = true;
    ambientOcclusionEnabled = true;
    ambientOcclusionStrength = kDefaultAmbientOcclusionStrength;
    ambientStrength = kDefaultAmbientStrength;
    environmentReflectionStrength = kDefaultEnvironmentReflectionStrength;
    rainScenario.enabled = false;
}

void Renderer::applyLight(const Light &light, const Camera &camera)
{
    sceneShader.setVec4("lightColor", light.color.x, light.color.y, light.color.z, 1.0f);
    sceneShader.setFloat("lightIntensity", light.intensity);
    sceneShader.setVec3("lightPos", light.position.x, light.position.y, light.position.z);
    sceneShader.setVec3("camPos", camera.position.x, camera.position.y, camera.position.z);

    const glm::vec3 direction = light.direction();
    sceneShader.setVec3("lightDir", direction.x, direction.y, direction.z);
    sceneShader.setInt("lightType", static_cast<int>(light.type));
}

void Renderer::drawLightMarker(const Light &light, Camera &camera)
{
    glm::mat4 model(1.0f);
    model = glm::translate(model, light.position);
    model = glm::scale(model, glm::vec3(0.2f));

    lightShader.use();
    lightShader.setInt("uFramebufferSRGB",
                       glIsEnabled(GL_FRAMEBUFFER_SRGB) == GL_TRUE ? 1 : 0);
    camera.setViewProjection(lightShader, "camMatrix");
    lightShader.setVec4("lightColor", light.color.x, light.color.y, light.color.z, 1.0f);
    lightShader.setMat4("model", model);

    lightMarker.draw();
}

void Renderer::drawDirectionalLightGizmo(const Light &light, Camera &camera)
{
    glm::vec3 forward = light.direction();
    const float directionLength = glm::length(forward);
    if (!finite(forward) || !std::isfinite(directionLength) ||
        directionLength <= 1e-5f)
        return;
    forward /= directionLength;

    // Constrói uma base ortonormal: o eixo local +Z acompanha os raios.
    const glm::vec3 referenceUp =
        std::fabs(glm::dot(forward, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.96f
            ? glm::vec3(0.0f, 0.0f, 1.0f)
            : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 right = glm::normalize(glm::cross(referenceUp, forward));
    const glm::vec3 up = glm::normalize(glm::cross(forward, right));

    glm::mat4 model(1.0f);
    model[0] = glm::vec4(right, 0.0f);
    model[1] = glm::vec4(up, 0.0f);
    model[2] = glm::vec4(forward, 0.0f);
    model[3] = glm::vec4(light.position, 1.0f);

    lightShader.use();
    lightShader.setInt("uFramebufferSRGB",
                       glIsEnabled(GL_FRAMEBUFFER_SRGB) == GL_TRUE ? 1 : 0);
    camera.setViewProjection(lightShader, "camMatrix");
    // Cor dourada mantém o gizmo legível mesmo quando a luz é branca ou muito fria.
    lightShader.setVec4("lightColor", 1.0f, 0.72f, 0.20f, 1.0f);
    lightShader.setMat4("model", model);

    GLfloat previousLineWidth = 1.0f;
    GLfloat supportedLineWidths[2] = {1.0f, 1.0f};
    glGetFloatv(GL_LINE_WIDTH, &previousLineWidth);
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, supportedLineWidths);
    const GLfloat requestedLineWidth = std::clamp(2.0f,
        supportedLineWidths[0], supportedLineWidths[1]);
    glLineWidth(requestedLineWidth);
    directionalLightGizmo.draw();
    glLineWidth(previousLineWidth);
}

void Renderer::render(const Scene &scene, const Light &light, Camera &camera)
{
    glm::mat4 lightSpaceMatrix(1.0f);
    const bool useDirectionalShadows =
        shadowsEnabled && shadowMapReady && light.type == LightType::Directional &&
        buildDirectionalLightSpaceMatrix(scene, light, lightSpaceMatrix);

    if (useDirectionalShadows)
        renderShadowMap(scene, lightSpaceMatrix);

    GLint currentViewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, currentViewport);
    const bool useAmbientOcclusion = ambientOcclusionEnabled && ssaoReady &&
        renderSsao(scene, camera, currentViewport[2], currentViewport[3]);

    // glClear não passa pela conversão automática de GL_FRAMEBUFFER_SRGB.
    // As cores de fundo já estão definidas nos valores sRGB desejados.
    if (rainScenario.enabled)
        glClearColor(0.035f, 0.055f, 0.075f, 1.0f);
    else
        glClearColor(0.1f, 0.2f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const bool framebufferSRGB = glIsEnabled(GL_FRAMEBUFFER_SRGB) == GL_TRUE;

    sceneShader.use();
    sceneShader.setInt("uFramebufferSRGB", framebufferSRGB ? 1 : 0);
    sceneShader.setFloat("ambientStrength", ambientStrength);
    sceneShader.setFloat("environmentReflectionStrength", environmentReflectionStrength);
    sceneShader.setInt("studioLightingEnabled", studioLightingEnabled ? 1 : 0);
    sceneShader.setInt("environmentDiffuseEnabled", environmentDiffuseEnabled ? 1 : 0);
    sceneShader.setInt("toneMappingEnabled", toneMappingEnabled ? 1 : 0);
    sceneShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
    sceneShader.setInt("shadowsEnabled", useDirectionalShadows ? 1 : 0);

    // O mapa de sombras ocupa uma unidade própria, sem conflitar com os materiais.
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, useDirectionalShadows ? shadowDepthTexture : 0);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, useAmbientOcclusion ? ssaoBlurTexture : 0);
    glActiveTexture(GL_TEXTURE0);

    sceneShader.setInt("ambientOcclusionMap", 7);
    sceneShader.setInt("ambientOcclusionEnabled", useAmbientOcclusion ? 1 : 0);
    sceneShader.setFloat("ambientOcclusionStrength", ambientOcclusionStrength);
    sceneShader.setVec2(
        "screenSize",
        static_cast<float>(std::max(1, currentViewport[2])),
        static_cast<float>(std::max(1, currentViewport[3])));

    applyLight(light, camera);
    camera.setViewProjection(sceneShader, "camMatrix");

    // Ambiente para reflexões dos materiais.
    sceneShader.setInt("environmentMap", 5);
    sceneShader.setInt("hasEnvironmentMap",
                       skybox.bindEnvironment(5) ? 1 : 0);

    drawScene(scene, camera.viewProjectionMatrix());

    if (showLightMarker)
    {
        if (light.type == LightType::Directional)
            drawDirectionalLightGizmo(light, camera);
        else
            drawLightMarker(light, camera);
    }

    if (showSkybox)
        skybox.draw(camera, framebufferSRGB);

    rainRenderer.render(rainScenario, camera, framebufferSRGB);
}

void Renderer::drawScene(
    const Scene &scene,
    const glm::mat4 &viewProjection)
{
    FrustumPlanes frustumPlanes{};
    const bool canCull = extractFrustumPlanes(viewProjection, frustumPlanes);

    for (const Scene::Object &object : scene.getObjects())
    {
        // Não desenha objetos ocultos.
        if (!object.visible)
            continue;

        const glm::mat4 matrix = object.transform.matrix();
        const BoundingBox* bounds = object.boundingBox();
        if (canCull && bounds &&
            !isInsideFrustum(*bounds, matrix, frustumPlanes))
            continue;

        if (object.model)
        {
            // O modelo envia a matriz de cada parte dele,
            // já combinada com a transformação do objeto.
            object.model->draw(sceneShader, defaultTexture, matrix);
        }
        else if (object.mesh)
        {
            setFrontFaceForTransform(matrix);
            sceneShader.setMat4("model", matrix);
            object.mesh->draw(sceneShader, &defaultTexture);
        }
        glFrontFace(GL_CCW);
    }
}
