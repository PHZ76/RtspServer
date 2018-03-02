#ifndef _XOP_PIPE_H
#define _XOP_PIPE_H

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

	int readfd() const { return _pipefd[0]; }
	int writefd() const { return _pipefd[1]; }
	
private:
	int _pipefd[2];
};

}

#endif
