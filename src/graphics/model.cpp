#include "model.h"
#include <json/json.h>
#include "mesh.h"
#include "camera.h"
#include "texture.h"
#include "buffers/VAO.h"
#include <fstream>
#include <iterator>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <algorithm>
#include <iostream>
#include <glm/gtc/matrix_inverse.hpp>

namespace
{
	std::string get_file_contents(const char *path)
	{
		if (path == nullptr)
			throw std::invalid_argument("O caminho do arquivo não pode ser nulo.");

		std::ifstream file(path, std::ios::binary);
		if (!file)
			throw std::runtime_error("Could not open file: " + std::string(path));
		return std::string(
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>());
	}

	std::uint32_t read_u32_le(const std::string &bytes, std::size_t offset)
	{
		if (offset > bytes.size() || bytes.size() - offset < 4)
			throw std::runtime_error("Invalid GLB header or chunk.");

		return static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset])) |
			   (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 8) |
			   (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 16) |
			   (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3])) << 24);
	}
}

Model::Model(const char *path)
{
	if (path == nullptr)
		throw std::invalid_argument("Model path cannot be null.");

	Model::file = path;
	const std::string contents = get_file_contents(path);
	constexpr std::uint32_t glbMagic = 0x46546C67;
	constexpr std::uint32_t jsonChunkType = 0x4E4F534A;
	constexpr std::uint32_t binChunkType = 0x004E4942;

	if (contents.size() >= 12 && read_u32_le(contents, 0) == glbMagic)
	{
		const std::uint32_t version = read_u32_le(contents, 4);
		const std::uint32_t totalLength = read_u32_le(contents, 8);
		if (version != 2 || totalLength < 12 || totalLength > contents.size())
			throw std::runtime_error("Invalid or unsupported GLB file.");

		bool hasJson = false;
		std::size_t offset = 12;
		while (offset <= totalLength && totalLength - offset >= 8)
		{
			const std::uint32_t chunkLength = read_u32_le(contents, offset);
			const std::uint32_t chunkType = read_u32_le(contents, offset + 4);
			offset += 8;
			if (chunkLength > totalLength - offset)
				throw std::runtime_error("Invalid GLB chunk length.");

			if (chunkType == jsonChunkType && !hasJson)
			{
				std::string jsonText = contents.substr(offset, chunkLength);
				while (!jsonText.empty() &&
					   (jsonText.back() == '\0' || std::isspace(static_cast<unsigned char>(jsonText.back()))))
					jsonText.pop_back();
				JSON = json::parse(jsonText);
				hasJson = true;
			}
			else if (chunkType == binChunkType)
			{
				data.assign(contents.begin() + offset, contents.begin() + offset + chunkLength);
			}

			offset += chunkLength;
		}

		if (!hasJson)
			throw std::runtime_error("GLB file has no JSON chunk.");
		if (data.empty() && JSON.contains("buffers") && !JSON["buffers"].empty() &&
			JSON["buffers"][0].contains("uri"))
			data = getData();
	}
	else
	{
		JSON = json::parse(contents);
		data = getData();
	}

	// A cena do arquivo pode ter várias raízes; percorre cada uma delas.
	// Se o arquivo não descreve a cena, começa pelo primeiro nó.
	const unsigned int sceneIndex = JSON.value("scene", 0u);
	if (JSON.contains("scenes") && sceneIndex < JSON["scenes"].size() &&
		JSON["scenes"][sceneIndex].contains("nodes"))
	{
		for (const auto &root : JSON["scenes"][sceneIndex]["nodes"])
			traverseNode(root.get<unsigned int>());
	}
	else
	{
		traverseNode(0);
	}
}

void Model::draw(Shader &shader, Camera &camera, const Texture &diffuseFallback,
				 const glm::mat4 &objectMatrix)
{
	(void)camera;

	// Um modelo pode conter várias partes; cada uma é desenhada na posição que o arquivo define para ela.
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		shader.setMat4("model", objectMatrix * matricesMeshes[i]);
		meshes[i].Mesh::draw(shader, &diffuseFallback);
	}
}

