cmake_minimum_required (VERSION 3.0)

project(odyssey)

#if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../odyssey_extras")
#    set(BIN_DIR_LEN 0)
#    string(LENGTH "${CMAKE_BINARY_DIR}" BIN_DIR_LEN)
#    string(SUBSTRING "${CMAKE_BINARY_DIR}" 0 "${BIN_DIR_LEN}" EXTERNAL_BIN_DIR)
#    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../odyssey_extras" "${EXTERNAL_BIN_DIR}_extras")
#endif()

find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

set(ODYSSEY_BIN "bin")
set(ODYSSEY_LIB "lib")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_BIN})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_BIN})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_BIN})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_BIN})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_BIN})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_BIN})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_BIN})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_BIN})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_LIB})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_LIB})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_LIB})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_SOURCE_DIR}/${ODYSSEY_LIB})

include_directories(
    ${ODYSSEY_PATH}/src
    ${ODYSSEY_PATH}/include
    ${ODYSSEY_PATH}/extern/glfw/include
    ${ODYSSEY_PATH}/extern/spdlog/include
    ${ODYSSEY_PATH}/extern/vma/include
    ${ODYSSEY_PATH}/extern/glm
    ${ODYSSEY_PATH}/extern/imgui
    ${ODYSSEY_PATH}/extern/vkbootstrap
    $ENV{VK_SDK_PATH}/Include
)

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${ODYSSEY_PATH}/assets_src/shaders/*.frag"
    "${ODYSSEY_PATH}/assets_src/shaders/*.vert"
    "${ODYSSEY_PATH}/assets_src/shaders/*.comp"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${ODYSSEY_PATH}/assets/shaders/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  ##execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

set(SOURCES
    "src/core/engine.cpp"
    "src/core/logger.cpp"

    "src/renderer/renderer_frontend.cpp"

    "src/renderer/vulkan/vulkan_backend.cpp"
    "src/renderer/vulkan/vulkan_initializers.cpp"
    "src/renderer/vulkan/vulkan_mesh.cpp"

    )

set(HEADERS
    "include/odyssey.h"

    "include/odyssey/defines.h"
    "include/odyssey/game.h"
    "include/odyssey/types.h"
    
    "include/odyssey/core/assert.h"
    "include/odyssey/core/engine.h"
    "include/odyssey/core/logger.h"

    "include/odyssey/platform/platform_layer.h"

    "src/renderer/renderer_types.h"
    "src/renderer/renderer_backend.h"
    "src/renderer/renderer_frontend.h"

    "src/renderer/vulkan/vulkan_backend.h"
    "src/renderer/vulkan/vulkan_types.h"
    "src/renderer/vulkan/vulkan_initializers.h"
    "src/renderer/vulkan/vulkan_mesh.h"

    "src/resources/resource_types.h"
    )

if (IS_WINDOWS)
    add_definitions(-DIS_WINDOWS_PLATFORM=1)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)

    set(SOURCES ${SOURCES}
        "src/platform/win_platform_layer.cpp"
    )

    set(HEADERS ${HEADERS}
        "include/odyssey/win_entry.h"
    )
else()
    add_definitions(-DIS_WINDOWS_PLATFORM=0)
endif()

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

add_library (odyssey ${SOURCES} ${HEADERS})

add_subdirectory(extern)