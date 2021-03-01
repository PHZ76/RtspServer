#ifndef _XOP_LOG_H
#define _XOP_LOG_H

#include <cstdio>

//#ifdef _DEBUG
#define LOG(format, ...)  	\
{								\
    fprintf(stderr, "[DEBUG] [%s:%s:%d] " format "", \
    __FILE__, __FUNCTION__ , __LINE__, ##__VA_ARGS__);     \
}
//#else
//#define LOG(format, ...)  	
//#endif 


#endif
