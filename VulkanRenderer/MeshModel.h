#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <assimp/scene.h>

#include "Mesh.h"

class MeshModel
{
public:
	MeshModel() = default;
	~MeshModel() = default;

	MeshModel(std::vector<Mesh> newMeshList);

	size_t GetMeshCount();
	Mesh* GetMesh(size_t index);

	glm::mat4 GetModel();
	void SetModel(glm::mat4 newModel);

	static std::vector<std::string> LoadMaterials(const aiScene* scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
		aiNode* node, const aiScene* scene, std::vector<int> mat2Tex);
	static Mesh LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool,
		aiMesh* mesh, const aiScene* scene, std::vector<int> mat2Tex);

	void DestroyMeshModel();


private:
	std::vector<Mesh> m_MeshList{};
	glm::mat4 m_Model;
};
