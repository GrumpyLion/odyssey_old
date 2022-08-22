ENGINE_DIR = path.getabsolute("..")

group "extern"
dofile(ENGINE_DIR .. "/extern/glm/glm.lua")
dofile(ENGINE_DIR .. "/extern/spdlog/spdlog.lua")
dofile(ENGINE_DIR .. "/extern/imgui/imgui.lua")
if platform == "x64" then
    dofile(ENGINE_DIR .. "/extern/glfw/glfw.lua")
end
group "game"

function setVariables()
    platforms { "x64" }
    targetdir "../bin"
    debugdir "../bin"
    defines { "IS_WINDOWS_PLATFORM=1" } 

    flags { "StaticRuntime" }
    if (os.is("Windows")) then defines { "_CRT_SECURE_NO_WARNINGS" } end    

    configuration { "x64", "Debug" }
        targetsuffix "_d"    
    configuration { "x64", "Release" }
        targetsuffix "_r"
    
    configuration { "Debug" }
        flags { "Symbols" }
        defines { "IS_DEBUG=1", "IS_RELEASE=0", "IS_FINAL=0" }
    configuration { "Release" }
        flags { "Optimize", "OptimizeSpeed", "No64BitChecks", "Symbols" }   
        defines { "IS_DEBUG=0", "IS_RELEASE=1", "IS_FINAL=0" }
    configuration { "Final" }
        flags { "Optimize", "OptimizeSpeed", "NoEditAndContinue", "No64BitChecks" }   
        defines { "IS_DEBUG=0", "IS_RELEASE=0", "IS_FINAL=1" }
    configuration {}
end

function setExternIncludes()
    if platform == "x64" then
        includeGLFW()
        includedirs { "$(VULKAN_SDK)" .. "/Include" }
    end

    includeGLM()
    includeSpdlog()
    includeImGUI()
end

function odysseyProject()
    links "odyssey"

    if platform == "x64" then
        links "glfw"
    end
    links "imgui"

    setExternIncludes()

    includedirs { ENGINE_DIR .. "/include"}
end

project "odyssey"
    kind "StaticLib"
    language "C++"

    includedirs { ENGINE_DIR .. "/include"}
    includedirs { ENGINE_DIR .. "/src"}

    files 
    {
        "../src/**.c*",
        "../src/**.h*",
        "../include/**.h*"
    }

    if platform == "x64" then
        links "glfw"
    end
    links "imgui"

    setExternIncludes()
    setVariables()