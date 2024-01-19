#include "Logger.h"
std::shared_ptr<spdlog::logger> Logger::sLogger;

void Logger::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");

	sLogger = spdlog::stdout_color_mt("lab-graphics");
	sLogger->set_level(spdlog::level::trace);

}

spdlog::logger* Logger::GetLogger()
{
	return sLogger.get();
}
