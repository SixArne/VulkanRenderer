#include "VulkanDevice.h"

#include <stdexcept>
#include <vector>

#include "Utilities.h"

VulkanDevice::VulkanDevice(VkInstance instance)
	:m_Instance{instance}
{}

void VulkanDevice::Init()
{
	SetupPhysicalDevice();
	CreateLogicalDevice();
}

VkPhysicalDevice VulkanDevice::GetPhysicalDevice()
{
	return m_MainDevice.physicalDevice;
}

VkDevice VulkanDevice::GetLogicalDevice()
{
	return m_MainDevice.logicalDevice;
}

void VulkanDevice::CreateLogicalDevice()
{
	// Get the queue family indices for the chosen Physical device
	QueueFamilyIndices indices = GetQueueFamilies(m_MainDevice.physicalDevice);

	// Queues the logical device needs to create and info to do so (1 for now TODO: add more later)
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;	// Index of the family to create a queue from
	queueCreateInfo.queueCount = 1;

	constexpr float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;				// Vulkan needs to know how to handle multiple queues so it needs a priority (0 lowest 1 highest)

	// Information to create logical device (called device)
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;		// List of queue create info so device can create required queues
	deviceCreateInfo.enabledExtensionCount = 0;					// Number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;			// List of enabled logical device extensions

	// Physical device features that the logical device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;		// Physical device features for logical device

	// Create logical device for the given physical device
	const VkResult result = vkCreateDevice(m_MainDevice.physicalDevice, &deviceCreateInfo, nullptr, &m_MainDevice.logicalDevice);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Error creating logical device");
	}

	// Queues are created at the same time as the device
	// So we want to handle the queues
	// Fetch queue memory index from created logical device
	// TODO vkGetDeviceQueue(m_MainDevice.logicalDevice, indices.graphicsFamily, 0, &m_GraphicsQueue);
}

void VulkanDevice::SetupPhysicalDevice()
{
	// Enumerate physical devices the VkInstance can access
	uint32_t deviceCount{};
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

	// If no devices available, then none support vulkan
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPU's that support Vulkan Instance!");
	}

	// Populate device list
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, deviceList.data());

	// Get suitable device
	for (const auto& device : deviceList)
	{
		if (CheckDeviceSuitable(device))
		{
			m_MainDevice.physicalDevice = device;
			break;
		}
	}
}

bool VulkanDevice::CheckDeviceSuitable(VkPhysicalDevice device)
{
	/*

	// Information about device itself (ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines etc...)
	VkPhysicalDeviceFeatures deviceFeatures{};
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	*/

	QueueFamilyIndices indices = GetQueueFamilies(device);

	return indices.IsValid();
}
