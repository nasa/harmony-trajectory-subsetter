#ifndef LogLevel_H
#define LogLevel_H

#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <string>

// Define Log Levels
enum class LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger
{
  public:
    // Get the single instance of the Logger
    static Logger &getInstance()
    {
        static Logger instance;
        return instance;
    }

    // Set the log level to display
    void setLogLevel(std::string level)
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::map<std::string, LogLevel> logLevelMap = {
            {"DEBUG", LogLevel::DEBUG},
            {"INFO", LogLevel::INFO},
            {"WARNING", LogLevel::WARNING},
            {"ERROR", LogLevel::ERROR},
            {"CRITICAL", LogLevel::CRITICAL}};

        currentLogLevel = logLevelMap[level];
    }

    // Set the output stream (e.g., &std::cout, &fileStream)
    void setOutputStream(std::ostream &fileStream)
    {
        std::lock_guard<std::mutex> lock(mtx);
        outputStream = &fileStream;
    }

    // Set the output stream (e.g., &std::cout, &fileStream)
    void openLogFile(const std::string logFile)
    {
        std::stringstream stream;
        logOutFile.open(logFile.c_str(),
                        std::ios_base::out | std::ios_base::trunc);
        if (logOutFile.is_open())
        {
            // Set logging to file
            outputStream = &logOutFile;

            stream << "Logger::openLogFile(): Log File: " << logFile;
            debug(stream.str());
        }
        else
        {
            stream << "Subset::process_args(): ERROR: Could not open log file:"
                   << logFile << ", logging to console";
            error(stream.str());
        }
    }

    // Set the output stream (e.g., &std::cout, &fileStream)
    void closeLogFile()
    {
        if (logOutFile.is_open())
        {
            logOutFile.close();
        }
    }

    // Log a debug message
    void debug(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(mtx);
        // Checks currentLogLevel is less than or equal to DEBUG
        if (currentLogLevel <= LogLevel::DEBUG)
        {
            *outputStream << "[DEBUG] " << message << std::endl;
        }
    }

    // Log a debug message
    void info(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(mtx);
        // Checks currentLogLevel is less than or equal to INFO
        if (currentLogLevel <= LogLevel::INFO)
        {
            *outputStream << "[INFO] " << message << std::endl;
        }
    }

    void warning(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(mtx);
        // Checks currentLogLevel is less than or equal to WARNING
        if (currentLogLevel <= LogLevel::WARNING)
        {
            *outputStream << "[WARNING] " << message << std::endl;
        }
    }

    void error(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(mtx);
        // Checks currentLogLevel is less than or equal to ERROR
        if (currentLogLevel <= LogLevel::ERROR)
        {
            *outputStream << "[ERROR] " << message << std::endl;
        }
    }

    void critical(const std::string &message)
    {
        std::lock_guard<std::mutex> lock(mtx);
        // Checks currentLogLevel is less than or equal to CRITICAL
        if (currentLogLevel <= LogLevel::CRITICAL)
        {
            *outputStream << "[CRITICAL] " << message << std::endl;
        }
    }

  private:
    Logger() : currentLogLevel(LogLevel::INFO), outputStream(&std::cout) {}
    ~Logger() {}

    LogLevel currentLogLevel;
    std::ostream *outputStream;
    std::ofstream logOutFile;
    std::mutex mtx;
};

// Convenience macros for logging
#define SET_LOG_LEVEL(level) Logger::getInstance().setLogLevel(level)
#define SET_OUTPUT_STREAM(stream) Logger::getInstance().setOutputStream(stream)
#define OPEN_LOG_FILE(logFile) Logger::getInstance().openLogFile(logFile)
#define CLOSE_LOG_FILE() Logger::getInstance().closeLogFile()

// Macro to simplify logging with stream-like syntax
#define LOG_DEBUG(message)                                                     \
    do                                                                         \
    {                                                                          \
        std::stringstream stream;                                              \
        stream << message;                                                     \
        Logger::getInstance().debug(stream.str());                             \
    } while (0)

#define LOG_INFO(message)                                                      \
    do                                                                         \
    {                                                                          \
        std::stringstream stream;                                              \
        stream << message;                                                     \
        Logger::getInstance().info(stream.str());                              \
    } while (0)

#define LOG_WARNING(message)                                                   \
    do                                                                         \
    {                                                                          \
        std::stringstream stream;                                              \
        stream << message;                                                     \
        Logger::getInstance().warning(stream.str());                           \
    } while (0)

#define LOG_ERROR(message)                                                     \
    do                                                                         \
    {                                                                          \
        std::stringstream stream;                                              \
        stream << message;                                                     \
        Logger::getInstance().error(stream.str());                             \
    } while (0)

#define LOG_CRITICAL(message)                                                  \
    do                                                                         \
    {                                                                          \
        std::stringstream stream;                                              \
        stream << message;                                                     \
        Logger::getInstance().critical(stream.str());                          \
    } while (0)

#endif
