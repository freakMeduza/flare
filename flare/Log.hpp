#pragma once

#include <spdlog/spdlog.h>

#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>

#define Log_info(format, ...) { fve::Log::log("flare", fve::Log::Level::Info, format, __VA_ARGS__); }
#define Log_warn(format, ...) { fve::Log::log("flare", fve::Log::Level::Warn, format, __VA_ARGS__); }
#define Log_error(format, ...) { fve::Log::log("flare", fve::Log::Level::Error, format, __VA_ARGS__); }
#define Log_debug(format, ...) { fve::Log::log("flare", fve::Log::Level::Debug, format, __VA_ARGS__); }

namespace fve {

	class Log {
	public:
		enum class Level : uint8_t {
			Debug,
			Info,
			Warn,
			Error
		};

		Log() = delete;
		~Log() = delete;

		Log(const Log&) = delete;
		Log& operator=(const Log&) = delete;

		static std::shared_ptr<spdlog::logger> get(const std::string& name);
		static std::shared_ptr<spdlog::logger> create(const std::string& name);
		static void destroy(const std::string& name);

		template<typename ... Args>
		static void log(const std::string& name, Level level, const std::string& format, Args&& ... args) {
			switch (level) {
			case Level::Debug: {
#ifdef _DEBUG
				get(name)->debug(format, std::forward<Args>(args)...);
#else
				// doing nothing
#endif
				break;
			}
			case Level::Info:
				get(name)->info(format, std::forward<Args>(args)...);
				break;
			case Level::Warn:
				get(name)->warn(format, std::forward<Args>(args)...);
				break;
			case Level::Error:
				get(name)->error(format, std::forward<Args>(args)...);
				break;
			}
		}

	private:
		static std::mutex mutex_;
		static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> loggers_;

	};

}