ENGINE_DIR = path.getabsolute("..")

function setVariablesWindows()
    platforms { "x64" }
    targetdir "../bin"
    debugdir "../bin"
    defines { "IS_WINDOWS_PLATFORM=1" }

    if (os.is("Windows")) then flags {"StaticRuntime"} end
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
end

function odysseyProject()
    links "odyssey"

    includedirs { ENGINE_DIR .. "/include"}
    includedirs { ENGINE_DIR .. "/src"}
end


project "odyssey"
    kind "StaticLib"
    language "C++"

    includedirs { ENGINE_DIR .. "/src"}

    files {
        "../src/**.c*",
        "../src/**.h*",
        "../include/**.h*"
    }

    setVariablesWindows()