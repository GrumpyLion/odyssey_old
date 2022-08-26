cmake_minimum_required (VERSION 3.0)

project(odyssey)

#if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../odyssey_extras")
#    set(BIN_DIR_LEN 0)
#    string(LENGTH "${CMAKE_BINARY_DIR}" BIN_DIR_LEN)
#    string(SUBSTRING "${CMAKE_BINARY_DIR}" 0 "${BIN_DIR_LEN}" EXTERNAL_BIN_DIR)
#    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../odyssey_extras" "${EXTERNAL_BIN_DIR}_extras")
#endif()

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
    "include"
    ${ODYSSEY_PATH}/extern/glfw/include
    ${ODYSSEY_PATH}/extern/spdlog/include
    ${ODYSSEY_PATH}/extern/glm
    ${ODYSSEY_PATH}/extern/imgui
)

set(SOURCES
    "src/core/engine.cpp"
    "src/core/logger.cpp"

    "src/platform/platform_layer.cpp"
    "src/platform/win_platform_layer.cpp"
    )

set(HEADERS
    "include/odyssey.h"

    "include/odyssey/defines.h"
    "include/odyssey/game.h"
    "include/odyssey/win_entry.h"
    
    "include/odyssey/core/assert.h"
    "include/odyssey/core/engine.h"
    "include/odyssey/core/logger.h"

    "include/odyssey/platform/platform_layer.h"
    "include/odyssey/platform/win_platform_layer.h"
    )

assign_source_group(${SOURCES})
assign_source_group(${HEADERS})

if (IS_WINDOWS)
    add_definitions(-DIS_WINDOWS_PLATFORM=1)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_definitions(-DIS_WINDOWS_PLATFORM=0)
endif()


add_library (odyssey ${SOURCES} ${HEADERS})

add_subdirectory(extern)