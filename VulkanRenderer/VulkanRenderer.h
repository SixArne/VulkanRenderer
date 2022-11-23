#pragma once


#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>
#include <vector>
#include <algorithm>

#include "Utilities.h"

class Window;

class VulkanRenderer final
{
public:
	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	int Init(Window* window);
	void Cleanup();

private:
	Window* m_pWindow;

	// Vulkan components
	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;

	struct
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} m_MainDevice;
	VkQueue m_GraphicsQueue{};
	VkQueue m_PresentationQueue{};
	VkSurfaceKHR m_Surface{};
	VkSwapchainKHR m_Swapchain{};
	std::vector<SwapchainImage> m_SwapchainImages{};

	// - Utility
	VkFormat m_SwapchainImageFormat{};
	VkExtent2D m_SwapchainExtent{};

	const std::vector<const char*> m_ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	bool m_EnableValidationLayers{ false };


	// Vulkan functions
	// - Create functions
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateSurface();
	void CreateDebugMessenger();
	void CreateSwapchain();
	void CreateGraphicsPipeline();
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	// - Destroy functions
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	// - Get functions
	void GetPhysicalDevice();
	std::vector<const char*> GetRequiredExtensions();

	// - Support functions
	bool CheckValidationEnabled();

	// -- Checker functions
	bool CheckValidationLayerSupport();
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool CheckDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);

	// -- Choose functions
	VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	// -- Populate functions
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	// -- Getter functions
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
	SwapchainDetails GetSwapChainDetails(VkPhysicalDevice device);

	// - Create functions
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	// Validation layer stuff
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

