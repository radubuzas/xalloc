#include <syslog.h>
#include <stdio.h>
#include "lib/include/daemon.h"
#include "lib/include/become_daemon.h"

int main(int argc, char *argv[]) {
    // skeleton_daemon();

    int ret;
    const char *LOGNAME = "xdaemon";

    // turn this process into a daemon
    ret = become_daemon(0);
    if(ret)
    {
        syslog(LOG_USER | LOG_ERR, "error starting");
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
    while(1)
    {
        sleep(15);
        count++;
        syslog(LOG_DAEMON | LOG_INFO, "running %d", count);
    }

    return 0;
}
