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

namespace xop
{

enum Priority 
{
    DEBUG, STATE, INFO, WARNING, ERROR
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
	
private:
    Logger();
    void processEntries();

    std::atomic<bool> _shutdown;
    std::thread _thread;
    std::mutex _mutex;
    std::condition_variable _cond;
    std::queue<std::string> _queue;
    std::ofstream _ofs;
};

#define LOG_DEBUG(fmt, ...) Logger::instance().log(DEBUG, __FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Logger::instance().log(ERROR, __FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)

}

#endif