bool Model::intersectRay(
    const glm::vec3& rayOrigin,
    const glm::vec3& rayDirection,
    const glm::mat4& objectMatrix,
    std::size_t& hitMeshIndex,
    unsigned int& hitFaceIndex,
    float& hitDistance) const
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
        if (!meshes[i].intersectRay(
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
    const glm::mat4& objectMatrix,
    glm::vec3& a,
    glm::vec3& b,
    glm::vec3& c) const
{
    if (meshIndex >= meshes.size() ||
        meshIndex >= matricesMeshes.size())
        return false;

    // Recupera as posições locais da face.
    if (!meshes[meshIndex].getFacePositions(
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

    return meshes[meshIndex].getExtrudedFaceIndex();
}


// Calcula normais suaves a partir dos triângulos, para modelos que não trazem normais.
static std::vector<glm::vec3> computeNormals(const std::vector<glm::vec3> &positions,
											 const std::vector<GLuint> &indices)
{
	std::vector<glm::vec3> normals(positions.size(), glm::vec3(0.0f));

	for (std::size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		const GLuint a = indices[i];
		const GLuint b = indices[i + 1];
		const GLuint c = indices[i + 2];
		if (a >= positions.size() || b >= positions.size() || c >= positions.size())
			continue;

		// Triângulos maiores pesam mais na normal de cada vértice.
		const glm::vec3 faceNormal = glm::cross(positions[b] - positions[a], positions[c] - positions[a]);
		normals[a] += faceNormal;
		normals[b] += faceNormal;
		normals[c] += faceNormal;
	}

	for (glm::vec3 &normal : normals)
		normal = glm::length(normal) > 0.0f ? glm::normalize(normal) : glm::vec3(0.0f, 1.0f, 0.0f);

	return normals;
}

void Model::loadMesh(unsigned int meshIndex, const glm::mat4 &matrix)
{
	// Uma malha do arquivo pode ter várias primitivas (por exemplo, uma por material);
	// cada uma vira uma Mesh própria, com a mesma matriz do nó.
	for (const json &primitive : JSON["meshes"][meshIndex]["primitives"])
	{
		// Só triângulos (modo 4, o padrão do formato).
		if (primitive.value("mode", 4) != 4)
			continue;

		const json &attributes = primitive["attributes"];
		if (!attributes.contains("POSITION"))
			continue;

		// A posição é obrigatória; os outros atributos são opcionais.
		std::vector<glm::vec3> positions =
			groupFloatsVec3(getFloats(JSON["accessors"][attributes["POSITION"].get<unsigned int>()]));

		// Calcula o raio considerando a transformação interna do modelo.
		for (const glm::vec3 &position : positions)
		{
			glm::vec3 transformedPosition =
				glm::vec3(matrix * glm::vec4(position, 1.0f));

			// transforma cada malha contribui para os limites gerais do modelo
			boundingBox.expand(transformedPosition);

			boundingRadius = std::max(
				boundingRadius,
				glm::length(transformedPosition));
		}

		// Sem índices no arquivo, os vértices já vêm em ordem: 0, 1, 2...
		std::vector<GLuint> indices;
		if (primitive.contains("indices"))
		{
			indices = getIndices(JSON["accessors"][primitive["indices"].get<unsigned int>()]);
		}
		else
		{
			indices.resize(positions.size());
			for (GLuint i = 0; i < indices.size(); i++)
				indices[i] = i;
		}

		// Sem UV, todos os vértices usam (0, 0).
		std::vector<glm::vec2> texUVs;
		if (attributes.contains("TEXCOORD_0"))
			texUVs = groupFloatsVec2(getFloats(JSON["accessors"][attributes["TEXCOORD_0"].get<unsigned int>()]));
		texUVs.resize(positions.size(), glm::vec2(0.0f));

		// Sem normais, calcula a partir dos triângulos.
		std::vector<glm::vec3> normals;
		if (attributes.contains("NORMAL"))
			normals = groupFloatsVec3(getFloats(JSON["accessors"][attributes["NORMAL"].get<unsigned int>()]));
		if (normals.size() != positions.size())
			normals = computeNormals(positions, indices);

		// Reúne os atributos em vértices completos e lê o material da primitiva.
		std::vector<Vertex> vertices = assembleVertices(positions, normals, texUVs);

		std::vector<Texture> textures;
		glm::vec4 baseColor(1.0f);
		loadMaterial(primitive, textures, baseColor);

		Mesh &mesh = meshes.emplace_back();
		if (!mesh.createIndexed(vertices, indices))
			throw std::runtime_error("Failed to create model mesh.");

		mesh.setBaseColor(baseColor);

		const std::string meshName =
			JSON["meshes"][meshIndex].value(
				"name",
				std::string("Mesh ") + std::to_string(meshIndex));

		std::cout
			<< "[Topologia]"
			<< " | Arquivo: " << file
			<< " | Malha: " << meshName
			<< " | Faces: " << mesh.getFaces().size()
			<< std::endl;

		// meshes e matricesMeshes andam juntas: a posição i de uma corresponde à posição i da outra.
		matricesMeshes.push_back(matrix);
	}
}

void Model::traverseNode(unsigned int nodeIndex, glm::mat4 matrix)
{
	// Cada nó descreve uma parte do modelo e pode ter filhos.
	json node = JSON["nodes"][nodeIndex];

	// Valores ausentes usam a transformação padrão: sem deslocamento, rotação ou escala.
	glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
	if (node.find("translation") != node.end())
	{
		float transValues[3];
		for (unsigned int i = 0; i < node["translation"].size(); i++)
			transValues[i] = (node["translation"][i]);
		translation = glm::make_vec3(transValues);
	}
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	if (node.find("rotation") != node.end())
	{
		float rotValues[4] =
			{
				node["rotation"][3],
				node["rotation"][0],
				node["rotation"][1],
				node["rotation"][2]};
		rotation = glm::make_quat(rotValues);
	}
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
	if (node.find("scale") != node.end())
	{
		float scaleValues[3];
		for (unsigned int i = 0; i < node["scale"].size(); i++)
			scaleValues[i] = (node["scale"][i]);
		scale = glm::make_vec3(scaleValues);
	}
	glm::mat4 matNode = glm::mat4(1.0f);
	if (node.find("matrix") != node.end())
	{
		float matValues[16];
		for (unsigned int i = 0; i < node["matrix"].size(); i++)
			matValues[i] = (node["matrix"][i]);
		matNode = glm::make_mat4(matValues);
	}

	// O formato permite uma matriz pronta ou transformação por posição, rotação e escala.
	glm::mat4 trans = glm::mat4(1.0f);
	glm::mat4 rot = glm::mat4(1.0f);
	glm::mat4 sca = glm::mat4(1.0f);

	trans = glm::translate(trans, translation);
	rot = glm::mat4_cast(rotation);
	sca = glm::scale(sca, scale);

	// Combina a transformação herdada com a deste nó.
	glm::mat4 matNextNode = matrix * matNode * trans * rot * sca;

	if (node.find("mesh") != node.end())
	{
		loadMesh(node["mesh"], matNextNode);
	}

	// Os filhos recebem a transformação acumulada até este ponto.
	if (node.find("children") != node.end())
	{
		for (unsigned int i = 0; i < node["children"].size(); i++)
			traverseNode(node["children"][i], matNextNode);
	}
}

std::vector<unsigned char> Model::getData()
{
	// O arquivo .gltf pode apontar para um arquivo binário separado.
	std::string bytesText;
	std::string uri = JSON["buffers"][0]["uri"];

	std::string fileStr = std::string(file);
	std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);
	bytesText = get_file_contents((fileDirectory + uri).c_str());

	// Mantém os bytes em memória para que os atributos possam ser extraídos.
	std::vector<unsigned char> data(bytesText.begin(), bytesText.end());
	return data;
}

std::vector<float> Model::getFloats(json accessor)
{
	std::vector<float> floatVec;

	// A referência descreve quantos valores ler, seu tipo e seu deslocamento.
	unsigned int buffViewInd = accessor.value("bufferView", 1);
	unsigned int count = accessor["count"];
	unsigned int accByteOffset = accessor.value("byteOffset", 0);
	std::string type = accessor["type"];

	// A seção indicada informa onde os valores começam no bloco binário.
	json bufferView = JSON["bufferViews"][buffViewInd];
	unsigned int byteOffset = bufferView.value("byteOffset", 0u);

	// O tipo define quantos valores formam cada elemento.
	unsigned int numPerVert;
	if (type == "SCALAR")
		numPerVert = 1;
	else if (type == "VEC2")
		numPerVert = 2;
	else if (type == "VEC3")
		numPerVert = 3;
	else if (type == "VEC4")
		numPerVert = 4;
	else
		throw std::invalid_argument("Type is invalid (not SCALAR, VEC2, VEC3, or VEC4)");

	// Converte cada grupo de quatro bytes em um número de ponto flutuante.
	unsigned int beginningOfData = byteOffset + accByteOffset;
	unsigned int lengthOfData = count * 4 * numPerVert;
	for (unsigned int i = beginningOfData; i < beginningOfData + lengthOfData; i += 4)
	{
		unsigned char bytes[] = {data[i], data[i + 1], data[i + 2], data[i + 3]};
		float value;
		std::memcpy(&value, bytes, sizeof(float));
		floatVec.push_back(value);
	}

	return floatVec;
}

std::vector<GLuint> Model::getIndices(json accessor)
{
	std::vector<GLuint> indices;

	// A referência informa a quantidade, o tipo e o deslocamento dos índices.
	unsigned int buffViewInd = accessor.value("bufferView", 0);
	unsigned int count = accessor["count"];
	unsigned int accByteOffset = accessor.value("byteOffset", 0);
	unsigned int componentType = accessor["componentType"];

	json bufferView = JSON["bufferViews"][buffViewInd];
	unsigned int byteOffset = bufferView.value("byteOffset", 0u);

	// O arquivo pode armazenar cada índice em 2 ou 4 bytes.
	unsigned int beginningOfData = byteOffset + accByteOffset;
	if (componentType == 5125)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 4; i += 4)
		{
			unsigned char bytes[] = {data[i], data[i + 1], data[i + 2], data[i + 3]};
			unsigned int value;
			std::memcpy(&value, bytes, sizeof(unsigned int));
			indices.push_back((GLuint)value);
		}
	}
	else if (componentType == 5123)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i += 2)
		{
			unsigned char bytes[] = {data[i], data[i + 1]};
			unsigned short value;
			std::memcpy(&value, bytes, sizeof(unsigned short));
			indices.push_back((GLuint)value);
		}
	}
	else if (componentType == 5122)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i += 2)
		{
			unsigned char bytes[] = {data[i], data[i + 1]};
			short value;
			std::memcpy(&value, bytes, sizeof(short));
			indices.push_back((GLuint)value);
		}
	}

	return indices;
}

