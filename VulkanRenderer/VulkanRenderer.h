#pragma once


#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <algorithm>
#include <array>

#include "Utilities.h"
#include "Mesh.h"

class Window;

class VulkanRenderer final
{
public:
	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	int Init(Window* window);
	void Update(float deltaTime);
	void UpdateModel(glm::mat4 newModel);
	void Draw();
	void Cleanup();

private:
	glm::vec3 m_CameraPos{ 0,0,10 };
	glm::vec3 m_CameraFront{ 0,0,1 };
	glm::vec3 m_CameraUp{ 0,1,0 };
	float m_CameraYaw{-90.f};
	float m_CameraPitch{};

	Window* m_pWindow;

	uint32_t m_CurrentFrame{};

	// Scene objects
	std::vector<Mesh> m_MeshList{};

	// Scene settings
	struct UboViewProjection {
		glm::mat4 projection;
		glm::mat4 view;
	} m_UboViewProjection;

	// Vulkan components
	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	struct {
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} m_MainDevice;
	VkQueue m_GraphicsQueue{};
	VkQueue m_PresentationQueue{};
	VkSurfaceKHR m_Surface{};
	VkSwapchainKHR m_Swapchain{};

	// These 3 will ALWAYS use the same index.
	// So getting a command at index 0 will get the frame buffer at index 0 and the swapchain at index 0
	std::vector<SwapchainImage> m_SwapchainImages{};
	std::vector<VkFramebuffer> m_SwapchainFramebuffers{};
	std::vector<VkCommandBuffer> m_CommandBuffers{};

	// - Descriptor
	VkDescriptorSetLayout m_DescriptorSetLayout{};

	VkDescriptorPool m_DescriptorPool{};
	std::vector<VkDescriptorSet> m_DescriptorSets{};

	std::vector<VkBuffer> m_VPUniformBuffer{};
	std::vector<VkDeviceMemory> m_VPUniformBufferMemory{};

	std::vector<VkBuffer> m_ModelDynamicUniformBuffer{};
	std::vector<VkDeviceMemory> m_ModelDynamicUniformBufferMemory{};

	VkDeviceSize m_MinUniformBufferOffset{};
	size_t m_ModelUniformAllignment{};

	UboModel* m_ModelTransferSpace{};

	// - Pipeline
	VkPipeline m_GraphicsPipeline{};
	VkPipelineLayout m_PipelineLayout{};
	VkRenderPass m_RenderPass{};

	// - Pools
	VkCommandPool m_GraphicsCommandPool{};

	// - Utility
	VkFormat m_SwapchainImageFormat{};
	VkExtent2D m_SwapchainExtent{};

	// - Synchronization
	std::vector<VkSemaphore> m_ImagesAvailable{};
	std::vector<VkSemaphore> m_RendersFinished{};
	std::vector<VkFence> m_DrawFences{};

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
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronization();
	
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void UpdateUniformBuffers(uint32_t imageIndex);

	// - Record functions
	void RecordCommands();

	// - Allocate functions
	void AllocateDynamicBufferTransferSpace();
	
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
	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {

		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

