#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace lob {
    
    class Logger {
        public:
        enum class Level {Debug, Info, Error};

        static void log(Level level, const std::string& message) {
            std::ostream& out = level == Level::Error ? std::cerr : std::cout;
            out << "[" << now_iso() << "] [" << level_name(level) << "]" << message << '\n';
        }

        static void debug(const std::string& msg) {
            log(Level::Debug, msg);
        }
        static void info(const std::string& msg) {
            log(Level::Info, msg);
        }
        static void error(const std::string& msg) {
            log(Level::Error, msg);
        }
        
        private:
        static std::string level_name(Level level) {
            switch (level) {
                case Level::Debug: return "DEBUG";
                case Level::Info: return "INFO";
                case Level::Error: return "ERROR";
            }
                return "INFO"; // default
        }

        static std::string now_iso() {
            const auto now = std::chrono::system_clock::now();
            const auto time = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            gmtime_s(&tm, &time); // thread-safe version of gmtime
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            return oss.str();
        }
    };
}
