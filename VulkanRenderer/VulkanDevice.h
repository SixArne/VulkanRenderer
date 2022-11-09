#pragma once
#include <vulkan/vulkan_core.h>

class VulkanDevice final
{
public:
	VulkanDevice(VkInstance instance);
	~VulkanDevice() = default;

	void Init();
	VkPhysicalDevice GetPhysicalDevice();
	VkDevice GetLogicalDevice();

private:
	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} m_MainDevice;

	const VkInstance m_Instance;

	void CreateLogicalDevice();
	void SetupPhysicalDevice();

	bool CheckDeviceSuitable(VkPhysicalDevice device);
};

