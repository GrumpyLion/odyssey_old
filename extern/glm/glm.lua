function includeGLM()
    includedirs { ENGINE_DIR .. "/extern/glm/include"}
end

project "glm"
	kind "StaticLib"
		
    files
    {
        "include/**.h"
    }