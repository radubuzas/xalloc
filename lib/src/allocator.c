#include "../include/allocator.h"

int start_allocator() {
	const char *LOGNAME = "xdaemon";

    // turn this process into a daemon
    int ret = become_daemon(0);
    if(ret) {
        syslog(LOG_USER | LOG_ERR, "error at starting daemon");
        closelog();
        return EXIT_FAILURE;
    }

    // we are now a daemon!
    // printf now will go to /dev/null

    // open up the system log
    openlog(LOGNAME, LOG_PID, LOG_DAEMON);
    syslog(LOG_DAEMON | LOG_INFO, "starting");

    // run forever in the background
    int count = 0;
    while(1) {
        sleep(15);
        count++;
        syslog(LOG_DAEMON | LOG_INFO, "running %d", count);
    }

    closelog();
}