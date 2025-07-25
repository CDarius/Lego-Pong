#ifndef __MESSAGE_LOGGER_HPP__
#define __MESSAGE_LOGGER_HPP__

#include <Arduino.h>

enum class LogLevel
{
    Error,
    Warning,
    Info
};

class Logger {
public:
    // Get the singleton instance
    static Logger& instance() {
        static Logger logger_instance;
        return logger_instance;
    }

    // Delete copy and move constructors and assignment operators
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    void logE(const char* message);
    void logE(const String& message);

    void logW(const char* message);
    void logW(const String& message);

    void logI(const char* message);
    void logI(const String& message);

private:
    // Private constructor to prevent instantiation
    Logger() = default;
    ~Logger() = default;
};

#endif