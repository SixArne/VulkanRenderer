#include "MeshModel.h"
#include <string>

MeshModel::MeshModel(std::vector<Mesh> newMeshList)
	:m_MeshList{ newMeshList }, m_Model{glm::mat4(1.f)}
{
}

size_t MeshModel::GetMeshCount()
{
	return m_MeshList.size();
}

Mesh* MeshModel::GetMesh(size_t index)
{
	if (index >= 0 && index < m_MeshList.size())
	{
		return &m_MeshList[index];
	}
}

glm::mat4 MeshModel::GetModel()
{
	return m_Model;
}

void MeshModel::SetModel(glm::mat4 newModel)
{
	m_Model = newModel;
}

std::vector<std::string> MeshModel::LoadMaterials(const aiScene* scene)
{
	// Create 1:1 sized list of textures
	std::vector<std::string> textureList(scene->mNumMaterials);
	
	// Go through each material and copy its texture file name
	for (size_t i{}; i < scene->mNumMaterials; ++i)
	{
		// Get the material
		aiMaterial* material = scene->mMaterials[i];

		// Initialize texture to empty string (will be replaced if texture exists)
		textureList[i] = "";

		// Check for a diffuse texture (standard detail texture)
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			// Get path of texture file
			aiString path{};
			if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
			{
				// Cut off any dir information
				int idx = std::string( path.data ).rfind("\\");
				std::string filename = std::string(path.data).substr(idx + 1);

				textureList[i] = filename;
			}
		}
	}

	return textureList;
}

std::vector<Mesh> MeshModel::LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> mat2Tex)
{
	std::vector<Mesh> meshList{};

	// Go through each mesh at this node, create it and add to list
	for (size_t i{}; i < node->mNumMeshes; i++)
	{
		// Load mesh here
		meshList.push_back(LoadMesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, scene->mMeshes[node->mMeshes[i]], scene, mat2Tex));
	}
	
	// Go through each node attached to this node, load it and append their meshes to this node's mesh list.
	for (size_t i{}; i < node->mNumChildren; ++i)
	{
		std::vector<Mesh> newList = LoadNode(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, node->mChildren[i], scene, mat2Tex);
		meshList.insert(meshList.end(), newList.begin(), newList.end());
	}

	return meshList;
}

Mesh MeshModel::LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> mat2Tex)
{
	std::vector<Vertex> vertices{};
	std::vector<uint32_t> indices{};

	// Resize vertex to hold all vertices
	vertices.resize(mesh->mNumVertices);

	// Go through each vertex and copy
	for (size_t i{}; i < mesh->mNumVertices; ++i)
	{
		// Set position
		vertices[i].pos = {
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z
		};

		// Set Tex coords if exist
		if (mesh->mTextureCoords[0])
		{
			vertices[i].uv = {
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y,
			};
		}
		else
		{
			vertices[i].uv = {
				0.0f,
				0.0f
			};
		}

		// Set color
		vertices[i].col = { 1.f, 1.f, 1.f };
	}

	// Iterate over indices through faces and copy across
	for (size_t i{}; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];

		// Go through face's indices
		for (size_t j{}; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	// Create new mesh with details and return it
	Mesh newMesh = Mesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, &vertices, &indices, mat2Tex[mesh->mMaterialIndex]);

	return newMesh;
}

void MeshModel::DestroyMeshModel()
{
	for (auto& mesh : m_MeshList)
	{
		mesh.DestroyBuffers();
	}
}
