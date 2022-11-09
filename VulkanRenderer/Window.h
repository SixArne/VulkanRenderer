#pragma once

#include <memory>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class Window
{
public:
	Window(std::string& wName, uint32_t windowWidth, uint32_t windowHeight);
	~Window();

	void Init();
	GLFWwindow* GetWindow() { return m_pWindow; }

private:
	GLFWwindow* m_pWindow{ };
	std::string m_Name{};
	uint32_t m_Width{};
	uint32_t m_Height{};
};

