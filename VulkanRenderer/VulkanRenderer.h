#pragma once

#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>
#include <vector>

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

	const std::vector<const char*> m_ValidationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};
	bool m_EnableValidationLayers{ false };


	// Vulkan functions
	// - Create functions
	void CreateInstance();
	void CreateLogicalDevice();
	void CreateDebugMessenger();
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

	// - Destroy functions
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

	// - Get functions
	VkDevice GetPhysicalDevice();
	std::vector<const char*> GetRequiredExtensions();

	// - Support functions
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool CheckDeviceSuitable(VkPhysicalDevice device);
	bool CheckValidationEnabled();
	bool CheckValidationLayerSupport();

	// - Checker functions


	// - Populate functions
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	// - Getter functions
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

