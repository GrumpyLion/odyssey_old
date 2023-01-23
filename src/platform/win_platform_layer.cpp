#include "odyssey/platform/platform_layer.h"

#include "renderer/vulkan/vulkan_backend.h"

#if IS_WINDOWS_PLATFORM

#include "odyssey/core/logger.h"

#include <windows.h>
#include <windowsx.h>

#include <thread>
#include <chrono>

#include <GLFW/glfw3.h>

static GLFWwindow* locWindow{};

bool PlatformLayer::Initialize(const char* title, int x, int y, int width, int height)
{
	glfwInit();

	if (!glfwVulkanSupported())
	{
		Logger::LogError("GLFW vulkan is not supported! Shutting down");
		return false;
	}
	Logger::Log("GLFW vulkan is supported!");

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	Logger::Log("Creating Window {} at {} {} with {} {}", title, x, y, width, height);
	locWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);

	glfwShowWindow(locWindow);

	return true;
}

void PlatformLayer::Shutdown()
{
	glfwDestroyWindow(locWindow);
	glfwTerminate();
}

bool PlatformLayer::PumpMessages()
{
	glfwPollEvents();
	return !glfwWindowShouldClose(locWindow);
}

double PlatformLayer::GetTimeSinceStartup()
{
	return glfwGetTime();
}

void PlatformLayer::Sleep(long ms)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int PlatformLayer::GetCoreCount()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	Logger::Log("{} cores detected.", sysinfo.dwNumberOfProcessors);
	return sysinfo.dwNumberOfProcessors;
}

VkSurfaceKHR PlatformLayer::GetVulkanSurface(VkInstance instance)
{
	VkSurfaceKHR surface{};
	VkResult err = glfwCreateWindowSurface(instance, locWindow, NULL, &surface);
	if (err)
	{
		Logger::LogError("Vulkan surface creation failed!");
		return {};
	}
	return surface;
}

bool PlatformLayer::GetVulkanExtensionNames(std::vector<const char*>& names)
{
	uint32_t count{};
	const char** extensions = glfwGetRequiredInstanceExtensions(&count);
	for (uint32_t i = 0; i < count; ++i)
	{
		names.push_back(extensions[i]);
	}

	return true;
}

#endif