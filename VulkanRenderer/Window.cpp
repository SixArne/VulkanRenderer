#include "Window.h"

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

void Window::Init()
{
	while (!glfwWindowShouldClose(m_pWindow))
	{
		// Fetch events every frame
		glfwPollEvents();
	}
}
