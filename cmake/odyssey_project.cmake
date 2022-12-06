cmake_minimum_required(VERSION 3.0.0)

SET(FASTLINK false CACHE BOOL "Enables /DEBUG:FASTLINK else /DEBUG:FULL")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DIS_DEBUG=1 -DIS_RELEASE=0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DIS_DEBUG=0 -DIS_RELEASE=1")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DIS_DEBUG=1 -DIS_RELEASE=0")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DIS_DEBUG=0 -DIS_RELEASE=1")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific flags
if (MSVC)
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /fp:fast /WX /W3 -D_ENABLE_EXTENDED_ALIGNED_STORAGE /wd4275 /wd4251 /bigobj")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /WX -D_ENABLE_EXTENDED_ALIGNED_STORAGE -Wno-unused-private-field -Wno-unused-variable -Wno-deprecated-declarations -Wno-microsoft-cast -Wno-switch -Wno-missing-declarations -Wno-string-plus-int -Wno-unused-function -Wno-assume")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17 /permissive-")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /sdl /Oi /Ot /Oy /Ob2 /Zi")
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /sdl /Oi /Ot /Oy /Ob2 /Zi")

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
		if(CI_BUILD EQUAL 1)
			set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /GL")
			set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /OPT:REF")
			set(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /OPT:REF")
			set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} /LTCG")
		endif()

		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /GL")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
		set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG")
	
		if (FASTLINK)
			set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK")
		else()
			set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FULL")		
		endif()
	endif()
	set(CMAKE_CXX_STANDARD 17)

	add_definitions(-D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
	add_definitions(-D_ENABLE_EXTENDED_ALIGNED_STORAGE)
else()
	if ((NOT EMSCRIPTEN) AND (NOT ANDROID_NDK))
		set(EXTRA_LIBS pthread)
	endif()
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU) 
		if (HALLEY_ENABLE_STATIC_STDLIB)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
		endif()

		# These are needed for DLLs built on GCC
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
	endif()
endif()


# From http://stackoverflow.com/questions/31422680/how-to-set-visual-studio-filters-for-nested-sub-directory-using-cmake
function(assign_source_group)
	foreach(_source IN ITEMS ${ARGN})
		if (IS_ABSOLUTE "${_source}")
			file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
		else()
			set(_source_rel "${_source}")
		endif()
		get_filename_component(_source_path "${_source_rel}" PATH)
		string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
		source_group("${_source_path_msvc}" FILES "${_source}")
	endforeach()
endfunction(assign_source_group)


set(CMAKE_DEBUG_POSTFIX "_d")

set(ODYSSEY_PROJECT_INCLUDE_DIRS
	${ODYSSEY_PATH}/include
	${ODYSSEY_PATH}/src
	${ODYSSEY_PATH}/extern/glfw/include
	${ODYSSEY_PATH}/extern/spdlog/include
	${ODYSSEY_PATH}/extern/glm
	${ODYSSEY_PATH}/extern/imgui
	${ODYSSEY_PATH}/extern/vkbootstrap
    $ENV{VK_SDK_PATH}/Include
    )

set(ODYSSEY_PROJECT_LIBS
	optimized odyssey
	optimized imgui
	optimized vulkan-1
	optimized vkbootstrap

	debug odyssey_d
	debug imgui_d
	debug vkbootstrap_d
	debug vulkan-1
	)

set(ODYSSEY_PROJECT_LIB_DIRS
	${ODYSSEY_PATH}/lib
	$ENV{VK_SDK_PATH}/Lib
	)


function(odysseyProject name sources headers proj_resources genDefinitions targetDir)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${targetDir})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${targetDir})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${targetDir})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${targetDir})

    add_subdirectory(odyssey)

	set(proj_sources ${sources})
	set(proj_headers ${headers} ${genDefinitions})

	assign_source_group(${proj_sources})
	assign_source_group(${proj_headers})
	assign_source_group(${proj_resources})

	SET(LINK_LIBRARIES "")
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} odyssey)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} imgui)
    SET(LINK_LIBRARIES ${LINK_LIBRARIES} vkbootstrap)
	SET(LINK_LIBRARIES ${LINK_LIBRARIES} ${ODYSSEY_PROJECT_LIBS})

    if (USE_GLFW)
        SET(LINK_LIBRARIES ${LINK_LIBRARIES} glfw)
    endif()

    include_directories(${ODYSSEY_PROJECT_INCLUDE_DIRS})
	link_directories(${ODYSSEY_PROJECT_LIB_DIRS})

    if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
        add_definitions(-DIS_WINDOWS_PLATFORM=1)
    else()
        add_definitions(-DIS_WINDOWS_PLATFORM=0)
    endif()

    add_executable(${name} ${proj_sources} ${proj_headers} ${proj_resources})
    target_link_libraries(${name} ${LINK_LIBRARIES})
endfunction(odysseyProject)
