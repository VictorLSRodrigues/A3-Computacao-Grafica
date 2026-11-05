#include"Model.h"
#include<json/json.h>
#include"Mesh.h"
#include"camera.h"
#include"texture.h"
#include"buffers/VAO.h"
#include <fstream>
#include <iterator>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
std::string get_file_contents(const char* path)
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

std::uint32_t read_u32_le(const std::string& bytes, std::size_t offset)
{
	if (offset > bytes.size() || bytes.size() - offset < 4)
		throw std::runtime_error("Invalid GLB header or chunk.");

	return static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset])) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 8) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 16) |
		(static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3])) << 24);
}
}

Model::Model(const char* file)
{
	if (file == nullptr)
		throw std::invalid_argument("Model path cannot be null.");

	Model::file = file;
	const std::string contents = get_file_contents(file);
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

	// Traverse all nodes
	traverseNode(0);
}

void Model::desenhar(Shader& shader, Camera& camera)
{
	// Go over all meshes and draw each one
	for (unsigned int i = 0; i < meshes.size(); i++)
	{
		meshes[i].Mesh::desenhar(shader);
	}
}

void Model::loadMesh(unsigned int indMesh)
{
	// Get all accessor indices
	unsigned int posAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["POSITION"];
	unsigned int normalAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["NORMAL"];
	unsigned int texAccInd = JSON["meshes"][indMesh]["primitives"][0]["attributes"]["TEXCOORD_0"];
	unsigned int indAccInd = JSON["meshes"][indMesh]["primitives"][0]["indices"];

	// Use accessor indices to get all vertices components
	std::vector<float> posVec = getFloats(JSON["accessors"][posAccInd]);
	std::vector<glm::vec3> positions = groupFloatsVec3(posVec);
	std::vector<float> normalVec = getFloats(JSON["accessors"][normalAccInd]);
	std::vector<glm::vec3> normals = groupFloatsVec3(normalVec);
	std::vector<float> texVec = getFloats(JSON["accessors"][texAccInd]);
	std::vector<glm::vec2> texUVs = groupFloatsVec2(texVec);

	// Combine all the vertex components and also get the indices and textures
	std::vector<Vertex> vertices = assembleVertices(positions, normals, texUVs);
	std::vector<GLuint> indices = getIndices(JSON["accessors"][indAccInd]);
	std::vector<Texture> textures = getTextures();

	Mesh& mesh = meshes.emplace_back();
	if (!mesh.criarEbo(vertices, indices))
		throw std::runtime_error("Failed to create model mesh.");

	for (Texture& texture : textures)
		mesh.adicionarTextura(std::move(texture));
}

void Model::traverseNode(unsigned int nextNode, glm::mat4 matrix)
{
	// Current node
	json node = JSON["nodes"][nextNode];

	// Get translation if it exists
	glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
	if (node.find("translation") != node.end())
	{
		float transValues[3];
		for (unsigned int i = 0; i < node["translation"].size(); i++)
			transValues[i] = (node["translation"][i]);
		translation = glm::make_vec3(transValues);
	}
	// Get quaternion if it exists
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	if (node.find("rotation") != node.end())
	{
		float rotValues[4] =
		{
			node["rotation"][3],
			node["rotation"][0],
			node["rotation"][1],
			node["rotation"][2]
		};
		rotation = glm::make_quat(rotValues);
	}
	// Get scale if it exists
	glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
	if (node.find("scale") != node.end())
	{
		float scaleValues[3];
		for (unsigned int i = 0; i < node["scale"].size(); i++)
			scaleValues[i] = (node["scale"][i]);
		scale = glm::make_vec3(scaleValues);
	}
	// Get matrix if it exists
	glm::mat4 matNode = glm::mat4(1.0f);
	if (node.find("matrix") != node.end())
	{
		float matValues[16];
		for (unsigned int i = 0; i < node["matrix"].size(); i++)
			matValues[i] = (node["matrix"][i]);
		matNode = glm::make_mat4(matValues);
	}

	// Initialize matrices
	glm::mat4 trans = glm::mat4(1.0f);
	glm::mat4 rot = glm::mat4(1.0f);
	glm::mat4 sca = glm::mat4(1.0f);

	// Use translation, rotation, and scale to change the initialized matrices
	trans = glm::translate(trans, translation);
	rot = glm::mat4_cast(rotation);
	sca = glm::scale(sca, scale);

	// Multiply all matrices together
	glm::mat4 matNextNode = matrix * matNode * trans * rot * sca;

	// Check if the node contains a mesh and if it does load it
	if (node.find("mesh") != node.end())
	{
		translationsMeshes.push_back(translation);
		rotationsMeshes.push_back(rotation);
		scalesMeshes.push_back(scale);
		matricesMeshes.push_back(matNextNode);

		loadMesh(node["mesh"]);
	}

	// Check if the node has children, and if it does, apply this function to them with the matNextNode
	if (node.find("children") != node.end())
	{
		for (unsigned int i = 0; i < node["children"].size(); i++)
			traverseNode(node["children"][i], matNextNode);
	}
}

std::vector<unsigned char> Model::getData()
{
	// Create a place to store the raw text, and get the uri of the .bin file
	std::string bytesText;
	std::string uri = JSON["buffers"][0]["uri"];

	// Store raw text data into bytesText
	std::string fileStr = std::string(file);
	std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);
	bytesText = get_file_contents((fileDirectory + uri).c_str());

	// Transform the raw text data into bytes and put them in a vector
	std::vector<unsigned char> data(bytesText.begin(), bytesText.end());
	return data;
}

