#pragma once

#include <spdlog/spdlog.h>

namespace Odyssey
{
	class Logger
	{
	public:
		static void Initialize();

		template<class ... Args>
		static void Log(const char* format, Args... args)
		{
			spdlog::info(format, std::forward<Args>(args)...);
		}

		template<class ... Args>
		static void LogError(const char* format, Args... args)
		{
			spdlog::error(format, std::forward<Args>(args)...);
		}

	private:
		static std::shared_ptr<spdlog::logger> myLogger;
	};
}