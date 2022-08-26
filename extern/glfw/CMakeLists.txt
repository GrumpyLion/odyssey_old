cmake_minimum_required(VERSION 3.0.0)

project(glfw)

include_directories("include")

set(SOURCES
    "src/context.c"
    "src/init.c"
    "src/input.c"
    "src/monitor.c"
    "src/vulkan.c"
    "src/window.c"

    "src/win32_init.c"
    "src/win32_joystick.c"
    "src/win32_monitor.c"
    "src/win32_time.c"
    "src/win32_thread.c"
    "src/win32_window.c"
    "src/wgl_context.c"
    "src/egl_context.c"
    "src/osmesa_context.c"
    )

set(HEADERS
    "include/GLFW/glfw3.h"
    "include/GLFW/glfw3native.h"
    "src/glfw_config.h"
    )

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_GLFW_WIN32)

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (glfw ${SOURCES} ${HEADERS})