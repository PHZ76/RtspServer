// PHZ
// 2018-5-15

#include "Pipe.h"
#include "SocketUtil.h"
#include <random>

using namespace xop;

Pipe::Pipe()
{

}

bool Pipe::create()
{
#if defined(WIN32) || defined(_WIN32) 
    TcpSocket rp(socket(AF_INET, SOCK_STREAM, 0)), wp(socket(AF_INET, SOCK_STREAM, 0));
    std::random_device rd;

    _pipefd[0] = rp.fd();
    _pipefd[1] = wp.fd();
    uint16_t port = 0;
    int again = 5;

    while(again--)
    {
        port = rd(); // random
        if (rp.bind("127.0.0.1", port))
            break;
    }
    if (again == 0)
        return false;

    if (!rp.listen(1))
        return false;
    if (!wp.connect("127.0.0.1", port))
        return false;
    if ((_pipefd[0] = rp.accept()) < 0)
        return false;

    SocketUtil::setNonBlock(_pipefd[0]);
    SocketUtil::setNonBlock(_pipefd[1]);
#elif defined(__linux) || defined(__linux__) 
	if (pipe2(_pipefd, O_NONBLOCK | O_CLOEXEC) < 0)
	{
		return false;
	}
#endif
    return true;
}

int Pipe::write(void *buf, int len)
{
#if defined(WIN32) || defined(_WIN32) 
    return ::send(_pipefd[1], (char *)buf, len, 0);
#elif defined(__linux) || defined(__linux__) 
    return ::write(_pipefd[1], buf, len);
#endif 
}

int Pipe::read(void *buf, int len)
{
#if defined(WIN32) || defined(_WIN32) 
    return recv(_pipefd[0], (char *)buf, len, 0);
#elif defined(__linux) || defined(__linux__) 
    return ::read(_pipefd[0], buf, len);
#endif 
}

void Pipe::close()
{
#if defined(WIN32) || defined(_WIN32) 
    closesocket(_pipefd[0]);
    closesocket(_pipefd[1]);
#elif defined(__linux) || defined(__linux__) 
    ::close(_pipefd[0]);
    ::close(_pipefd[1]);
#endif

}


