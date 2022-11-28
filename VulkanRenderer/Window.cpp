#include "Window.h"

#include <chrono>

#include "VulkanRenderer.h"

Window::Window(std::string& wName, uint32_t windowWidth, uint32_t windowHeight)
	: m_Name{wName}, m_Width {windowWidth}, m_Height{ windowHeight }
{
	// Initialize GLFW
	glfwInit();

	// Disable default API
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	// Create window
	m_pWindow = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);
}

Window::~Window()
{
	glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}

void Window::Init(VulkanRenderer& renderer)
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		auto start = std::chrono::high_resolution_clock::now();

		// Fetch events every frame
		glfwPollEvents();

		renderer.Draw();

		auto end = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(end - start).count();
		
		m_ElapsedMilliSeconds += elapsed;

		if (m_ElapsedMilliSeconds >= m_FPSIntervalMilliseconds)
		{
			std::cout << "----------------------------------------------" << '\n';
			std::cout << "Time since last frame: " <<  elapsed << "ms" << '\n';
			std::cout << "FPS: " << 1.f / elapsed << '\n';
			std::cout << "----------------------------------------------" << '\n';
			m_ElapsedMilliSeconds = 0;
		}
	}
}

