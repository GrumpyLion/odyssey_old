function includeImGUI()
	includedirs { ENGINE_DIR .. "/extern/imgui"}
end

project "imgui"
	kind "StaticLib"
	language "C++"
	targetdir "bin"
	debugdir "bin"
	
	flags { "StaticRuntime" }

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

	files
	{
        "*.cpp",
        "*.h"
	}