#include "Timestamp.h"
#include <iostream>
#include <iomanip>  
#include <sstream>

using namespace xop;
using namespace std;
using namespace std::chrono;

std::string Timestamp::Localtime()
{
    std::ostringstream stream;
    auto now = system_clock::now();
    time_t tt = system_clock::to_time_t(now);
	
#if defined(WIN32) || defined(_WIN32)
    struct tm tm;
    localtime_s(&tm, &tt);
    stream << std::put_time(&tm, "%F %T");
#elif  defined(__linux) || defined(__linux__) 
    char buffer[200] = {0};
    std::string timeString;
    std::strftime(buffer, 200, "%F %T", std::localtime(&tt));
    stream << buffer;
#endif	
    return stream.str();
}