#include <stdexcept>
#include <iostream>

#include "Window.h"
#include "VulkanRenderer.h"

int main()
{
	std::string windowName = "Vulkan renderer";

	Window window = Window{ windowName, 1600, 900 };
	VulkanRenderer renderer = VulkanRenderer{};

	if(renderer.Init(&window) == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}

	// Start window event loop
	window.Init();

	// Cleanup vulkan
	renderer.Cleanup();

	return EXIT_SUCCESS;
}