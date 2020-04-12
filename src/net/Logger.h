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
	static Logger& instance();
	~Logger();

	void init(char *pathname = nullptr);
	void exit();

	void log(Priority priority, const char* __file, const char* __func, int __line, const char *fmt, ...);
	void log2(Priority priority, const char *fmt, ...);

private:
	void write(std::string buf);
	Logger();

	std::mutex _mutex;
	std::ofstream _ofs;
};
 
}

#ifdef _DEBUG
#define LOG_DEBUG(fmt, ...) xop::Logger::instance().log(LOG_DEBUG, __FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif
#define LOG_INFO(fmt, ...) xop::Logger::instance().log2(LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) xop::Logger::instance().log(LOG_ERROR, __FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)

#endif

