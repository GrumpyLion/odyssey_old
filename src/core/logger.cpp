#include "odyssey/core/logger.h"

#include <iomanip>
#include <sstream>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

using namespace Odyssey;

std::shared_ptr<spdlog::logger> Logger::myLogger = nullptr;

void Logger::Initialize()
{
	const auto now = std::time(nullptr);
	std::ostringstream os;
	os << "logs/";
	os << std::put_time(std::gmtime(&now), "%F - %H-%M-%S.txt");

	myLogger = spdlog::basic_logger_mt("Engine", os.str());
	myLogger->sinks().push_back(std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>());
	myLogger->sinks().push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
	spdlog::set_default_logger(myLogger);
	myLogger->set_pattern("%+");
}
