#include "model.h"
#include "model_loader.h"

#include "engine/graphics/resources/shader.h"
#include "engine/graphics/resources/texture.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

#include <glm/gtc/matrix_inverse.hpp>

Model::Model(const char *path)
{
    if (path == nullptr)
        throw std::invalid_argument("Model path cannot be null.");

    ModelLoadResult loaded = ModelLoader::load(path);

    meshes = std::move(loaded.meshes);
    matricesMeshes = std::move(loaded.matricesMeshes);
    boundingRadius = loaded.boundingRadius;
    boundingBox = std::move(loaded.boundingBox);
}

void Model::draw(Shader &shader, const Texture &diffuseFallback,
				 const glm::mat4 &objectMatrix)
{
	// Um modelo pode conter várias partes; cada uma é desenhada na posição que o arquivo define para ela.
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		shader.setMat4("model", objectMatrix * matricesMeshes[i]);
		meshes[i].Mesh::draw(shader, &diffuseFallback);
	}
}

bool Model::intersectRay(
	const glm::vec3 &rayOrigin,
	const glm::vec3 &rayDirection,
	const glm::mat4 &objectMatrix,
	std::size_t &hitMeshIndex,
	unsigned int &hitFaceIndex,
	float &hitDistance) const
{
	if (meshes.empty() || matricesMeshes.size() != meshes.size())
		return false;

	const float directionLength = glm::length(rayDirection);

	if (directionLength < 0.000001f)
		return false;

	const glm::vec3 worldDirection =
		rayDirection / directionLength;

	float nearestDistance =
		std::numeric_limits<float>::max();

	bool foundIntersection = false;

	// Percorre todas as malhas do modelo.
	for (std::size_t i = 0; i < meshes.size(); ++i)
	{
		// Combina a transformação do objeto com a transformação interna.
		const glm::mat4 meshMatrix =
			objectMatrix * matricesMeshes[i];

		const float determinant = glm::determinant(meshMatrix);

		if (std::abs(determinant) < 0.000001f)
			continue;

		const glm::mat4 inverseMatrix = glm::inverse(meshMatrix);

		// Converte o raio do espaço global para o espaço local da malha.
		const glm::vec3 localOrigin = glm::vec3(
			inverseMatrix * glm::vec4(rayOrigin, 1.0f));

		glm::vec3 localDirection = glm::vec3(
			inverseMatrix * glm::vec4(worldDirection, 0.0f));

		const float localDirectionLength =
			glm::length(localDirection);

		if (localDirectionLength < 0.000001f)
			continue;

		localDirection /= localDirectionLength;

		unsigned int faceIndex = 0;
		float localDistance = 0.0f;

		// Verifica a interseção com a malha atual.
		if (!meshes[i].getGeometry().intersectRay(
				localOrigin,
				localDirection,
				faceIndex,
				localDistance))
		{
			continue;
		}

		// Converte o ponto atingido novamente para o espaço global.
		const glm::vec3 localHitPosition =
			localOrigin + localDirection * localDistance;

		const glm::vec3 worldHitPosition = glm::vec3(
			meshMatrix * glm::vec4(localHitPosition, 1.0f));

		const float worldDistance = glm::dot(
			worldHitPosition - rayOrigin,
			worldDirection);

		if (worldDistance < 0.0f)
			continue;

		// Guarda a face mais próxima da câmera.
		if (worldDistance < nearestDistance)
		{
			nearestDistance = worldDistance;
			hitMeshIndex = i;
			hitFaceIndex = faceIndex;
			foundIntersection = true;
		}
	}

	if (!foundIntersection)
		return false;

	hitDistance = nearestDistance;

	return true;
}

// Recupera os vértices da face considerando as transformações.
bool Model::getFacePositions(
	std::size_t meshIndex,
	unsigned int faceIndex,
	const glm::mat4 &objectMatrix,
	glm::vec3 &a,
	glm::vec3 &b,
	glm::vec3 &c) const
{
	if (meshIndex >= meshes.size() ||
		meshIndex >= matricesMeshes.size())
		return false;

	// Recupera as posições locais da face.
	if (!meshes[meshIndex].getGeometry().getFacePositions(
			faceIndex, a, b, c))
		return false;

	// Combina a transformação do objeto com a da malha.
	const glm::mat4 finalMatrix =
		objectMatrix * matricesMeshes[meshIndex];

	// Converte os três vértices para o espaço global.
	a = glm::vec3(finalMatrix * glm::vec4(a, 1.0f));
	b = glm::vec3(finalMatrix * glm::vec4(b, 1.0f));
	c = glm::vec3(finalMatrix * glm::vec4(c, 1.0f));

	return true;
}

bool Model::beginExtrusion(
	std::size_t meshIndex,
	unsigned int faceIndex)
{
	if (meshIndex >= meshes.size())
		return false;

	return meshes[meshIndex].beginExtrusion(faceIndex);
}

bool Model::updateExtrusion(
	std::size_t meshIndex,
	float distance)
{
	if (meshIndex >= meshes.size())
		return false;

	return meshes[meshIndex].updateExtrusion(distance);
}

void Model::finishExtrusion(std::size_t meshIndex)
{
	if (meshIndex >= meshes.size())
		return;

	meshes[meshIndex].finishExtrusion();
}

void Model::cancelExtrusion(std::size_t meshIndex)
{
	if (meshIndex >= meshes.size())
		return;

	meshes[meshIndex].cancelExtrusion();
}

unsigned int Model::getExtrudedFaceIndex(
	std::size_t meshIndex) const
{
	if (meshIndex >= meshes.size())
		return std::numeric_limits<unsigned int>::max();

	return meshes[meshIndex].getGeometry().getExtrudedFaceIndex();
}
