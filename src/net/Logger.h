// PHZ
// 2020-5-15

#ifndef XOP_LOGGER_H
#define XOP_LOGGER_H

#include <string>
#include <mutex>
#include <thread>
#include <fstream>
#include <cstring>
#include <iostream>
#include <sstream>

namespace xop {


enum Priority 
{
    LOG_DEBUG, LOG_STATE, LOG_INFO, LOG_WARNING, LOG_ERROR,
};	
	
class Logger
{
public:
	Logger &operator=(const Logger &) = delete;
	Logger(const Logger &) = delete;	
	static Logger& Instance();
	~Logger();

	void Init(char *pathname = nullptr);
	void Exit();

	void Log(Priority priority, const char* __file, const char* __func, int __line, const char *fmt, ...);
	void Log2(Priority priority, const char *fmt, ...);

private:
	void Write(std::string buf);
	Logger();

	std::mutex mutex_;
	std::ofstream ofs_;
};
 
}

#ifdef _DEBUG
#define LOG_DEBUG(fmt, ...) xop::Logger::Instance().Log(LOG_DEBUG, __FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif
#define LOG_INFO(fmt, ...) xop::Logger::Instance().Log2(LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) xop::Logger::Instance().Log(LOG_ERROR, __FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)

#endif

