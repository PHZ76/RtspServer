// PHZ
// 2018-5-15

#ifndef XOP_SOCKET_UTIL_H
#define XOP_SOCKET_UTIL_H

#include "Socket.h"
#include <string>

namespace xop
{
    
class SocketUtil
{
public:
    static bool Bind(SOCKET sockfd, std::string ip, uint16_t port);
    static void SetNonBlock(SOCKET fd);
    static void SetBlock(SOCKET fd, int write_timeout=0);
    static void SetReuseAddr(SOCKET fd);
    static void SetReusePort(SOCKET sockfd);
    static void SetNoDelay(SOCKET sockfd);
    static void SetKeepAlive(SOCKET sockfd);
    static void SetNoSigpipe(SOCKET sockfd);
    static void SetSendBufSize(SOCKET sockfd, int size);
    static void SetRecvBufSize(SOCKET sockfd, int size);
    static std::string GetPeerIp(SOCKET sockfd);
    static std::string GetSocketIp(SOCKET sockfd);
    static int GetSocketAddr(SOCKET sockfd, struct sockaddr_in* addr);
    static uint16_t GetPeerPort(SOCKET sockfd);
    static int GetPeerAddr(SOCKET sockfd, struct sockaddr_in *addr);
    static void Close(SOCKET sockfd);
    static bool Connect(SOCKET sockfd, std::string ip, uint16_t port, int timeout=0);
};

}

#endif // _SOCKET_UTIL_H




