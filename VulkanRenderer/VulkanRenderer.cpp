#include "VulkanRenderer.h"

#include <set>
#include <stdexcept>

#include "Window.h"


int VulkanRenderer::Init(Window* window)
{
	m_pWindow = window;

	try {
		// Will set a debug bool if validation is needed
		if (CheckValidationEnabled() && !CheckValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}

		// Order is important
		CreateInstance();
		CreateDebugMessenger();
		CreateSurface();
		GetPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapchain();
		CreateGraphicsPipeline();
	}
	catch(const std::runtime_error &e) {
		printf("[ERROR]: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRenderer::Cleanup()
{
	for (const auto& image: m_SwapchainImages)
	{
		vkDestroyImageView(m_MainDevice.logicalDevice, image.imageView, nullptr);
	}

	vkDestroySwapchainKHR(m_MainDevice.logicalDevice, m_Swapchain, nullptr);
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr); // allocator param not needed.

	if (CheckValidationEnabled())
	{
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}

	// Order is important, instance should be last (I think)
	vkDestroyDevice(m_MainDevice.logicalDevice, nullptr);
	vkDestroyInstance(m_Instance, nullptr); // allocator param is custom de allocator func
}

void VulkanRenderer::CreateInstance()
{
	// Application information
	// Most stuff here is for developers only
	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = "Vulkan app";				// Custom name for application
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);	// Custom version of application
	applicationInfo.pEngineName = "No Engine";						// Custom engine name
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);		// Custom engine version
	applicationInfo.apiVersion = VK_API_VERSION_1_3;				// Vulkan API version (affects application)

	// Creation information for VkInstance
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &applicationInfo;					// Reference to application info above

	const auto instanceExtensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (CheckValidationEnabled())
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}

	// Create instance
	const VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);	// Allocator parameter is important! Look into it later

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan instance");
	}
}

void VulkanRenderer::CreateLogicalDevice()
{
	// Get the queue family indices for the chosen Physical device
	QueueFamilyIndices indices = GetQueueFamilies(m_MainDevice.physicalDevice);

	// Vector for queue create information
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
	// At this point the queue indices are set and the std::set assures no duplicate indices will be given
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// Queues the logical device needs to create and info to do so (1 for now TODO: add more later)
	for (const int queueFamilyIndex: queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;	// Index of the family to create a queue from
		queueCreateInfo.queueCount = 1;
		constexpr float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;				// Vulkan needs to know how to handle multiple queues so it needs a priority (0 lowest 1 highest)

		queueCreateInfos.push_back(queueCreateInfo);
	}
	

	// Information to create logical device (called device)
	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();														// List of queue create info so device can create required queues
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(g_DeviceExtensions.size());							// Number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = g_DeviceExtensions.data();												// List of enabled logical device extensions

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
	vkGetDeviceQueue(m_MainDevice.logicalDevice, indices.graphicsFamily, 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_MainDevice.logicalDevice, indices.presentationFamily, 0, &m_PresentationQueue);
}

void VulkanRenderer::CreateSurface()
{
	// Create surface (This creates a surface struct set up depending on your system)
	const VkResult result = glfwCreateWindowSurface(m_Instance, m_pWindow->GetWindow(), nullptr, &m_Surface);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Error creating window surface");
	}
}

