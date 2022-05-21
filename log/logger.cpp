#include "logger.h"
#include <memory>
#include <fstream>
#include <algorithm>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include <boost/foreach.hpp>

LOG_NAMESPACE_BEGIN

std::mutex OutStream::_m;

class JudgeFile
{
public:
	bool operator()(const std::string& filePath)
	{
		bool res = false;
		do
		{
			std::fstream fs{ filePath, std::ios_base::in | std::ios_base::out };
			if (!fs)
			{
				std::ofstream fOut{ filePath };
				if (!fOut)
				{
					break;
				}
				fOut.close();
				res = true;
				break;
			}
			fs.close();
			res = true;
		} while (false);
		return res;
	}
};

struct Logger::Impl
{
	struct OutInfo
	{
		OutInfo() = default;
		OutInfo(const OutInfo& rhs) = delete;
		OutInfo& operator = (const OutInfo&) = delete;
		/*������־����buffer*/
		MyStringStream _outStreamBuffer;
		OutMethod         _outMethod{ OutMethod::CONSOLE };
		/*��������־·��*/
		std::string       _outFilePath;
	};
	struct ConfReader
	{
		static void parse(Impl& impl, const std::string& path)
		{
			boost::property_tree::ptree json_root;
			boost::property_tree::read_json<boost::property_tree::ptree>(path, json_root);
			const boost::property_tree::ptree& logConf = json_root.get_child("LOG");
			size_t flushTime = logConf.get<int>("flush time");
			std::string logLevel = logConf.get<std::string>("log level");
			std::string logMethod = logConf.get<std::string>("log method");
			std::string logPath = logConf.get<std::string>("log path");

			transform(logLevel.begin(), logLevel.end(), logLevel.begin(), toupper);
			transform(logMethod.begin(), logMethod.end(), logMethod.begin(), toupper);

			impl._time = flushTime;
			impl._outInfo._outFilePath = logPath;
			impl._outInfo._outMethod = _logMethodMap[logMethod];
			impl._logLevel = _logLevelMap[logLevel];

		}

		static std::unordered_map<std::string, Log::LogLevel> _logLevelMap;
		static std::unordered_map<std::string, Log::OutMethod> _logMethodMap;
	};

public:
	Impl(Log::LogLevel logLevel = LogLevel::DEBUG, bool outStop = false);
	Impl& operator = (const Impl&) = delete;
	~Impl();

public:
	void flush();

public:
	OutInfo			_outInfo;
	friend class OutStream;
	Log::LogLevel   _logLevel;
	std::mutex      _m;

	std::condition_variable _condition;
	std::atomic<bool> _outStop = false;
	std::atomic<int>  _time = 100;
	std::thread _outThread;
};

std::unordered_map<std::string, Log::LogLevel> Logger::Impl::ConfReader::_logLevelMap = { {"DEBUG", Log::LogLevel::DEBUG},
																						{ "WARNING", Log::LogLevel::WARNING },
																						{ "INFO", Log::LogLevel::INFO },
																						{ "ERROR", Log::LogLevel::ERROR },
																						{ "FATAL", Log::LogLevel::FATAL } };

std::unordered_map<std::string, Log::OutMethod> Logger::Impl::ConfReader::_logMethodMap = { {"CONSOLE", Log::OutMethod::CONSOLE},
																							{"FILE", Log::OutMethod::FILE },
																							{"BOTH", Log::OutMethod::BOTH} };
Logger::Impl::Impl(Log::LogLevel logLevel, bool outStop)
{
	this->_logLevel = logLevel;
	_outStop = outStop;
	this->_outInfo._outStreamBuffer.clear();
	this->_outInfo._outStreamBuffer.clear();
	this->_outInfo._outFilePath.clear();
	std::string path = "C:\\Users\\86107\\Desktop\\test.json";
	ConfReader::parse(*this, path);
	this->_outThread = std::thread([this]() {
		while (!_outStop)
		{
			std::unique_lock<std::mutex> ulm(this->_m);
			_condition.wait_for(ulm, std::chrono::milliseconds(_time));
			this->flush();
			ulm.unlock();
		}
		});
}

Logger::Impl::~Impl()
{
	this->_time = 0;
	this->_condition.notify_all();
	this->_outStop = true;
	this->_outThread.join();
}

void Logger::Impl::flush()
{
	switch (this->_outInfo._outMethod)
	{
	case OutMethod::CONSOLE:
	{
		std::cout << this->_outInfo._outStreamBuffer.str();
		this->_outInfo._outStreamBuffer.clear();
		break;
	}
	case OutMethod::FILE:
	{
		if (!this->_outInfo._outFilePath.empty())
		{
			std::ofstream fileWrite(this->_outInfo._outFilePath, std::ofstream::app);
			fileWrite << this->_outInfo._outStreamBuffer.str();
			this->_outInfo._outStreamBuffer.clear();
		}
		break;
	}
	case OutMethod::BOTH:
	{
		std::cout << this->_outInfo._outStreamBuffer.str();
		if (!this->_outInfo._outFilePath.empty())
		{
			std::ofstream fileWrite(this->_outInfo._outFilePath, std::ofstream::app);
			fileWrite << this->_outInfo._outStreamBuffer.str();
			this->_outInfo._outStreamBuffer.clear();
		}
		this->_outInfo._outStreamBuffer.clear();
		break;
	}
	}
}


OutStream::~OutStream()
{
	std::string logContent{ this->str() };
	if (_logLevel < _lowestLogLevel) return;
	if (!logContent.empty())
	{
		std::string currentTime = "[" + GetCurrentTime::getNowTime() + "]";
		std::lock_guard<std::mutex> lgm(this->_m);
		_outStream << _map[_logLevel] << currentTime << ":" << this->str() << std::endl;
	}
}

Logger& Logger::getInstance()
{
	static Logger _singleton;
	return _singleton;
}
Logger::Logger()
{
	_implPtr = std::make_shared<Logger::Impl>();
}

Logger::~Logger()
{
}

OutStream Logger::operator () (LogLevel logLevel)
{
	OutStream outStream{ this->_implPtr->_outInfo._outStreamBuffer, logLevel, this->_implPtr->_logLevel };
	return outStream;
}

void Logger::setLogLevel(LogLevel logLevel)
{
	std::lock_guard<std::mutex> lgm(this->_implPtr->_m);
	this->_implPtr->_logLevel = logLevel;
}

bool Logger::setLogOutFile(const std::string filePath)
{
	std::lock_guard<std::mutex> lgm(this->_implPtr->_m);
	if (this->_implPtr->_outInfo._outMethod < Log::OutMethod::FILE)
	{
		return false;
	}
	JudgeFile judgeObj;
	if (!judgeObj(filePath))
	{
		return false;
	}
	this->_implPtr->_outInfo._outFilePath = filePath;
	return true;
}

void Logger::setOutMethod(OutMethod outMethod)
{
	std::lock_guard<std::mutex> lgm(this->_implPtr->_m);
	this->_implPtr->_outInfo._outMethod = outMethod;
}

std::string Logger::getOutFile() const
{
	std::lock_guard<std::mutex> lgm(this->_implPtr->_m);
	return this->_implPtr->_outInfo._outFilePath;
}
LOG_NAMESPACE_END