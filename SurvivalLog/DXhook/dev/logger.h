#pragma once
#include <cstdarg>
#include <cstdio>
#include <string>
#include <windows.h>

class Logger {
public:
    enum class LogType {
        INFO,
        WARN,
        ERR,
        DEBUG,
    };

public:
    static void log(LogType level, const char* format, ...) {
        // 保存原始颜色
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO originalInfo;
        GetConsoleScreenBufferInfo(hConsole, &originalInfo);

        // 设置颜色和中文标签
        switch (level) {
        case LogType::INFO:
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
            printf("[INFO]\t");
            break;
        case LogType::WARN:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            printf("[WARING]\t");
            break;
        case LogType::ERR:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
            printf("[ERROR]\t");
            break;
        case LogType::DEBUG:
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            printf("[DEBUG]\t");
            break;
        }

        // 恢复默认颜色打印内容
        SetConsoleTextAttribute(hConsole, originalInfo.wAttributes);

        // 打印日志内容
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        // 换行并恢复原始颜色
        printf("\n");
        SetConsoleTextAttribute(hConsole, originalInfo.wAttributes);
    }

    // 支持模板占位符的格式化日志
    template<typename... Args>
    static void InfoFormat(const std::string& format, Args... args) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), format.c_str(), args...);
        log(LogType::INFO, "%s", buffer);
    }

    template<typename... Args>
    static void WarnFormat(const std::string& format, Args... args) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), format.c_str(), args...);
        log(LogType::WARN, "%s", buffer);
    }

    template<typename... Args>
    static void ErrorFormat(const std::string& format, Args... args) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), format.c_str(), args...);
        log(LogType::ERR, "%s", buffer);
    }

    template<typename... Args>
    static void DebugFormat(const std::string& format, Args... args) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), format.c_str(), args...);
        log(LogType::DEBUG, "%s", buffer);
    }
};

// 宏定义简化调用
#define LOG_ERROR(format, ...) Logger::log(Logger::LogType::ERR, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  Logger::log(Logger::LogType::WARN, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  Logger::log(Logger::LogType::INFO, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) Logger::log(Logger::LogType::DEBUG, format, ##__VA_ARGS__)
