#pragma once


#define GLFW_INCLUDE_VULKAN
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <vector>
#include <algorithm>
#include <array>

#include "stb_image.h"
#include "Utilities.h"
#include "Mesh.h"
#include "MeshModel.h"

class Window;

class VulkanRenderer final
{
public:
	VulkanRenderer() = default;
	~VulkanRenderer() = default;

	int Init(Window* window);
	void Update(float deltaTime);
	void UpdateModel(int modelId, glm::mat4 newModel);
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
	VkSampler m_TextureSampler{};

	// These 3 will ALWAYS use the same index.
	// So getting a command at index 0 will get the frame buffer at index 0 and the swapchain at index 0
	std::vector<SwapchainImage> m_SwapchainImages{};
	std::vector<VkFramebuffer> m_SwapchainFramebuffers{};
	std::vector<VkCommandBuffer> m_CommandBuffers{};

	// Depth stencil
	VkImage m_DepthBufferImage{};
	VkDeviceMemory m_DepthBufferImageMemory{};
	VkImageView m_DepthBufferImageView{};
	VkFormat m_DepthFormat{};

	// - Descriptor
	VkDescriptorSetLayout m_DescriptorSetLayout{};
	VkDescriptorSetLayout m_SamplerSetLayout{};

	VkPushConstantRange m_PushConstantRange{};

	VkDescriptorPool m_DescriptorPool{};
	VkDescriptorPool m_SamplerDescriptorPool{};
	std::vector<VkDescriptorSet> m_DescriptorSets{};
	std::vector<VkDescriptorSet> m_SamplerDescriptorSets{};

	std::vector<VkBuffer> m_VPUniformBuffer{};
	std::vector<VkDeviceMemory> m_VPUniformBufferMemory{};

	std::vector<VkBuffer> m_ModelDynamicUniformBuffer{};
	std::vector<VkDeviceMemory> m_ModelDynamicUniformBufferMemory{};


	// VkDeviceSize m_MinUniformBufferOffset{};
	// size_t m_ModelUniformAllignment{};
	// Model* m_ModelTransferSpace{};

	// - Assets
	std::vector<VkImage> m_TextureImages{};
	std::vector<VkDeviceMemory> m_TextureImageMemory{};
	std::vector<VkImageView> m_TextureImageViews{};
	std::vector<MeshModel> m_ModelList{};

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
	void CreatePushConstantRange();
	void CreateGraphicsPipeline();
	void CreateDepthBufferImage();
	void CreateFrameBuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateSynchronization();
	void CreateTextureSampler();
	
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void UpdateUniformBuffers(uint32_t imageIndex);

	// - Record functions
	void RecordCommands(uint32_t currentImage);

	// - Allocate functions
	//void AllocateDynamicBufferTransferSpace();
	
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
	VkFormat ChooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	// -- Populate functions
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo);

	// -- Getter functions
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
	SwapchainDetails GetSwapChainDetails(VkPhysicalDevice device);

	// - Create functions
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule CreateShaderModule(const std::vector<char>& code);
	VkImage CreateImage(
		uint32_t width, 
		uint32_t height, 
		VkFormat format, 
		VkImageTiling tiling, 
		VkImageUsageFlags useFlags, 
		VkMemoryPropertyFlags propFlags,
		VkDeviceMemory *imageMemory
	);

	int CreateTextureImage(std::string filename);
	int CreateTexture(std::string filename);
	int CreateTextureDescriptor(VkImageView textureImage);

	void CreateMeshModel(std::string modelFile);

	// -- Loader functions
	stbi_uc* LoadTextureFile(std::string& filename, int* width, int* height, VkDeviceSize* imageSize);

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

