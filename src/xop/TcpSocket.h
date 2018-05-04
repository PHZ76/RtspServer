#ifndef _XOP_TCP_SOCKET_H
#define _XOP_TCP_SOCKET_H

#include <cstdint>
#include <string>
#include "Socket.h"

namespace xop
{
    class TcpSocket
    {
    public:
        TcpSocket(SOCKET sockfd);
        virtual ~TcpSocket();

        bool bind(std::string ip, uint16_t port);
        bool listen(int backlog);
        SOCKET accept();
        bool connect(std::string ip, uint16_t port);
        void close();
        void shutdownWrite();

        SOCKET fd() const { return _sockfd; }
        
    private:
        SOCKET _sockfd = -1;
    };
}

#endif


