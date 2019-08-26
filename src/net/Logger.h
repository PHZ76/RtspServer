// PHZ
// 2018-5-15

#ifndef XOP_LOGGER_H
#define XOP_LOGGER_H

#include <queue>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <fstream>
#include <cstring>
#include <iostream>
#include <sstream>

namespace xop
{

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

    void setLogFile(char *pathname);
    void log(Priority priority, const char* __file, const char* __func, int __line, const char *fmt, ...);
	void log2(Priority priority, const char *fmt, ...);

private:
    Logger();
    void run();

    std::atomic<bool> _shutdown;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::queue<std::string> _queue;
    std::ofstream _ofs;
};

}
#ifdef _DEBUG
#define LOG_DEBUG(fmt, ...) xop::Logger::instance().log(xop::LOG_DEBUG, __FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif
#define LOG_INFO(fmt, ...) xop::Logger::instance().log2(xop::LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) xop::Logger::instance().log(xop::LOG_ERROR, __FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)

#endif

