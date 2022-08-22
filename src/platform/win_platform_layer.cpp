#include "odyssey/platform/win_platform_layer.h"

#if IS_WINDOWS_PLATFORM

#include "odyssey/core/logger.h"
using namespace Odyssey;

#include <windows.h>
#include <windowsx.h>
#include <cstdlib>

#include <thread>
#include <chrono>

#include <GLFW/glfw3.h>

bool WindowsPlatform::Initialize(const char* title, int x, int y, int width, int height)
{
	glfwInit();

	if (!glfwVulkanSupported())
	{
		return false;
	}
	Logger::Log("GLFW vulkan is supported!");

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Logger::Log("Creating Window {} at {} {} with {} {}", title, x, y, width, height);
	myWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);

	glfwShowWindow(myWindow);

	return true;
}

void WindowsPlatform::Shutdown()
{
	glfwDestroyWindow(myWindow);
	glfwTerminate();
}

bool WindowsPlatform::PumpMessages()
{
	glfwPollEvents();
	return !glfwWindowShouldClose(myWindow);
}

double WindowsPlatform::GetTimeSinceStartup()
{
	return 0.0;
}

void WindowsPlatform::Sleep(long ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int WindowsPlatform::GetCoreCount()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	Logger::Log("{} cores detected.", sysinfo.dwNumberOfProcessors);
	return sysinfo.dwNumberOfProcessors;
}

#endif
