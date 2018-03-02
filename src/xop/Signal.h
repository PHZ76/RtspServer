#ifndef XOP_SINNAL_H
#define XOP_SINNAL_H

#if defined(__linux) || defined(__linux__) 
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#endif
//signal(SIGPIPE, SIG_IGN);

class Signal
{
public:	
	static void ignoreSignal()
	{
#if defined(__linux) || defined(__linux__) 
		signal(SIGPIPE, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		signal(SIGUSR1, SIG_IGN);
		signal(SIGTERM, SIG_IGN);
		signal(SIGKILL, SIG_IGN);
#endif
	}
};

#endif

