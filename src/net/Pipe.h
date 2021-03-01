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
	bool  Create();
	int   Write(void *buf, int len);
	int   Read(void *buf, int len);
	void  Close();

	SOCKET Read() const { return pipe_fd_[0]; }
	SOCKET Write() const { return pipe_fd_[1]; }
	
private:
	SOCKET pipe_fd_[2];
};

}

#endif