void VulkanRenderer::CreateDebugMessenger()
{
	if (!CheckValidationEnabled())
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void VulkanRenderer::CreateSwapchain()
{
	// Get swapchain details so we can pick best settings.
	const SwapchainDetails swapChainDetails = GetSwapChainDetails(m_MainDevice.physicalDevice);

	// Choose best surface format
	const VkSurfaceFormatKHR format = ChooseBestSurfaceFormat(swapChainDetails.formats);

	// Choose best presentation mode
	const VkPresentModeKHR presentMode = ChooseBestPresentationMode(swapChainDetails.presentationModes);

	// Choose swapchain image resolutions
	const VkExtent2D extent = ChooseSwapExtent(swapChainDetails.capabilities);

	// How many images are in swapchain => get 1 more than min to allow triple buffering
	uint32_t imageCount = swapChainDetails.capabilities.minImageCount + 1;

	// Clamp values, if maxImageCount is negative then we have infinite images, so no need to clamp
	if (swapChainDetails.capabilities.maxImageCount > 0 && swapChainDetails.capabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.capabilities.maxImageCount;
	}

	// Create swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.presentMode = presentMode;										// Swapchain present mode
	swapchainCreateInfo.imageExtent = extent;											// Swapchain image size
	swapchainCreateInfo.imageColorSpace = format.colorSpace;							// Swapchain color space
	swapchainCreateInfo.imageFormat = format.format;									// Swapchain format
	swapchainCreateInfo.minImageCount = imageCount;										// Swapchain image count (buffering)
	swapchainCreateInfo.imageArrayLayers = 1;											// Number of layers for each image in chain
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;				// What attachment images will be used as
	swapchainCreateInfo.preTransform = swapChainDetails.capabilities.currentTransform;	// Transform to apply on swapchain images
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;				// Draw as it normally is: don't blend! (Window overlap)
	swapchainCreateInfo.clipped = VK_TRUE;												// Whether to clip part of image not in view (offscreen or behind other windows)
	swapchainCreateInfo.surface = m_Surface;											// Surface to build swapchain on

	// Get queue family indices
	const QueueFamilyIndices familyIndices = GetQueueFamilies(m_MainDevice.physicalDevice);

	// If graphics and presentation families are different then swapchain must let images be shared between families
	if (familyIndices.graphicsFamily != familyIndices.presentationFamily)
	{
		// Queues to share between
		const uint32_t queueFamilyIndices[] = {
			(uint32_t) familyIndices.graphicsFamily,
			(uint32_t) familyIndices.presentationFamily
		};

		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;	// Image share handling
		swapchainCreateInfo.queueFamilyIndexCount = 2;						// Number of queues to share images between
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;		// Array of queues to share between
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;	// Image share handling
		swapchainCreateInfo.queueFamilyIndexCount = 0;						// Number of queues to share images between
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;					// Array of queues to share between
	}

	// If you want to create a new swapchain you can pass on responsibilities (resize should destroy swapchain)
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// Create chain
	const VkResult result = vkCreateSwapchainKHR(m_MainDevice.logicalDevice, &swapchainCreateInfo, nullptr, &m_Swapchain);

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create swapchain");
	}

	// Store for later reference
	m_SwapchainImageFormat = format.format;
	m_SwapchainExtent = extent;

	// Get amount of images first
	uint32_t swapchainImageCount{};
	vkGetSwapchainImagesKHR(m_MainDevice.logicalDevice, m_Swapchain, &swapchainImageCount, nullptr);

	std::vector<VkImage> images(swapchainImageCount);
	vkGetSwapchainImagesKHR(m_MainDevice.logicalDevice, m_Swapchain, &swapchainImageCount, images.data());

	for (const VkImage image: images)
	{
		// VkImages is a identifier
		SwapchainImage swapchainImage{};
		swapchainImage.image = image;
		swapchainImage.imageView = CreateImageView(image, format.format, VK_IMAGE_ASPECT_COLOR_BIT);

		m_SwapchainImages.push_back(swapchainImage);
	}
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	// Read in SPIR-V code of shaders
	auto vertexShaderCode = readFile("Shaders/vert.spv");
	auto fragmentShaderCode = readFile("Shaders/frag.spv");

	// Build shader modules to link to graphics pipeline
	VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode);

	// -- SHADER STAGE CREATION INFORMATION ---
	// Vertex stage creation information
	VkPipelineShaderStageCreateInfo vertexShaderCreateInfo{};
	vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Shader stage type
	vertexShaderCreateInfo.module = vertexShaderModule; // Module to be used
	vertexShaderCreateInfo.pName = "main"; // function to run in shader file

	// Fragment stage creation information
	VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
	fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.module = fragmentShaderModule;
	fragmentShaderCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};

	// CREATE PIPELINE (once we create pipeline we can destroy here)
	vkDestroyShaderModule(m_MainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_MainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::GetPhysicalDevice()
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
	for (const auto& device: deviceList)
	{
		if (CheckDeviceSuitable(device))
		{
			m_MainDevice.physicalDevice = device;
			break;
		}
	}

	if (!m_MainDevice.physicalDevice)
	{
		throw std::runtime_error("Found GPU but it can't be used as a physical device due to lack of extensions.");
	}
}