std::vector<float> Model::getFloats(json accessor)
{
	std::vector<float> floatVec;

	// Get properties from the accessor
	unsigned int buffViewInd = accessor.value("bufferView", 1);
	unsigned int count = accessor["count"];
	unsigned int accByteOffset = accessor.value("byteOffset", 0);
	std::string type = accessor["type"];

	// Get properties from the bufferView
	json bufferView = JSON["bufferViews"][buffViewInd];
	unsigned int byteOffset = bufferView["byteOffset"];

	// Interpret the type and store it into numPerVert
	unsigned int numPerVert;
	if (type == "SCALAR") numPerVert = 1;
	else if (type == "VEC2") numPerVert = 2;
	else if (type == "VEC3") numPerVert = 3;
	else if (type == "VEC4") numPerVert = 4;
	else throw std::invalid_argument("Type is invalid (not SCALAR, VEC2, VEC3, or VEC4)");

	// Go over all the bytes in the data at the correct place using the properties from above
	unsigned int beginningOfData = byteOffset + accByteOffset;
	unsigned int lengthOfData = count * 4 * numPerVert;
	for (unsigned int i = beginningOfData; i < beginningOfData + lengthOfData; i += 4)
	{
		unsigned char bytes[] = { data[i], data[i + 1], data[i + 2], data[i + 3] };
		float value;
		std::memcpy(&value, bytes, sizeof(float));
		floatVec.push_back(value);
	}

	return floatVec;
}

std::vector<GLuint> Model::getIndices(json accessor)
{
	std::vector<GLuint> indices;

	// Get properties from the accessor
	unsigned int buffViewInd = accessor.value("bufferView", 0);
	unsigned int count = accessor["count"];
	unsigned int accByteOffset = accessor.value("byteOffset", 0);
	unsigned int componentType = accessor["componentType"];

	// Get properties from the bufferView
	json bufferView = JSON["bufferViews"][buffViewInd];
	unsigned int byteOffset = bufferView["byteOffset"];

	// Get indices with regards to their type: unsigned int, unsigned short, or short
	unsigned int beginningOfData = byteOffset + accByteOffset;
	if (componentType == 5125)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 4; i += 4)
		{
			unsigned char bytes[] = { data[i], data[i + 1], data[i + 2], data[i + 3] };
			unsigned int value;
			std::memcpy(&value, bytes, sizeof(unsigned int));
			indices.push_back((GLuint)value);
		}
	}
	else if (componentType == 5123)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i += 2)
		{
			unsigned char bytes[] = { data[i], data[i + 1] };
			unsigned short value;
			std::memcpy(&value, bytes, sizeof(unsigned short));
			indices.push_back((GLuint)value);
		}
	}
	else if (componentType == 5122)
	{
		for (unsigned int i = beginningOfData; i < byteOffset + accByteOffset + count * 2; i += 2)
		{
			unsigned char bytes[] = { data[i], data[i + 1] };
			short value;
			std::memcpy(&value, bytes, sizeof(short));
			indices.push_back((GLuint)value);
		}
	}

	return indices;
}

std::vector<Texture> Model::getTextures()
{
	std::vector<Texture> textures;

	std::string fileStr = std::string(file);
	std::string fileDirectory = fileStr.substr(0, fileStr.find_last_of('/') + 1);

	// Go over all images
	for (unsigned int i = 0; i < JSON["images"].size(); i++)
	{
		const json& image = JSON["images"][i];
		const std::string texPath = image.value("uri", image.value("name", std::string()));

		Texture texture;
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
			const json& view = JSON["bufferViews"][viewIndex];
			const std::size_t offset = view.value("byteOffset", std::size_t(0));
			const std::size_t length = view["byteLength"].get<std::size_t>();
			if (offset > data.size() || length > data.size() - offset)
				throw std::runtime_error("GLB image data is outside the binary buffer.");
			loaded = texture.loadFromMemory(data.data() + offset, length);
		}

		if (!loaded)
			throw std::runtime_error("Failed to load model texture: " + texPath);
		texture.setType(texPath.find("metallicRoughness") != std::string::npos
			? "specular" : "diffuse");
		textures.push_back(std::move(texture));
	}

	return textures;
}

std::vector<Vertex> Model::assembleVertices
(
	std::vector<glm::vec3> positions,
	std::vector<glm::vec3> normals,
	std::vector<glm::vec2> texUVs
)
{
	std::vector<Vertex> vertices;
	for (int i = 0; i < positions.size(); i++)
	{
		vertices.push_back
		(
			Vertex
			{
				positions[i],
				glm::vec3(1.0f, 1.0f, 1.0f),
				texUVs[i],
				normals[i]
			}
		);
	}
	return vertices;
}

std::vector<glm::vec2> Model::groupFloatsVec2(std::vector<float> floatVec)
{
	const unsigned int floatsPerVector = 2;
	
	std::vector<glm::vec2> vectors;
	for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec2(0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = floatVec[i + j];
		}
	}
	return vectors;
}
std::vector<glm::vec3> Model::groupFloatsVec3(std::vector<float> floatVec)
{
	const unsigned int floatsPerVector = 3;

	std::vector<glm::vec3> vectors;
	for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec3(0, 0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = floatVec[i + j];
		}
	}
	return vectors;
}
std::vector<glm::vec4> Model::groupFloatsVec4(std::vector<float> floatVec)
{
	const unsigned int floatsPerVector = 4;

	std::vector<glm::vec4> vectors;
	for (unsigned int i = 0; i < floatVec.size(); i += floatsPerVector)
	{
		vectors.push_back(glm::vec4(0, 0, 0, 0));

		for (unsigned int j = 0; j < floatsPerVector; j++)
		{
			vectors.back()[j] = floatVec[i + j];
		}
	}
	return vectors;
}