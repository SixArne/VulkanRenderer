#include <stdexcept>
#include <iostream>

#include "Window.h"
#include "VulkanRenderer.h"

int main()
{
	std::string windowName = "Vulkan renderer";

	Window window = Window{ windowName, 640, 480 };
	VulkanRenderer renderer = VulkanRenderer{};

	if(renderer.Init(&window) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	// Start window event loop
	window.Init(renderer);

	// Cleanup vulkan
	renderer.Cleanup();

	return EXIT_SUCCESS;
}