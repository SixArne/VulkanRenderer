#include "Mesh.h"

Mesh::Mesh(
	VkPhysicalDevice newPhysicalDevice, 
	VkDevice newDevice, 
	VkQueue transferQueue, 
	VkCommandPool transferCommandPool, 
	std::vector<Vertex>* vertices, 
	std::vector<uint32_t>* indices
)
{
	m_VertexCount = (int32_t)vertices->size();
	m_PhysicalDevice = newPhysicalDevice;
	m_IndexCount = indices->size();
	m_Device = newDevice;
	CreateVertexBuffer(transferQueue, transferCommandPool, vertices);
	CreateIndexBuffer(transferQueue, transferCommandPool, indices);
}

uint32_t Mesh::GetVertexCount()
{
	return m_VertexCount;
}

VkBuffer Mesh::GetVertexBuffer()
{
	return m_VertexBuffer;
}

uint32_t Mesh::GetIndexCount()
{
	return m_IndexCount;
}

VkBuffer Mesh::GetIndexBuffer()
{
	return m_IndexBuffer;
}

void Mesh::DestroyBuffers()
{
	// Destroy buffer
	vkDestroyBuffer(m_Device, m_VertexBuffer, nullptr);
	vkDestroyBuffer(m_Device, m_IndexBuffer, nullptr);

	// Free memory
	vkFreeMemory(m_Device, m_VertexBufferMemory, nullptr);
	vkFreeMemory(m_Device, m_IndexBufferMemory, nullptr);
}

void Mesh::CreateVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
	/************************************************************************/
	// We can't copy data directly on the GPU, we can only directly place
	// it through the CPU, which means it will use non-optimized
	// memory, so here we make the buffer on CPU and copy it over
	// and then copy the CPU buffer to a dedicated GPU buffer.
	/************************************************************************/

	const VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size(); // Get size of buffer needed for vertices

	// Temporary buffer to stage vertex data before transferring to GPU
	VkBuffer stagingBuffer{};
	VkDeviceMemory stagingBufferMemory{};

	// Create staging buffer and allocate memory to it
	CreateBuffer(
		m_PhysicalDevice,
		m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory
	);

	// MAP MEMORY TO VERTEX BUFFER
	void * data;																								// 1: Create pointer to a point in normal memory
	vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);			// 2: Map vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize);										// 3: Copy vertices to point
	vkUnmapMemory(m_Device, stagingBufferMemory);																// 4: Unmap the vertex buffer memory


	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
	// Buffer memory is to be device local bit which means memory is on GPU and only accessible to it and not the CPU (host)
	CreateBuffer(
		m_PhysicalDevice,
		m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&m_VertexBuffer,
		&m_VertexBufferMemory
	);

	// Copy staging buffer to vertex buffer on GPU
	CopyBuffer(m_Device, transferQueue, transferCommandPool, stagingBuffer, m_VertexBuffer, bufferSize);

	// Destroy staging buffer
	vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
	vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
}

void Mesh::CreateIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
	// Get size of buffer needed for indices
	const VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// Temporary buffer to stage vertex data before transferring to GPU
	VkBuffer stagingBuffer{};
	VkDeviceMemory stagingBufferMemory{};

	// Create staging buffer and allocate memory to it
	CreateBuffer(
		m_PhysicalDevice,
		m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer,
		&stagingBufferMemory
	);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;																						
	vkMapMemory(m_Device, stagingBufferMemory, 0, bufferSize, 0, &data);			
	memcpy(data, indices->data(), (size_t)bufferSize);								
	vkUnmapMemory(m_Device, stagingBufferMemory);

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
	// Buffer memory is to be device local bit which means memory is on GPU and only accessible to it and not the CPU (host)
	CreateBuffer(
		m_PhysicalDevice,
		m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&m_IndexBuffer,
		&m_IndexBufferMemory
	);

	// Copy staging buffer to vertex buffer on GPU
	CopyBuffer(m_Device, transferQueue, transferCommandPool, stagingBuffer, m_IndexBuffer, bufferSize);

	// Destroy staging buffer
	vkDestroyBuffer(m_Device, stagingBuffer, nullptr);
	vkFreeMemory(m_Device, stagingBufferMemory, nullptr);
}
