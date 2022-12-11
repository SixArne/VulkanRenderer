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
	//const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	m_pWindow = glfwCreateWindow(m_Width, m_Height, m_Name.c_str(), nullptr, nullptr);
	//glfwSetWindowMonitor(m_pWindow, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
}

Window::~Window()
{
	glfwDestroyWindow(m_pWindow);
	glfwTerminate();
}

void Window::Init(VulkanRenderer& renderer)
{
	float angle = 0.0f;

	// Keys to listen to
	std::vector<int> keys{
		GLFW_KEY_W,
		GLFW_KEY_S,
		GLFW_KEY_D,
		GLFW_KEY_A,
	};

	InputHandler::CreateInstance(m_pWindow, keys);
	glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	while (!glfwWindowShouldClose(m_pWindow))
	{
		const auto start = std::chrono::high_resolution_clock::now();

		// Fetch events every frame
		glfwPollEvents();

		angle += 100000.f * m_DeltaTime;
		//glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f))
		renderer.Update(m_DeltaTime);
		//renderer.UpdateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));
		renderer.Draw();

		const auto end = std::chrono::high_resolution_clock::now();
		m_DeltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
		
		m_ElapsedMilliSeconds += m_DeltaTime;

		if (m_ElapsedMilliSeconds >= m_FPSIntervalMilliseconds)
		{
			std::cout << "----------------------------------------------" << '\n';
			std::cout << "Time since last frame: " << m_DeltaTime << "ms" << '\n';
			std::cout << "FPS: " << 1.0 / m_DeltaTime << '\n';
			std::cout << "----------------------------------------------" << '\n';
			m_ElapsedMilliSeconds = 0;
		}
	}

	InputHandler::Destroy();
}

