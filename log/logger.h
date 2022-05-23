#pragma once
#pragma warning(disable:4996)
#ifndef __LOGGER_H__
#define __LOGGER_H__
#include <sstream>
#include <iostream>
#include <mutex>
#include <ctime>
#include <locale>
#include <chrono>
#include <iomanip>
#include <unordered_map>
#include "base_def.h"

LOG_NAMESPACE_BEGIN

#ifdef ERROR
#undef ERROR
#endif //ERROR


enum class LogLevel
{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    FATAL = 4
};

enum class OutMethod
{
    CONSOLE = 0,
    FILE = 1,
    BOTH = 2
};

class GetCurrentTime
{
public:
    static std::string getNowTime()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        tm* nowTM = std::localtime(&t);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%F %T");
        std::string str = ss.str();
        return str;
    }
};

class MyStringStream : public std::stringstream
{
public:
    void clear()
    {
        std::stringstream::clear();
        std::stringstream::str("");
    }
};

class Logger;

class OutStream : public std::ostringstream
{
public:
    OutStream() = delete;
    OutStream& operator = (OutStream& rhs) = delete;
    ~OutStream();
private:
    OutStream(MyStringStream& outStream, LogLevel logLevel = LogLevel::DEBUG, LogLevel lowestLogLevel = LogLevel::DEBUG)
        :_outStream(outStream),
        _logLevel(logLevel),
        _lowestLogLevel(lowestLogLevel) {
    }

    OutStream(OutStream& rhs)
        :_outStream(rhs._outStream),
        _logLevel(rhs._logLevel),
        _lowestLogLevel(rhs._lowestLogLevel),
        _map(rhs._map) {
    }
private:
    friend class Logger;
    MyStringStream& _outStream;
    Log::LogLevel _logLevel;
    Log::LogLevel _lowestLogLevel;
    static std::mutex _m;
    std::unordered_map<Log::LogLevel, std::string> _map{ {Log::LogLevel::DEBUG, "[DEBUG]"},
                                                        {Log::LogLevel::WARNING, "[WARNING]"},
                                                        {Log::LogLevel::INFO, "[INFO]"},
                                                        {Log::LogLevel::ERROR, "[ERROR]"},
                                                        {Log::LogLevel::FATAL, "[FATAL]"} };
};

class Logger
{
public:
    Logger(const Logger&) = delete;
    Logger& operator = (const Logger& rhs) = delete;
    static Logger& getInstance();

public:
    OutStream operator () (LogLevel logLevel = LogLevel::DEBUG);
    void setLogLevel(LogLevel logLevel);
    bool setLogOutFile(const std::string filePath);
    void setOutMethod(OutMethod outMethod);
    std::string getOutFile() const;

private:
    Logger();
    ~Logger();
    struct Impl;
    std::shared_ptr<Impl> _implPtr;
};
LOG_NAMESPACE_END
#endif //__LOGGER_H__