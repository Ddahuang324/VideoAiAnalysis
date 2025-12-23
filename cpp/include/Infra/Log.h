#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#ifdef ERROR
#    undef ERROR
#endif

#define LOG_TRACE(msg) Infra::Logger::getInstance().log(Infra::Level::TRACE, msg)
#define LOG_DEBUG(msg) Infra::Logger::getInstance().log(Infra::Level::DEBUG, msg)
#define LOG_INFO(msg) Infra::Logger::getInstance().log(Infra::Level::INFO, msg)
#define LOG_WARN(msg) Infra::Logger::getInstance().log(Infra::Level::WARN, msg)
#define LOG_ERROR(msg) Infra::Logger::getInstance().log(Infra::Level::ERR, msg)
#define LOG_FATAL(msg) Infra::Logger::getInstance().log(Infra::Level::FATAL, msg)

namespace Infra {

enum class Level { TRACE = 0, DEBUG, INFO, WARN, ERR, FATAL };

enum class OutputTarget { CONSOLE = 0, FILE, BOTH };

inline const char* levelToString(Level level) {
    switch (level) {
        case Level::TRACE:
            return "TRACE";
        case Level::DEBUG:
            return "DEBUG";
        case Level::INFO:
            return "INFO";
        case Level::WARN:
            return "WARN";
        case Level::ERR:
            return "ERROR";
        case Level::FATAL:
            return "FATAL";
        default:
            return "UNKNOWN";
    }
}

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void log(Level level, const std::string& message) {
        if (level < currentLevel)
            return;

        std::ostringstream logStream;
        logStream << getCurrentTime() << " [" << levelToString(level) << "] "
                  << "[Thread " << std::this_thread::get_id() << "] " << message;

        std::lock_guard<std::mutex> lock(mutex);
        if (outputTarget == OutputTarget::FILE || outputTarget == OutputTarget::BOTH) {
            std::ofstream ofs(logFile, std::ios::app);
            if (ofs) {
                ofs << logStream.str() << std::endl;
            }
        }
        if (outputTarget == OutputTarget::CONSOLE || outputTarget == OutputTarget::BOTH) {
            std::cout << logStream.str() << std::endl;
        }
    }

    void setLogLevel(Level level) {
        std::lock_guard<std::mutex> lock(mutex);
        currentLevel = level;
    }

    void setOutputTarget(OutputTarget target) {
        std::lock_guard<std::mutex> lock(mutex);
        outputTarget = target;
    }

    void setLogFile(const std::string& filePath) {
        std::lock_guard<std::mutex> lock(mutex);
        logFile = filePath;
    }

private:
    Logger() : currentLevel(Level::INFO), outputTarget(OutputTarget::CONSOLE), logFile("app.log") {}
    ~Logger() = default;

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        auto ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm timeStruct;
        localtime_s(&timeStruct, &timeT);

        std::ostringstream timeStream;
        timeStream << std::put_time(&timeStruct, "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0')
                   << std::setw(3) << ms.count();
        return timeStream.str();
    }

    std::mutex mutex;
    Level currentLevel;
    OutputTarget outputTarget;
    std::string logFile;
};

}  // namespace Infra