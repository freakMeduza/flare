#include "Log.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>

namespace fve {

	std::mutex Log::mutex_;
	std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Log::loggers_;

	std::shared_ptr<spdlog::logger> Log::get(const std::string& name) {
		{
			std::lock_guard<std::mutex> lock{ mutex_ };
			if (auto it = loggers_.find(name); it != loggers_.end())
				return it->second;
		}
		return create(name);
	}

	std::shared_ptr<spdlog::logger> Log::create(const std::string& name) {
		{
			std::lock_guard<std::mutex> lock{ mutex_ };
			if (auto it = loggers_.find(name); it != loggers_.end())
				return it->second;
		}
		const std::string pattern = "%^[%Y-%m-%d %H:%M:%S.%e][thread %t][%n][%l]: %v%$";
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console_sink->set_color_mode(spdlog::color_mode::always);
		console_sink->set_level(spdlog::level::trace);
		console_sink->set_pattern(pattern);
		auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("flare.log", 23, 59);
		file_sink->set_level(spdlog::level::trace);
		file_sink->set_pattern(pattern);
		auto logger = std::shared_ptr<spdlog::logger>(new spdlog::logger{ name, { console_sink, file_sink } });
		logger->set_level(spdlog::level::trace);
		{
			std::lock_guard<std::mutex> lock{ mutex_ };
			loggers_.insert({ name, logger });
		}
		return logger;
	}

	void Log::destroy(const std::string& name) {
		std::lock_guard<std::mutex> lock{ mutex_ };
		if (auto it = loggers_.find(name); it != loggers_.end())
			loggers_.erase(it);
	}

}