// PHZ
// 2018-5-15

#ifndef XOP_PIPE_H
#define XOP_PIPE_H

#include "TcpSocket.h"

namespace xop
{
	
class Pipe
{
public:
    Pipe();
    bool create();
    int write(void *buf, int len);
    int read(void *buf, int len);
    void close();

    SOCKET readfd() const { return _pipefd[0]; }
    SOCKET writefd() const { return _pipefd[1]; }
	
private:
    SOCKET _pipefd[2];
};

}

#endif
