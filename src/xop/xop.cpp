#include "xop.h"
#if defined(WIN32) || defined(_WIN32) 
#include<windows.h>
#endif

bool XOP_Init()
{
#if defined(WIN32) || defined(_WIN32) 
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        WSACleanup();
        return false;
    }

    // ...

#endif
    return true;
}

