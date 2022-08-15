function includeGLFW()
    includedirs { ENGINE_DIR .. "/extern/glfw/include"}
end

project "glfw"
	kind "StaticLib"
	language "C++"
    targetdir "bin"
    debugdir "bin"

	files
	{
		"include/GLFW/glfw3.h",
		"include/GLFW/glfw3native.h",
		"src/glfw_config.h",
		"src/context.c",
		"src/init.c",
		"src/input.c",
		"src/monitor.c",
		"src/vulkan.c",
		"src/window.c"
	}

	files
	{
		"src/win32_init.c",
		"src/win32_joystick.c",
		"src/win32_monitor.c",
		"src/win32_time.c",
		"src/win32_thread.c",
		"src/win32_window.c",
		"src/wgl_context.c",
		"src/egl_context.c",
		"src/osmesa_context.c"
	}

	defines 
	{ 
		"_GLFW_WIN32",
		"_CRT_SECURE_NO_WARNINGS"
	}