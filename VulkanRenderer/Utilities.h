#pragma once

const std::vector<const char*> g_DeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of Queue families (if they exist at all)
struct QueueFamilyIndices
{
	int32_t graphicsFamily = -1;			// Location of graphics queue family on GPU (-1 means non-existent)
	int32_t presentationFamily = -1;		// Location of presentation queue family

	// Check if queue families are valid
	bool IsValid() const
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapchainDetails
{
	VkSurfaceCapabilitiesKHR capabilities{};				// Surface properties e.g. image size/extent
	std::vector<VkSurfaceFormatKHR> formats{};				// Surface image formats e.g. RGBA and size of each color
	std::vector<VkPresentModeKHR> presentationModes{};		// Surface presentationModes: e.g. Mailbox
};

struct SwapchainImage
{
	VkImage image;
	VkImageView imageView;
};