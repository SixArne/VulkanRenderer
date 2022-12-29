#pragma once

#include <memory>
#include <string>

#define GLM_FORCE_DEPTH_ZERO_ONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanRenderer.h"
#include "InputHandler.h"

class Window
{
public:
	Window(std::string& wName, uint32_t windowWidth, uint32_t windowHeight);
	~Window();

	void Init(VulkanRenderer& renderer);
	GLFWwindow* GetWindow() const { return m_pWindow; }

private:
	GLFWwindow* m_pWindow{ };
	InputHandler* m_pInputHandler{};

	std::string m_Name{};
	uint32_t m_Width{};
	uint32_t m_Height{};

	float m_Distance{};

	double m_ElapsedMilliSeconds{};
	float m_DeltaTime{};
	double m_FPSIntervalMilliseconds{1.0};
};

