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
