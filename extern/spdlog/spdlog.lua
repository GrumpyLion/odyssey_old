function includeSpdlog()
    includedirs { ENGINE_DIR .. "/extern/spdlog/include"}
end

project "spdlog"
	kind "StaticLib"
		
    files
    {
        "include/**.h"
    }