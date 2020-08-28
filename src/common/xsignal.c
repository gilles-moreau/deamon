#include <signal.h>

#include "src/common/xsignal.h"
#include "src/common/logs.h"

SigFunc *xsignal(int signo, SigFunc *f)
{
	struct sigaction sa, sa_old;

	sa.sa_handler = f;
	sigemptyset(&sa.sa_mask);
	sigaddset(&sa.sa_mask, signo);
	sa.sa_flags = 0;
	if (sigaction(signo, &sa, &sa_old) < 0)
		error("xsignal(%d): failed", signo);
	return (sa_old.sa_handler);
}
