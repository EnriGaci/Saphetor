#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>

enum class LogLevel { DEBUG, INFO, WARNING, ERROR, TEST }; // Loglevel TEST to be used in tests, so that we can easily filter out test logs from production logs

#define LOG(level, msg) Logger::getInstance().log(level, msg, __FILE__, __LINE__)

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }
    void setLevel(LogLevel lvl) { minLevel = lvl; }
    void log(LogLevel level, const std::string& msg, const char* file, int line) const {
        if (level < minLevel) return;
        std::ofstream logFile("app.log", std::ios_base::app);
        logFile << timestamp() << " [" << levelToString(level) << "] "
            << "[" << file << ":" << line << "] " << msg << std::endl;
    }
    void log(LogLevel level, const std::ostringstream& msg, const char* file, int line) {
        log(level, msg.str(), file, line);
    }
private:
    Logger() : minLevel(LogLevel::INFO) {}
        LogLevel minLevel;
        std::string timestamp() const {
        std::time_t now = std::time(nullptr);
        char buf[32];
        std::tm tm_buf;
    #if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm_buf, &now); // Windows/MSVC
    #else
        localtime_r(&now, &tm_buf); // POSIX/Linux
    #endif
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
        return buf;    
    }

    std::string levelToString(LogLevel level) const {
        switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
        }
    }
};