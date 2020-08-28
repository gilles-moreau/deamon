#ifndef _XSIGNAL_H
#define _XSIGNAL_H
#include <signal.h>

typedef void SigFunc(int);

SigFunc *xsignal(int signo, SigFunc *f);

#endif
