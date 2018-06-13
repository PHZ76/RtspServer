// PHZ
// 2018-5-15

#include "xop.h"
#if defined(WIN32) || defined(_WIN32) 
#include<windows.h>
#endif

#if defined(WIN32) || defined(_WIN32) 
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
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
#endif
    return true;
}

