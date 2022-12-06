#pragma once

#define SPDLOG_NO_EXCEPTIONS
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
		static void Log(Args... args)
		{
			spdlog::info(std::forward<Args>(args)...);
		}

		template<class ... Args>
		static void LogWarn(const char* format, Args... args)
		{
			spdlog::warn(format, std::forward<Args>(args)...);
		}

		template<class ... Args>
		static void LogWarn(Args... args)
		{
			spdlog::warn(std::forward<Args>(args)...);
		}

		template<class ... Args>
		static void LogError(const char* format, Args... args)
		{
			spdlog::error(format, std::forward<Args>(args)...);
		}

		template<class ... Args>
		static void LogError(Args... args)
		{
			spdlog::error(std::forward<Args>(args)...);
		}

	private:
		static std::shared_ptr<spdlog::logger> myLogger;
	};
}