void Model::loadMaterial(const json &primitive, std::vector<Texture> &textures, glm::vec4 &baseColor)
{
	// Sem material, a parte usa a textura padrão da aplicação e cor neutra.
	if (!primitive.contains("material"))
		return;

	const json &material = JSON["materials"][primitive["material"].get<unsigned int>()];
	const json pbr = material.value("pbrMetallicRoughness", json::object());

	// A cor base multiplica a textura; sem textura, é a própria cor da parte.
	if (pbr.contains("baseColorFactor"))
	{
		for (unsigned int i = 0; i < 4; i++)
			baseColor[i] = pbr["baseColorFactor"][i].get<float>();
	}

	Texture colorTexture;
	if (pbr.contains("baseColorTexture") && loadMaterialTexture(pbr["baseColorTexture"], colorTexture))
	{
		colorTexture.setType("diffuse");
		textures.push_back(std::move(colorTexture));
	}
	else
	{
		// Sem imagem: usa um pixel branco para a cor do material aparecer pura,
		// sem ser tingida pela textura padrão.
		const unsigned char white[4] = {255, 255, 255, 255};
		Texture solid;
		if (solid.create(1, 1, 4, white))
		{
			solid.setType("diffuse");
			textures.push_back(std::move(solid));
		}
	}

	Texture shineTexture;
	if (pbr.contains("metallicRoughnessTexture") && loadMaterialTexture(pbr["metallicRoughnessTexture"], shineTexture))
	{
		shineTexture.setType("specular");
		textures.push_back(std::move(shineTexture));
	}
}

