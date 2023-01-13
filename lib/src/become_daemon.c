#include "../include/become_daemon.h"

/* Returns 0 on success, -1 on error */
int become_daemon(int flags) {
    // This function will make the calling process a daemon process, following the 'double fork' method
    int maxfd, fd;

    // Perform a fork after which the parent exits, and the child continues.
    // 1. The terminal will notice the parent's exit so it will display another shell process, 
    //    and the child will run in the background
    // 2. The child process is guaranteed not to be a process group leader, so it can start a new session
    printf("Before first fork:\n");
    printf("pid=%d ppid=%d pgid=%d sid=%d\n", getpid(), getppid(), getpgid(getpid()), getsid(getpid()));
    pid_t pid = fork();
    if(pid < 0) // Fork failure
        return -1;
    if(pid > 0) // Parent terminates
        exit(EXIT_SUCCESS);
    
    printf("After first fork:\n");
    printf("pid=%d ppid=%d pgid=%d sid=%d\n", getpid(), getppid(), getpgid(getpid()), getsid(getpid()));
    // We start a new session so the child process isn't associated with a controlling terminal
    if(setsid() < 0) // Become leader of new session
        return -1;

    printf("After setsid:\n");
    printf("pid=%d ppid=%d pgid=%d sid=%d\n", getpid(), getppid(), getpgid(getpid()), getsid(getpid()));
    // After this second fork the child process won't be the session leader so it can never acquire a controlling terminal
    pid = fork();
    if(pid < 0)
        return -1;
    if(pid > 0)
        exit(EXIT_SUCCESS);
    
    printf("After second fork:\n");
    printf("pid=%d ppid=%d pgid=%d sid=%d\n", getpid(), getppid(), getpgid(getpid()), getsid(getpid()));
    // Clear the process umask, to ensure that, when the daemon creates files and directories, 
    // they have the requested permissions.
    if (!(flags & BD_NO_UMASK0))
        umask(0);

    // Change the process's current working directory to root. 
    // It should change to any file path that isn't unmounted during the daemon's execution
    if (!(flags & BD_NO_CHDIR))
        chdir("/");


    // Close all open file descriptors that the daemon has inherited from its parent.
    // Some file descriptors, such as 0, 1, 2 refer to the initial controlling terminal.
    if (!(flags & BD_NO_CLOSE_FILES)) { // Close all open files
        maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd == -1) // Limit is indeterminate...
            maxfd = BD_MAX_CLOSE; // so take a guess
        for (fd = 0; fd < maxfd; fd++)
            close(fd);
    }

    // Redirect I/O to /dev/null, which flushes everything it is written into it, 
    // This prevents a daemon failure in case I/O is done with corrupted or old file descriptors
    if (!(flags & BD_NO_REOPEN_STD_FDS)) {
        close(STDIN_FILENO); // Reopen standard fd's to /dev/null
        fd = open("/dev/null", O_RDWR);
        if (fd != STDIN_FILENO) // 'fd' should be 0
            return -1;
        if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -1;
        if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -1;
    }

    return 0;
}
