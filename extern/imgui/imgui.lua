function includeImGUI()
	includedirs { ENGINE_DIR .. "/extern/imgui"}
end

project "imgui"
	kind "StaticLib"
	language "C++"
	targetdir "bin"
	debugdir "bin"

	files
	{
        "*.cpp",
        "*.h"
	}