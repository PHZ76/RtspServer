// PHZ
// 2018-5-15

#if defined(WIN32) || defined(_WIN32) 
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include "Logger.h"
#include <stdarg.h>
#include <iostream>
#include "Timestamp.h"

using namespace xop;

const char* Priority_To_String[] =
{
    "DEBUG",
    "CONFIG",
    "INFO",
    "WARNING",
    "ERROR"
};

Logger::Logger() 
    : _shutdown(false)
{
    _thread = std::thread(&Logger::run, this);
}

Logger& Logger::instance()
{
    static Logger s_logger;
    return s_logger;
}

Logger::~Logger()
{
    _shutdown = true;
    _cond.notify_all();

    _thread.join();
}

void Logger::setLogFile(char *pathname)
{
    _ofs.open(pathname);
    if (_ofs.fail()) 
    {
        std::cerr << "Failed to open logfile." << std::endl;
    }
}

void Logger::log(Priority priority, const char* __file, const char* __func, int __line, const char *fmt, ...)
{	
    char buf[2048] = {0};

    sprintf(buf, "[%s][%s:%s:%d] ", Priority_To_String[priority],  __file, __func, __line);
    va_list args;
    va_start(args, fmt);
    vsprintf(buf + strlen(buf), fmt, args);
    va_end(args);

    std::string entry(buf);
    std::unique_lock<std::mutex> lock(_mutex);	
    _queue.push(std::move(entry));
    _cond.notify_all(); 
}

void Logger::log2(Priority priority, const char *fmt, ...)
{
	char buf[4096] = { 0 };

	sprintf(buf, "[%s] ", Priority_To_String[priority]);  
	va_list args;
	va_start(args, fmt);
	vsprintf(buf + strlen(buf), fmt, args);
	va_end(args);

	std::string entry(buf);
	std::unique_lock<std::mutex> lock(_mutex); 
	_queue.push(std::move(entry));
	_cond.notify_all();
}

void Logger::run()
{
    std::unique_lock<std::mutex> lock(_mutex);

    while(!_shutdown) 
    {
        if(!_queue.empty())
        {
            if(_ofs.is_open() && (!_shutdown))
                _ofs << "[" << Timestamp::localtime() << "]" 					 
                     << _queue.front() << std::endl;		
            else
                std::cout << "[" << Timestamp::localtime() << "]" 
                      << _queue.front() << std::endl;		
            _queue.pop();
        }
        else
        {
            _cond.wait(lock);
        }
    }
}


