#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "Utilities.h"

struct UboModel
{
	glm::mat4 model{};
};

class Mesh
{
public:
	Mesh() = default;
	Mesh(
		VkPhysicalDevice newPhysicalDevice, 
		VkDevice newDevice, 
		VkQueue transferQueue, 
		VkCommandPool transferCommandPool, 
		std::vector<Vertex>* vertices,
		std::vector<uint32_t>* indices
	);

	void SetModel(glm::mat4 newModel);
	UboModel GetModel();

	uint32_t GetVertexCount();
	uint32_t GetIndexCount();
	VkBuffer GetVertexBuffer();
	VkBuffer GetIndexBuffer();

	void DestroyBuffers();

private:
	UboModel m_UboModel{};

	int32_t m_VertexCount{};
	VkBuffer m_VertexBuffer{};
	VkDeviceMemory m_VertexBufferMemory{};

	uint32_t m_IndexCount{};
	VkBuffer m_IndexBuffer{};
	VkDeviceMemory m_IndexBufferMemory{};

	VkPhysicalDevice m_PhysicalDevice{};
	VkDevice m_Device{};

	void CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
	void CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
};

