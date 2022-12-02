#include <unordered_map>
#include <GLFW/glfw3.h>
#include <memory>

#pragma once
class InputHandler
{
public:
	static bool GetKeyIsUp(int key);
	static bool GetKeyIsDown(int key);
	static void CreateInstance(GLFWwindow* window, std::vector<int> keysToTrack);
	static void Destroy();

	static double MouseXDelta();
	static double MouseYDelta();

private:
	enum class KeyState
	{
		PRESS,
		RELEASE, 
		DEFAULT
	};

	static InputHandler* m_Instance;

	void SetKeyIsDown(int key, bool isDown);
	void SetKeyIsUp(int key, bool isUp);
	
	std::unordered_map<int, KeyState> m_Keys{};

	double m_XLastFrame{};
	double m_YLastFrame{};
	double m_XDelta{};
	double m_YDelta{};
	bool m_FirstMouse{true};

	static void KeyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
};

