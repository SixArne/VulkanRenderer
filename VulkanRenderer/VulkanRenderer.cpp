#include "VulkanRenderer.h"

#include <functional>
#include <set>
#include <stdexcept>

#include "Window.h"
#include <random>

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
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandPool();

		m_UboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)m_SwapchainExtent.width / (float)m_SwapchainExtent.height, 0.1f, 100.0f);
		m_UboViewProjection.view = glm::lookAt(m_CameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		m_UboViewProjection.projection[1][1] *= -1.f;

		// Create a mesh
		// Vertex data
		std::vector<Vertex> meshVertices = { 
			{{-0.1, -0.4, 0.0}, {1.0,0.0,0.0}},		// 0
			{{-0.1, 0.4, 0.0}, {0.0,1.0,0.0}},		// 1
			{{-0.9, 0.4, 0.0}, {0.0,0.0,1.0}},		// 2
			{{-0.9, -0.4, 0.0}, {1.0,1.0,0.0}},		// 3
		};

		std::vector<Vertex> meshVertices2 = {
			{{0.9, -0.4, 0.0}, {1.0,0.0,0.0}},		// 0
			{{0.9, 0.4, 0.0}, {0.0,1.0,0.0}},		// 1
			{{0.1, 0.4, 0.0}, {0.0,0.0,1.0}},		// 2
			{{0.1, -0.4, 0.0}, {1.0,1.0,0.0}},		// 3
		};

		// Index data
		std::vector<uint32_t> meshIndices = {
			0, 1, 2,
			2, 3, 0
		};

		Mesh m1 = Mesh(
			m_MainDevice.physicalDevice,
			m_MainDevice.logicalDevice,
			m_GraphicsQueue,
			m_GraphicsCommandPool,
			&meshVertices,
			&meshIndices
		);

		Mesh m2 = Mesh(
			m_MainDevice.physicalDevice,
			m_MainDevice.logicalDevice,
			m_GraphicsQueue,
			m_GraphicsCommandPool,
			&meshVertices2,
			&meshIndices
		);

		m_MeshList.push_back(m1);
		m_MeshList.push_back(m2);

		glm::mat4 meshModelMatrix = m_MeshList[0].GetModel().model;
		meshModelMatrix = glm::rotate(meshModelMatrix, glm::radians(45.f), glm::vec3(0.f,0.f,1.f));
		m_MeshList[0].SetModel(meshModelMatrix);

		CreateCommandBuffers();
		AllocateDynamicBufferTransferSpace();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		RecordCommands();
		CreateSynchronization();
	}
	catch(const std::runtime_error &e) {
		printf("[ERROR]: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void VulkanRenderer::Update(float deltaTime)
{
	bool hasPressedW = InputHandler::GetKeyIsDown(GLFW_KEY_W);
	bool hasPressedS = InputHandler::GetKeyIsDown(GLFW_KEY_S);
	bool hasPressedD = InputHandler::GetKeyIsDown(GLFW_KEY_D);
	bool hasPressedA = InputHandler::GetKeyIsDown(GLFW_KEY_A);

	double deltaX = InputHandler::MouseXDelta();
	double deltaY = InputHandler::MouseYDelta();

	float rotationSpeed{.1f};
	float moveSpeed{20.f};

	m_CameraYaw += deltaX * rotationSpeed;
	m_CameraPitch += deltaY * rotationSpeed;

	if (hasPressedW)
	{
		m_CameraPos += m_CameraFront * moveSpeed * deltaTime;
	}
	else if (hasPressedS)
	{
		m_CameraPos -= m_CameraFront * moveSpeed * deltaTime;
	}

	if (hasPressedD)
	{
		m_CameraPos += (glm::normalize(glm::cross(m_CameraFront, m_CameraUp))) * moveSpeed * deltaTime;
	}
	else if (hasPressedA)
	{
		m_CameraPos -= (glm::normalize(glm::cross(m_CameraFront, m_CameraUp))) * moveSpeed * deltaTime;
	}

	glm::vec3 direction{};
	direction.x = cos(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
	direction.y = sin(glm::radians(m_CameraPitch));
	direction.z = sin(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));

	m_CameraFront = glm::normalize(direction);

	m_UboViewProjection.view =  glm::lookAt(m_CameraPos, m_CameraPos + m_CameraFront, m_CameraUp);
}

void VulkanRenderer::UpdateModel(glm::mat4 newModel)
{
	//m_UboViewProjection
}

void VulkanRenderer::Cleanup()
{
	// Wait until no actions being run on device before destroy
	vkDeviceWaitIdle(m_MainDevice.logicalDevice);

	// Free memory blocks
	_aligned_free(m_ModelTransferSpace);

	vkDestroyDescriptorPool(m_MainDevice.logicalDevice, m_DescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_MainDevice.logicalDevice, m_DescriptorSetLayout, nullptr);
	for (size_t i{}; i < m_SwapchainImages.size(); ++i)
	{
		vkDestroyBuffer(m_MainDevice.logicalDevice, m_VPUniformBuffer[i], nullptr);
		vkFreeMemory(m_MainDevice.logicalDevice, m_VPUniformBufferMemory[i], nullptr);

		vkDestroyBuffer(m_MainDevice.logicalDevice, m_ModelDynamicUniformBuffer[i], nullptr);
		vkFreeMemory(m_MainDevice.logicalDevice, m_ModelDynamicUniformBufferMemory[i], nullptr);
	}

	// Cleanup
	for (auto& mesh : m_MeshList)
	{
		mesh.DestroyBuffers();
	}

	for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
	{
		vkDestroySemaphore(m_MainDevice.logicalDevice, m_RendersFinished[i], nullptr);
		vkDestroySemaphore(m_MainDevice.logicalDevice, m_ImagesAvailable[i], nullptr);
		vkDestroyFence(m_MainDevice.logicalDevice, m_DrawFences[i], nullptr);
	}

	vkDestroyCommandPool(m_MainDevice.logicalDevice, m_GraphicsCommandPool, nullptr);

	for (const auto& framebuffer : m_SwapchainFramebuffers)
	{
		vkDestroyFramebuffer(m_MainDevice.logicalDevice, framebuffer, nullptr);
	}

	vkDestroyPipeline(m_MainDevice.logicalDevice, m_GraphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_MainDevice.logicalDevice, m_PipelineLayout, nullptr);
	vkDestroyRenderPass(m_MainDevice.logicalDevice, m_RenderPass, nullptr);

	for (const auto& image : m_SwapchainImages)
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


void VulkanRenderer::Draw()
{

	// -- GET NEXT IMAGE --

	// Wait for fence until received signal
	vkWaitForFences(m_MainDevice.logicalDevice, 1, &m_DrawFences[m_CurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	// Undo signal
	vkResetFences(m_MainDevice.logicalDevice, 1, &m_DrawFences[m_CurrentFrame]);

	// Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
	uint32_t imageIndex{};
	VkResult result = vkAcquireNextImageKHR(m_MainDevice.logicalDevice, m_Swapchain, std::numeric_limits<uint64_t>::max(), m_ImagesAvailable[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to acquire next image");
	}

	UpdateUniformBuffers(imageIndex);

	// -- SUBMIT COMMAND BUFFER TO RENDER
	// Queue submission information
	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;											// Number of semaphores to wait on
	submitInfo.pWaitSemaphores = &m_ImagesAvailable[m_CurrentFrame];			// List of semaphores to wait on

	const VkPipelineStageFlags waitStages[] = {									// Stages to check semaphores on
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;											// number of cmd buffers to submit
	submitInfo.pCommandBuffers = &m_CommandBuffers[imageIndex];					// Cmd buffer to submit
	submitInfo.signalSemaphoreCount = 1;										// Semaphores to signal before end
	submitInfo.pSignalSemaphores = &m_RendersFinished[m_CurrentFrame];			// Semaphores to signal when cmd buffer finishes.

	// Submit cmd buffer to queue and signal fence so code can continue (fence is initially closed)
	result = vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_DrawFences[m_CurrentFrame]);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit command to queue");
	}

	// -- PRESENT RENDERED IMAGE TO SCREEN --
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;											// Number of semaphores to wait on
	presentInfo.pWaitSemaphores = &m_RendersFinished[m_CurrentFrame];			// Semaphores to wait on
	presentInfo.swapchainCount = 1;												// Number of swapchains to present to
	presentInfo.pSwapchains = &m_Swapchain;										// Swapchains to present images to
	presentInfo.pImageIndices = &imageIndex;									// index of images in swapchains to present

	// Present image
	result = vkQueuePresentKHR(m_PresentationQueue, &presentInfo);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to present info to surface");
	}

	

	// Used to make semi unique semaphores, otherwise semaphore will be used for multiple frames and trigger irregularly
	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAME_DRAWS;
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

void VulkanRenderer::CreateRenderPass()
{
	// Color attachment of render pass
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_SwapchainImageFormat;					// Format to use for attachment
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples for multi-sampling
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Describes to do with attachment before rendering.
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;				// Describes to do with attachment after rendering.
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    // Describes to do with stencil before rendering.
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;  // Describes to do with stencil after rendering.

	// Frame buffer data will be stored as an image, but images can be given different data layouts
	// to give optimal use for certain operations
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// The image data layout before render pass starts.
	// So the data will start with any layout, then the sub passes will
	// transform to their optimal layout and when the render pass finally
	// ends it will be put in the final layout.
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// The image data layout after render (to change to)

	// Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;										// index of it, in this case we only have 1 so we pick index 0
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;	// Layout for data in sub pass.

	// Information about specific sub pass the render pass is using
	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// Pipeline type sub pass is to be bound to.
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	// Need to determine when layout transitions occur using sub pass dependencies
	std::array<VkSubpassDependency, 2> subpassDependencies{};

	// -- Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL --
	// When the bottom of the external subpass attempts to read the memory we start converting from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	// But make sure before first subpass kicks in and if the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT starts to either write or read.
	// List of what stage triggers what bit => https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkAccessFlagBits.html

	// Transition must happen after these conditions...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;								// Subpass index (external = special value outside of renderpass)
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;				// Pipeline stage
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;						// Stage access mask (memory access)

	// Transition must happen before these conditions...
	subpassDependencies[0].dstSubpass = 0; // index of subpass (only have 1 so index 0)	
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// -- Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR --
	// Transition must happen after these conditions...
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Transition must happen before these conditions...
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	// Create info for render pass
	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	const VkResult result = vkCreateRenderPass(m_MainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &m_RenderPass);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create render pass. HAVE FUN DEBUGGING THIS IF IT FAILED LOL");
	}
}

void VulkanRenderer::CreateDescriptorSetLayout()
{
	// MVP binding info
	VkDescriptorSetLayoutBinding vpLayoutBinding{};
	vpLayoutBinding.binding = 0;											// Where to bind in shader (layout(binding = 0))
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// Type of descriptor (uniform, dynamic uniform, image sampler etc...)
	vpLayoutBinding.descriptorCount = 1;									// Number of descriptors for binding
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage to bind to
	vpLayoutBinding.pImmutableSamplers = nullptr;							// For textures: can make sampler immutable

	VkDescriptorSetLayoutBinding modelLayoutBinding{};
	modelLayoutBinding.binding = 1;
	modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelLayoutBinding.descriptorCount = 1;
	modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	modelLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
		vpLayoutBinding, modelLayoutBinding
	};

	// Create descriptor set layout with given bindings.
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());				// Number of binding info's
	layoutCreateInfo.pBindings = layoutBindings.data();											// Bindings

	VkResult result = vkCreateDescriptorSetLayout(m_MainDevice.logicalDevice, &layoutCreateInfo, nullptr, &m_DescriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Error creating descriptor layout");
	}
}

void VulkanRenderer::CreateGraphicsPipeline()
{
	// Read in SPIR-V code of shaders
	auto vertexShaderCode = ReadFile("Shaders/vert.spv");
	auto fragmentShaderCode = ReadFile("Shaders/frag.spv");

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

	// How the data for a single vertex (including info such as position, color, texture coordinates and normals) are at a whole
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;									// Binding position (can bind multiple streams of data)
	bindingDescription.stride = sizeof(Vertex);						// Offset to next piece of data
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// How to move between data between each vertex
																	// VK_VERTEX_INPUT_RATE_VERTEX		: move on to next vertex
																	// VK_VERTEX_INPUT_RATE_INSTANCE	: move on to vertex for next instance (can draw 100 trees as 1 tree)

	// How the data for an attribute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

	// Position attribute
	attributeDescriptions[0].binding = 0;								// Which binding it is at (same as above)
	attributeDescriptions[0].location = 0;								// Which location it is at (same as above)
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;		// Format data will take (helps define size)
	attributeDescriptions[0].offset = offsetof(Vertex, pos);			// Offset from next attribute in data

	// Color attribute
	attributeDescriptions[1].binding = 0;								
	attributeDescriptions[1].location = 1;								
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;	
	attributeDescriptions[1].offset = offsetof(Vertex, col);			

	// -- VERTEX INPUT --
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;											// List of vertex binding descriptions (data spacing/stride information)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();								// List of vertex attribute descriptions (data format and where to bind to/from)

	// -- INPUT ASSEMBLY --
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;						// Primitive type to assemble vertices as
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;									// Allow overriding of "strip" topology to start new primitives

	// -- VIEWPORT & SCISSOR --
	// Create a viewport info struct
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)m_SwapchainExtent.width;
	viewport.height = (float)m_SwapchainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// Create a scissor info struct
	VkRect2D scissor{};
	scissor.offset = { 0,0 };		// Offset to use region from
	scissor.extent = m_SwapchainExtent;		// Extent to describe region to use

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	// -- DYNAMIC STATES --
	// Dynamic states to enable (to avoid baked in values)
	//std::vector<VkDynamicState> dynamicStateEnables{};
	//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	// Dynamic viewport, resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport)
	//dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// Dynamic scissor, resize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor)

	//// Dynamic state creation info
	//VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
	//dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	//dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	//dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

	// -- RASTERIZER --
	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;					// Change if fragments beyond near/far plane are clipped or clamped to plane (need to enable device feature for this)
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;			// Discard data and skip. Used for data without rendering (Leave to false)

	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;			// How to handle polygon rendering. (fill will consider all points within polygon as fragment/pixels. line is handy for wire frames)
																		// other needs GPU features

	rasterizerCreateInfo.lineWidth = 1.0f;								// Any value other that 1 needs GPU feature
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;				// Don't render back side.
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Which side is front
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;					// Whether to add depth bias to fragments (good for stopping "Shadow acne" in shadow mapping)

	// -- MULTISAMPLING --
	VkPipelineMultisampleStateCreateInfo multiSamplingCreateInfo{};
	multiSamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSamplingCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multi sampling shading or not
	multiSamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment

	// -- BLENDING --
	// Blending decides how to blend a new color being written to a fragment, with the old value

	// Blend attachment state => how blending is handled
	VkPipelineColorBlendAttachmentState colorState{};
	colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;  // Colors to apply blending to
	colorState.blendEnable = VK_TRUE;							// Enable blending

	// Blending uses the follow equation: (srcColorBlendFactor * new color) colorBlendOp (destColorBlendFactor * old color)
	colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorState.colorBlendOp = VK_BLEND_OP_ADD;

	// Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new color) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
	//			   ( new color alpha * new color) + ((1- new color alpha) * old color)

	colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorState.alphaBlendOp = VK_BLEND_OP_ADD;
	// Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;			// Alternative to calculations is to use logical operations.
	//colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorState;
	
	// -- PIPELINE LAYOUT --
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &m_DescriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	// Create Pipeline layout
	VkResult result = vkCreatePipelineLayout(m_MainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create pipeline layout");
	}

	// -- Depth stencil testing
	// TODO: Set up depth stencil testing

	// -- Graphics pipeline creation
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;										// Number of shader stages
	pipelineCreateInfo.pStages = shaderStages;								// List of shader stages
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;          // All the fixed function pipeline states
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multiSamplingCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.layout = m_PipelineLayout;							// Pipeline layout to be used
	pipelineCreateInfo.renderPass = m_RenderPass;							// Render pass description the pipeline is compatible with
	pipelineCreateInfo.subpass = 0;											// Subpass of render pass to use with pipeline

	// Pipeline derivatives: can create multiple pipelines that derive from one another for optimization
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;					// Existing pipeline to derive from
	pipelineCreateInfo.basePipelineIndex = -1;								// or index of pipeline being create to derive from (if making multiple)

	// Create graphics pipeline
	result = vkCreateGraphicsPipelines(m_MainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_GraphicsPipeline);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create graphics pipelines");
	}

	// CREATE PIPELINE (once we create pipeline we can destroy here)
	vkDestroyShaderModule(m_MainDevice.logicalDevice, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(m_MainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::CreateFrameBuffers()
{
	m_SwapchainFramebuffers.resize(m_SwapchainImages.size());

	// Create a framebuffer for each swapchain image.
	for (size_t i{}; i < m_SwapchainFramebuffers.size(); ++i)
	{
		std::array<VkImageView, 1> attachments{
			m_SwapchainImages[i].imageView
		}; 

		VkFramebufferCreateInfo framebufferCreateInfo{};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = m_RenderPass;										// Render pass layout the framebuffer will be used with
		framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());		
		framebufferCreateInfo.pAttachments = attachments.data();								// List of attachments 1:1 with render pass
		framebufferCreateInfo.width = m_SwapchainExtent.width;
		framebufferCreateInfo.height = m_SwapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		const VkResult result = vkCreateFramebuffer(m_MainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &m_SwapchainFramebuffers[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Error creating framebuffer");
		}
	}
}

void VulkanRenderer::CreateCommandPool()
{
	const QueueFamilyIndices indices = GetQueueFamilies(m_MainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = indices.graphicsFamily;

	// Create a graphics queue family command pool
	const VkResult result = vkCreateCommandPool(m_MainDevice.logicalDevice, &poolInfo, nullptr, &m_GraphicsCommandPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create command pool");
	}
}

void VulkanRenderer::CreateCommandBuffers()
{
	m_CommandBuffers.resize(m_SwapchainFramebuffers.size());

	VkCommandBufferAllocateInfo cbAllocInfo{};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = m_GraphicsCommandPool;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;		// Primary is buffer you submit directly to queue, can not be called by other buffers.
																// Secondary can't be called directly but can be executed by other buffers.

	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(m_SwapchainFramebuffers.size());

	// Allocate command buffers and give reference to commandBuffers
	const VkResult result = vkAllocateCommandBuffers(m_MainDevice.logicalDevice, &cbAllocInfo, m_CommandBuffers.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Error allocating command buffers");
	}
}

void VulkanRenderer::CreateSynchronization()
{
	m_ImagesAvailable.resize(MAX_FRAME_DRAWS);
	m_RendersFinished.resize(MAX_FRAME_DRAWS);
	m_DrawFences.resize(MAX_FRAME_DRAWS);

	// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence creation information
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i{}; i < MAX_FRAME_DRAWS; ++i)
	{
		if (vkCreateSemaphore(m_MainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &m_ImagesAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_MainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &m_RendersFinished[i]) != VK_SUCCESS || 
			vkCreateFence(m_MainDevice.logicalDevice, &fenceCreateInfo, nullptr, &m_DrawFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create semaphore or fence");
		}
	}
}

void VulkanRenderer::CreateUniformBuffers()
{
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

	// Model buffer size
	VkDeviceSize modelBufferSize = m_ModelUniformAllignment * MAX_OBJECTS;

	// One uniform buffer for each image (and by extension, command buffer)
	// To prevent race conditions
	m_VPUniformBuffer.resize(m_SwapchainImages.size());
	m_VPUniformBufferMemory.resize(m_SwapchainImages.size());

	m_ModelDynamicUniformBuffer.resize(m_SwapchainImages.size());
	m_ModelDynamicUniformBufferMemory.resize(m_SwapchainImages.size());
	

	// Create uniform buffers
	for (size_t i{}; i < m_SwapchainImages.size(); ++i)
	{
		CreateBuffer(
			m_MainDevice.physicalDevice,
			m_MainDevice.logicalDevice,
			vpBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_VPUniformBuffer[i],
			&m_VPUniformBufferMemory[i]
		);

		CreateBuffer(
			m_MainDevice.physicalDevice,
			m_MainDevice.logicalDevice,
			modelBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&m_ModelDynamicUniformBuffer[i],
			&m_ModelDynamicUniformBufferMemory[i]
		);
	}
}

void VulkanRenderer::CreateDescriptorPool()
{
	// Type of descriptors and how many descriptors (combined makes pool size)
	// ViewProjection pool
	VkDescriptorPoolSize vpPoolSize{};
	vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vpPoolSize.descriptorCount = static_cast<uint32_t>(m_VPUniformBuffer.size());

	VkDescriptorPoolSize modelPoolSize{};
	modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelPoolSize.descriptorCount = static_cast<uint32_t>(m_ModelDynamicUniformBuffer.size());

	// List of pool sizes
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
		vpPoolSize, modelPoolSize
	};

	VkDescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(m_SwapchainImages.size());					// Maximum number of descriptor sets that can be created from pool
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());				// Amount of pool sizes being passed
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();										// Pool sizes to create pool with

	VkResult result = vkCreateDescriptorPool(m_MainDevice.logicalDevice , &poolCreateInfo, nullptr, &m_DescriptorPool);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void VulkanRenderer::CreateDescriptorSets()
{
	// Resize descriptor size so one for every buffer
	m_DescriptorSets.resize(m_SwapchainImages.size());

	std::vector<VkDescriptorSetLayout> setLayouts(m_SwapchainImages.size(), m_DescriptorSetLayout);

	// Descriptor set allocation info
	VkDescriptorSetAllocateInfo setAllocInfo{};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = m_DescriptorPool;										// Pool to allocate descriptor set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_SwapchainImages.size());	// Number of sets to allocate
	setAllocInfo.pSetLayouts = setLayouts.data();										// Layouts to use to allocate sets (1:1 relationship)
	
	VkResult result = vkAllocateDescriptorSets(m_MainDevice.logicalDevice, &setAllocInfo, m_DescriptorSets.data());
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Error allocating descriptor set");
	}

	// Update all descriptor set buffer bindings
	for (size_t i = 0; i < m_SwapchainImages.size(); i++)
	{
		// Buffer info and data offset info
		// ViewProjection descriptor
		VkDescriptorBufferInfo vpBufferInfo{};
		vpBufferInfo.buffer = m_VPUniformBuffer[i];						// Buffer to get data from
		vpBufferInfo.offset = 0;										// Position of start of data
		vpBufferInfo.range = sizeof(UboViewProjection);					// Size of data

		// Data about connection between binding and buffer
		VkWriteDescriptorSet vpSetWrite{};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = m_DescriptorSets[i];						// Descriptor set to update
		vpSetWrite.dstBinding = 0;										// Binding to update (matches with binding on layout/shader)
		vpSetWrite.dstArrayElement = 0;									// Index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Type of descriptor
		vpSetWrite.descriptorCount = 1;									// Amount to update
		vpSetWrite.pBufferInfo = &vpBufferInfo;							// Buffer information data to bind

		// Model descriptor
		// Model buffer binding info
		VkDescriptorBufferInfo modelBufferInfo{};
		modelBufferInfo.buffer = m_ModelDynamicUniformBuffer[i];
		modelBufferInfo.offset = 0;
		modelBufferInfo.range = m_ModelUniformAllignment;

		VkWriteDescriptorSet modelSetWrite{};
		modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		modelSetWrite.dstSet = m_DescriptorSets[i];
		modelSetWrite.dstBinding = 1;
		modelSetWrite.dstArrayElement = 0;
		modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelSetWrite.descriptorCount = 1;
		modelSetWrite.pBufferInfo = &modelBufferInfo;

		// List of descriptor write sets
		std::vector<VkWriteDescriptorSet> setWrites = {
			vpSetWrite, modelSetWrite
		};

		// Update descriptor sets with new buffer binding info
		vkUpdateDescriptorSets(
			m_MainDevice.logicalDevice, 
			static_cast<uint32_t>(setWrites.size()), 
			setWrites.data(), 
			0, 
			nullptr
		);
	}
}

void VulkanRenderer::UpdateUniformBuffers(uint32_t imageIndex)
{
	// Copy VP data
	void* data;
	vkMapMemory(m_MainDevice.logicalDevice, m_VPUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
	memcpy(data, &m_UboViewProjection, sizeof(UboViewProjection));
	vkUnmapMemory(m_MainDevice.logicalDevice, m_VPUniformBufferMemory[imageIndex]);

	// Copy Model data
	for (size_t i{}; i < m_MeshList.size(); i++)
	{
		UboModel* thisModel = (UboModel*)((uint64_t)m_ModelTransferSpace + (i * m_ModelUniformAllignment));
		*thisModel = m_MeshList[i].GetModel();
	}

	// Map the list of model data
	vkMapMemory(m_MainDevice.logicalDevice, m_ModelDynamicUniformBufferMemory[imageIndex], 0, m_ModelUniformAllignment * m_MeshList.size(), 0, &data);
	memcpy(data, m_ModelTransferSpace, m_ModelUniformAllignment * m_MeshList.size());
	vkUnmapMemory(m_MainDevice.logicalDevice, m_ModelDynamicUniformBufferMemory[imageIndex]);
}

void VulkanRenderer::RecordCommands()
{
	VkCommandBufferBeginInfo commandBufferBeginInfo{};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	const VkClearValue clearValues[] = {
		{0.8f, 0.1f, 0.2f, 1.0f}
	};

	// Info on how to begin a render pass (only graphical applications)
	VkRenderPassBeginInfo renderpassBeginInfo{};
	renderpassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderpassBeginInfo.renderPass = m_RenderPass;
	renderpassBeginInfo.renderArea.offset = { 0,0 };		// Area to render on, we could set this to a smaller size. (test frame rate difference later)
	renderpassBeginInfo.renderArea.extent = m_SwapchainExtent;
	renderpassBeginInfo.pClearValues = clearValues;				// list of clear values (TODO: Depth attachment clear value)
	renderpassBeginInfo.clearValueCount = 1;

	for (size_t i{}; i < m_CommandBuffers.size(); i++)
	{
		renderpassBeginInfo.framebuffer = m_SwapchainFramebuffers[i];

		// Start recording
		VkResult result = vkBeginCommandBuffer(m_CommandBuffers[i], &commandBufferBeginInfo);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Error recording command buffer");
		}

		{
			// Begin render pass
			vkCmdBeginRenderPass(m_CommandBuffers[i], &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			{
				// Bind pipeline to be used in render pass (could use different pipelines here e.g. other shading)
				vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

				for (size_t j{}; j < m_MeshList.size(); j++)
				{
					const VkBuffer vertexBuffers[] = { m_MeshList[j].GetVertexBuffer()};			// Buffers to bind
					const VkDeviceSize offsets[] = { 0 };									// Offsets into buffers being bound
					vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);

					// Bind mesh index buffer with 0 offset and using uint32_t
					vkCmdBindIndexBuffer(m_CommandBuffers[i], m_MeshList[j].GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

					// Dynamic offset amount
					uint32_t dynamicOffset = static_cast<uint32_t>(m_ModelUniformAllignment) * j;

					// Bind descriptor sets
					vkCmdBindDescriptorSets(
						m_CommandBuffers[i],
						VK_PIPELINE_BIND_POINT_GRAPHICS,
						m_PipelineLayout,
						0,
						1,
						&m_DescriptorSets[i],
						1,
						&dynamicOffset
					);

					// Execute pipeline (will run through this x amount of times)
					vkCmdDrawIndexed(m_CommandBuffers[i], m_MeshList[j].GetIndexCount(), 1, 0, 0, 0);
				}
			}

			// End render pass
			vkCmdEndRenderPass(m_CommandBuffers[i]);
		}

		// End recording
		result = vkEndCommandBuffer(m_CommandBuffers[i]);
		if (result != VK_SUCCESS)
		{
			throw std::runtime_error("Error to stop recording command buffer");
		}
	}
}

void VulkanRenderer::AllocateDynamicBufferTransferSpace()
{
	// here be magic: TODO: look this up later
	// Calculate alignment of model data
	m_ModelUniformAllignment = (sizeof(UboModel) + m_MinUniformBufferOffset - 1) & ~(m_MinUniformBufferOffset - 1);

	// Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds max objects.
	m_ModelTransferSpace = (UboModel*)_aligned_malloc(m_ModelUniformAllignment * MAX_OBJECTS, m_ModelUniformAllignment);
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

	// Get properties of our new device
	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(m_MainDevice.physicalDevice, &deviceProperties);

	m_MinUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;
		
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


	// Information about device itself (ID, name, type, vendor, etc)
	//VkPhysicalDeviceProperties deviceProperties{};
	//vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines etc...)
	//VkPhysicalDeviceFeatures deviceFeatures{};
	//vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

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