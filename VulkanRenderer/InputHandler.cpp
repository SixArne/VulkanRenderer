#include "InputHandler.h"
#include <stdexcept>
#include <iostream>

InputHandler* InputHandler::m_Instance{nullptr};

bool InputHandler::GetKeyIsDown(int key)
{
	auto it = InputHandler::m_Instance->m_Keys.find(key);
	if (it != InputHandler::m_Instance->m_Keys.end())
	{
		return InputHandler::m_Instance->m_Keys[key] == KeyState::PRESS;
	}

	throw std::runtime_error("No key with the requested key code");
}

bool InputHandler::GetKeyIsUp(int key)
{
	auto it = InputHandler::m_Instance->m_Keys.find(key);
	if (it != InputHandler::m_Instance->m_Keys.end())
	{
		bool result = InputHandler::m_Instance->m_Keys[key] == KeyState::RELEASE;

		// Reset key state
		InputHandler::m_Instance->m_Keys[key] = KeyState::DEFAULT;

		return result;
	}

	throw std::runtime_error("No key with the requested key code");
}

double InputHandler::MouseXDelta()
{
	auto val = InputHandler::m_Instance->m_XDelta;
	InputHandler::m_Instance->m_XDelta = 0;
	return val;
}

double InputHandler::MouseYDelta()
{
	auto val =  InputHandler::m_Instance->m_YDelta;
	InputHandler::m_Instance->m_YDelta = 0;
	return val;
}

void InputHandler::Destroy()
{
	if (m_Instance != nullptr)
	{
		delete m_Instance;
	}
}

void InputHandler::CreateInstance(GLFWwindow* window, std::vector<int> keysToTrack)
{
	if (InputHandler::m_Instance == nullptr)
	{
		InputHandler::m_Instance = new InputHandler();
		glfwSetKeyCallback(window, InputHandler::KeyboardCallback);
		glfwSetCursorPosCallback(window, InputHandler::MouseCallback);

		for (int key : keysToTrack)
		{
			InputHandler::m_Instance->m_Keys[key] = KeyState::DEFAULT;
		}
	}
}

void InputHandler::SetKeyIsDown(int key, bool isDown)
{
	auto it = m_Keys.find(key);
	if (it != m_Keys.end())
	{
		m_Keys[key] = KeyState::PRESS;
	}
}

void InputHandler::SetKeyIsUp(int key, bool isUp)
{
	auto it = m_Keys.find(key);
	if (it != m_Keys.end())
	{
		m_Keys[key] = KeyState::RELEASE;
	}
}

void InputHandler::KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (action)
	{
	case GLFW_RELEASE:
		InputHandler::m_Instance->SetKeyIsUp(key, true);
		break;
	case GLFW_PRESS:
		InputHandler::m_Instance->SetKeyIsDown(key, true);
		break;
	case GLFW_REPEAT:
		InputHandler::m_Instance->SetKeyIsDown(key, true);
		break;
	default:
		break;
	}
}

void InputHandler::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (InputHandler::m_Instance->m_FirstMouse)
	{
		InputHandler::m_Instance->m_XLastFrame = xpos;
		InputHandler::m_Instance->m_YLastFrame = ypos;
		InputHandler::m_Instance->m_FirstMouse = false;
	}

	InputHandler::m_Instance->m_XDelta = xpos - InputHandler::m_Instance->m_XLastFrame;
	InputHandler::m_Instance->m_YDelta = InputHandler::m_Instance->m_YLastFrame - ypos;

	InputHandler::m_Instance->m_XLastFrame = xpos;
	InputHandler::m_Instance->m_YLastFrame = ypos;
}