bool Model::loadMaterialTexture(const json &textureInfo, Texture &texture)
{
	// O material aponta para uma textura, e a textura aponta para uma imagem.
	const unsigned int textureIndex = textureInfo["index"].get<unsigned int>();
	const json &textureDesc = JSON["textures"][textureIndex];

	if (!textureDesc.contains("source"))
		return false;

	loadImage(textureDesc["source"].get<unsigned int>(), texture);
	return true;
}

void Model::loadImage(unsigned int imageIndex, Texture &texture)
{
	std::string fileStr = std::string(file);
	std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);

	// A imagem pode estar em arquivo separado ou dentro do próprio GLB.
	const json &image = JSON["images"][imageIndex];
	const std::string texPath = image.value("uri", image.value("name", std::string()));

	bool loaded = false;
	if (image.contains("uri"))
	{
		if (texPath.rfind("data:", 0) == 0)
			throw std::runtime_error("Data URI textures are not supported.");
		loaded = texture.load(fileDirectory + texPath);
	}
	else if (image.contains("bufferView"))
	{
		const unsigned int viewIndex = image["bufferView"].get<unsigned int>();
		const json &view = JSON["bufferViews"][viewIndex];
		const std::size_t offset = view.value("byteOffset", std::size_t(0));
		const std::size_t length = view["byteLength"].get<std::size_t>();
		if (offset > data.size() || length > data.size() - offset)
			throw std::runtime_error("GLB image data is outside the binary buffer.");
		loaded = texture.loadFromMemory(data.data() + offset, length);
	}

	if (!loaded)
		throw std::runtime_error("Failed to load model texture: " + texPath);
}