std::vector<const char*> VulkanRenderer::GetRequiredExtensions()
{
	// Create list to hold instance extensions
	std::vector<const char*> instanceExtensions = {};

	// Set up extensions
	uint32_t glfwExtensionCount = 0;								// GLFW may require multiple extensions
	const char** glfwExtensions{};

	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Add GLFW extensions to list of extensions
	for (size_t i{}; i < glfwExtensionCount; ++i)
	{
		instanceExtensions.emplace_back(glfwExtensions[i]);
	}

	if (CheckValidationEnabled())
	{
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	// Check instance extensions are supported
	if (!CheckInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions");
	}

	return instanceExtensions;
}

bool VulkanRenderer::CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	// Need to get nr of extensions to create array with correct size to hold extensions.
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	for (const auto& checkExt : *checkExtensions)
	{
		bool hasExtension = false;
		for (const VkExtensionProperties& extension : extensions)
		{
			if (strcmp(checkExt, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
	/*

	// Information about device itself (ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines etc...)
	VkPhysicalDeviceFeatures deviceFeatures{};
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	*/

	const QueueFamilyIndices indices = GetQueueFamilies(device);
	const bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainValid = false;
	if (extensionsSupported)
	{
		const SwapchainDetails swapChainDetails = GetSwapChainDetails(device);
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}

	// Only suitable if all extensions are available and if it has the right queue's
	return indices.IsValid() && extensionsSupported && swapChainValid;
}

bool VulkanRenderer::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount{};
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	if (extensionCount == 0)
	{
		return false;
	}

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	// Check for extension
	for (const auto & deviceExtension : g_DeviceExtensions)
	{
		bool hasExtension = false;
		for (const auto &extension: extensions) // As soon as we don't have 1 extension we exit
		{
			if (strcmp(extension.extensionName, deviceExtension) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

// Best format is subjective but mine will be:
// format		: VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM) as backup
// colorSpace	: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR 
VkSurfaceFormatKHR VulkanRenderer::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	// This triggers if all formats are available... weird but fair
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	}

	// If not search for optimal format
	for (const auto& format: formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}

	// If no optimal format return first.
	return formats[0];
}

VkPresentModeKHR VulkanRenderer::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	// Look for mailbox presentation mode
	for (const auto& presentationMode: presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	// Fifo always has to be available as stated in the vulkan specification
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	// If current extent is at numeric limit, then extent can vary, Otherwise, it is the size of the window.
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		// If value can vary, need to set manually
		int width, height;
		glfwGetFramebufferSize(m_pWindow->GetWindow(), &width, &height);

		// Create new extent using window size
		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		// Surface also defines max and min, so make sure within boundaries by clamping values
		newExtent.width = std::clamp(newExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
		newExtent.height = std::clamp(newExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

		return newExtent;
	}
}

bool VulkanRenderer::CheckValidationEnabled()
{
#ifdef NDEBUG
	return false;
#else
	return true;
#endif
}

bool VulkanRenderer::CheckValidationLayerSupport()
{
	uint32_t layerCount{};
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : m_ValidationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

void VulkanRenderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo)
{
	debugCreateInfo = {};
	debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugCreateInfo.pfnUserCallback = debugCallback;
}

QueueFamilyIndices VulkanRenderer::GetQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices{};

	// Get all Queue family property info of given device
	uint32_t queueFamilyCount{};
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	// Go through each Queue family and check if it has at least 1 required queue
	int32_t i{};
	for (const auto& queueFamily: queueFamilyList)
	{
		// Queue family can have 0 queue's, so we check for those and if the queue can be used as a graphic's queue.
		// These can be used as a bitmask
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i; // If queue family is valid, then get index
		}

		// Check if queue family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentationSupport);
		if (queueFamily.queueCount > 0 && presentationSupport)
		{
			indices.presentationFamily = i;
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

SwapchainDetails VulkanRenderer::GetSwapChainDetails(VkPhysicalDevice device)
{
	SwapchainDetails swapChainDetails{};

	// Get surface capabilities of physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &swapChainDetails.capabilities);

	// Get formats of physical device
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, swapChainDetails.formats.data());
	}

	// Get presentation modes of physical device
	uint32_t presentationModesCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentationModesCount, nullptr);

	if (presentationModesCount != 0)
	{
		swapChainDetails.presentationModes.resize(presentationModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentationModesCount, swapChainDetails.presentationModes.data());
	}

	return swapChainDetails;
}

VkImageView VulkanRenderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;										// Image to create view for
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;					// Type of image (1D, 2D, 3D, Cube etc)
	viewCreateInfo.format = format;										// Format of image data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;		// Allows remapping of rgba components to other rgba values WHY
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;		// WHY
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;		// WHY
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;		// WHY

	// Sub resources allows the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;			// Which aspect of image to view (e.g. COLO_BIT for viewing color)
	viewCreateInfo.subresourceRange.baseMipLevel = 0;					// Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1;						// Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;					// Start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1;						// Number of array levels to view

	// Create image view
	VkImageView imageView;

	const VkResult result = vkCreateImageView(m_MainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create an image view");
	}

	return imageView;
}

VkShaderModule VulkanRenderer::CreateShaderModule(const std::vector<char>& code)
{
	// Shader module creation information
	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule{};
	VkResult result = vkCreateShaderModule(m_MainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create shader module");
	}

	return shaderModule;
}

VkResult VulkanRenderer::CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}