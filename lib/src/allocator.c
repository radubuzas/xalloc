#include "../include/allocator.h"

#define INTDIGITS 9

const char *ALLOCATOR_PID_SHM = "xa_allocator_pid_shm"; // here the allocator process will store its pid

// Logs the value of errno along with any formatted message passed as argument.
void syslog_err(const char *format, va_list arg_list) {
    syslog(LOG_DAEMON | LOG_ERR, "errno=%d, error message is on the following line:", errno);
    vsyslog(LOG_DAEMON | LOG_ERR, format, arg_list);
}

// Terminates process with EXIT_FAILURE after calling syslog_err
void err_exit(const char *format, ...) {
    va_list arg_list;
    va_start(arg_list, format);
    syslog_err(format, arg_list);
    va_end(arg_list);
    syslog(LOG_DAEMON | LOG_INFO, "this allocator will terminate");
    exit(EXIT_FAILURE);
}

// Returns 0 if no allocator instance is running, 1 otherwise. Should be used only by allocator process.
static int another_allocator_exists() {
    syslog(LOG_DAEMON | LOG_INFO, "checking to see if there is another allocator instance running...");
    // open shared memory that contains last allocator's pid
    int fd = shm_open(ALLOCATOR_PID_SHM, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if(fd < 0)
        err_exit("error in another_allocator_esists() in shm_open() for memory object \"%s\"", ALLOCATOR_PID_SHM);
    
    struct stat buffer;
    if(fstat(fd, &buffer) < 0)
        err_exit("error in another_allocator_esists() in fstat()");

    if(buffer.st_size == 0) { // no pid was written to shared memory
        if(close(fd) < 0) 
            err_exit("error in another_allocator_esists() in close()");
        return 0;
    }

    char *addr = mmap(NULL, buffer.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED)
        err_exit("error in another_allocator_esists() in mmap()");

    pid_t pid;
    if(sscanf(addr, "%d", &pid) == EOF) // read pid from shared memory
        err_exit("error in another_allocator_esists() in sscanf()");
    syslog(LOG_DAEMON | LOG_INFO, "found old allocator pid = %d", pid);

    if(kill(pid, 0) < 0) // call fails so there is no process with this pid
        return 0;
    return 1; // call succeeds so there is another allocator running
}

// Writes allocator's pid in ALLOCATOR_PID_SHM shared memory. Returns -1 on error and 0 on success.
static int write_allocator_pid() {
    int fd = shm_open(ALLOCATOR_PID_SHM, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fd < 0)
        err_exit("error in write_allocator_pid() in shm_open() for memory object \"%s\"", ALLOCATOR_PID_SHM);
    if(ftruncate(fd, INTDIGITS) < 0)
        err_exit("error in write_allocator_pid() in ftruncate()");

    char *addr = mmap(NULL, INTDIGITS, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(addr == MAP_FAILED)
        err_exit("error in write_allocator_pid() in mmap()");

    pid_t pid = getpid();
    if(sprintf(addr, "%d", pid) < 0)
        err_exit("error in write_allocator_pid() in sprintf()");

    if(munmap(addr, INTDIGITS) < 0)
        err_exit("error in write_allocator_pid() in munmap()");
    return 0;
}

int start_allocator() {
	const char *LOGNAME = "xalloc";

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

    int exists = another_allocator_exists();
    if(!exists) {
        syslog(LOG_DAEMON | LOG_INFO, "no other running allocator found, this allocator will start");
        write_allocator_pid();
    }
    else {
        err_exit("found another allocator process running!");
    }

    // run forever in the background
    while(1) {
        sleep(15);
    }

    closelog();
}