std::vector<Vertex> Model::assembleVertices(
	std::vector<glm::vec3> positions,
	std::vector<glm::vec3> normals,
	std::vector<glm::vec2> texUVs)
{
	std::vector<Vertex> vertices;
	for (int i = 0; i < positions.size(); i++)
	{
		vertices.push_back(
			Vertex{
				positions[i],
				glm::vec3(1.0f, 1.0f, 1.0f),
				texUVs[i],
				normals[i]});
	}
	return vertices;
}

std::vector<glm::vec2> Model::groupFloatsVec2(std::vector<float> values)
{
	const unsigned int floatsPerVector = 2;

	std::vector<glm::vec2> vectors;
	for (unsigned int i = 0; i < values.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec2(0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = values[i + j];
		}
	}
	return vectors;
}
std::vector<glm::vec3> Model::groupFloatsVec3(std::vector<float> values)
{
	const unsigned int floatsPerVector = 3;

	std::vector<glm::vec3> vectors;
	for (unsigned int i = 0; i < values.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec3(0, 0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = values[i + j];
		}
	}
	return vectors;
}
std::vector<glm::vec4> Model::groupFloatsVec4(std::vector<float> values)
{
	const unsigned int floatsPerVector = 4;

	std::vector<glm::vec4> vectors;
	for (unsigned int i = 0; i < values.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec4(0, 0, 0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = values[i + j];
		}
	}
	return vectors;
}