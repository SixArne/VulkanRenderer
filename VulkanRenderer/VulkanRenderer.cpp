#include "VulkanRenderer.h"

#include <stdexcept>


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
		GetPhysicalDevice();
		CreateLogicalDevice();
	}
	catch(const std::runtime_error &e) {
		printf("[ERROR]: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRenderer::Cleanup()
{
	if (CheckValidationEnabled())
	{
		DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}

	// Order is important, instance should be last (I think)
	vkDestroyDevice(m_MainDevice.logicalDevice, nullptr);
	vkDestroyInstance(m_Instance, nullptr); // allocator param is custom de allocator func
}

void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
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

	// Setup validation layers
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (CheckValidationEnabled())
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}

	// Create instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_Instance);	// Allocator parameter is important! Look into it later

	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan instance");
	}
}

void VulkanRenderer::CreateLogicalDevice()
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
	vkGetDeviceQueue(m_MainDevice.logicalDevice, indices.graphicsFamily, 0, &m_GraphicsQueue);
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

VkDevice VulkanRenderer::GetPhysicalDevice()
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

	// Check instance extensions are supported
	if (!CheckInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions");
	}

	if (CheckValidationEnabled())
	{
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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
			if (strcmp(checkExt, extension.extensionName))
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

	QueueFamilyIndices indices = GetQueueFamilies(device);

	return indices.IsValid();
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
	uint32_t i{};
	for (const auto& queueFamily: queueFamilyList)
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
