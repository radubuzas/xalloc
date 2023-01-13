// For reference: https://sciencesoftcode.files.wordpress.com/2018/12/the-linux-programming-interface-michael-kerrisk-1.pdf#page=812

#ifndef BECOME_DAEMON_H
#define BECOME_DAEMON_H

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

// Bit-mask values for 'flags' argument of becomeDaemon()
#define BD_NO_CHDIR 01 // Don't chdir("/")
#define BD_NO_CLOSE_FILES 02 // Don't close all open files
#define BD_NO_REOPEN_STD_FDS 04 // Don't reopen stdin, stdout, and stderr to /dev/null
#define BD_NO_UMASK0 010 // Don't do a umask(0)
#define BD_MAX_CLOSE 8192 // Maximum file descriptors to close if sysconf(_SC_OPEN_MAX) is indeterminate

int become_daemon(int flags);

#endif
