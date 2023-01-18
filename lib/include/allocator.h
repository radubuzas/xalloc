#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include "become_daemon.h"

int start_allocator();
void * request_memory(unsigned long long);
void free_memory(void *);

#endif
