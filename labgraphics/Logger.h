#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <cassert>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger
{
public:
	static void Init();

	static spdlog::logger *GetLogger();

private:
	static std::shared_ptr<spdlog::logger> sLogger;
};

#define LOG_ERROR(...)                               \
	do                                               \
	{                                                \
		spdlog::error("{},{}:", __FILE__, __LINE__); \
		::Logger::GetLogger()->error(__VA_ARGS__);   \
		assert(0);                                   \
	} while (false);

#define LOG_WARN(...)                               \
	do                                              \
	{                                               \
		spdlog::warn("{},{}:", __FILE__, __LINE__); \
		::Logger::GetLogger()->warn(__VA_ARGS__);   \
	} while (false);

#define LOG_INFO(...) ::Logger::GetLogger()->info(__VA_ARGS__)