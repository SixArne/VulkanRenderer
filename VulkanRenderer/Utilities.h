#pragma once

// Indices (locations) of Queue families (if they exist at all)
struct QueueFamilyIndices
{
	uint32_t graphicsFamily = -1; // Location of graphics queue family on GPU (-1 means non-existent)

	// Check if queue families are valid
	bool IsValid()
	{
		return graphicsFamily >= 0;
	}
};

static QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices{};

	// Get all Queue family property info of given device
	uint32_t queueFamilyCount{};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	// Go through each Queue family and check if it has at least 1 required queue
	uint32_t i{};
	for (const auto& queueFamily : queueFamilyList)
	{
		// Queue family can have 0 queue's, so we check for those and if the queue can be used as a graphic's queue.
		// These can be used as a bitmask
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i; // If queue family is valid, then get index
		}

		// Check if queue family indices are in valid state and stop looking.
		if (indices.IsValid())
		{
			break;
		}

		i++;
	}

	return indices;
}