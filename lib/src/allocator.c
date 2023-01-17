#include "../include/allocator.h"

#define INTDIGITS 9
#define REQUEST_SIZE 64
#define RESPONSE_SIZE 64

const char *ALLOCATOR_PID_SHM = "xa_allocator_pid_shm"; //  here the allocator process will store its pid
const char *ALLOCATED_SPACE = "xalloc_shared_memory";   //  this is storing allocated data
const char *MEM_REQUEST = "xalloc_memory_request";      //  the shm_mem where we are gonna put the request
const char *CRITICAL_SECTION = "xalloc_mutex";          //  the mutex key will be accesible via shm
const char *MEM_RESPONSE = "xalloc_memory_response";    //  the proces that's accessing the critical section will wait for approval!
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

void * open_memory(const char * shm_name, const int open_flag, const int open_mode,
                 const unsigned size, const int protection, const int map_flag){
    int fd = shm_open(shm_name, open_flag, (mode_t) open_mode);
    if (fd == -1)
        err_exit("error in request_memory at shm_open \"%s\"", shm_name);

    if(ftruncate(fd, size) == -1)
        err_exit("error in open_memory at ftruncate \"%s\"", shm_name);

    void * shm =  mmap(NULL, size,
                       protection, map_flag,
                       fd, 0);

    if(shm == MAP_FAILED)
        err_exit("error in request_memory at mmap() \"%s\"", shm_name);

    if(close(fd) == -1)
        err_exit("error in request_memory at close() \"%s\"", shm_name);
    return shm; 
}

pthread_mutex_t * create_mutex(){
    pthread_mutex_t * a_mutex;
    a_mutex = (pthread_mutex_t *) open_memory(CRITICAL_SECTION, O_RDWR | O_CREAT | O_TRUNC, 0666,
                                              sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED);

    pthread_mutex_init(a_mutex, NULL);
    return a_mutex;
}

//  Request memory from daemon
void * request_memory(unsigned long long nr_bytes){
    openlog("xalloc", LOG_PID, LOG_DAEMON);
    syslog(LOG_DAEMON | LOG_ERR, "pid: %d has requested memory", getpid());

    pthread_mutex_t * a_mutex = create_mutex();
    /*
    a_mutex = (pthread_mutex_t *) open_memory(CRITICAL_SECTION, O_RDWR, 0644,
                                              sizeof(pthread_mutex_t), PROT_READ, MAP_SHARED);
*/
    pthread_mutex_lock(a_mutex);
    //  START OF CRITIZAL SECTION
        syslog(LOG_DAEMON | LOG_ERR, "pid: %d has locked mutex", getpid());
        void * shm_request = open_memory(MEM_REQUEST, O_RDWR, 0644,
                                         REQUEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED);
        //syslog(LOG_DAEMON | LOG_INFO, "proces %d requesting memory from daemon, mutex = %p", getpid(), shm_lock);
       
        void * shm_response = open_memory(MEM_RESPONSE, O_RDWR, 0644,
                                          RESPONSE_SIZE, PROT_READ, MAP_SHARED);

        void * x = open_memory(ALLOCATED_SPACE, O_RDWR, 0666,
                               getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED);

        pid_t own_pid, response_pid;
        own_pid = getpid();
        //  the caller proces sends its pid and the requiested bytes
        sprintf(shm_request, "%d %lld", own_pid, nr_bytes);

        unsigned long long response;
        while(1){   //  waiting for response
            sscanf(shm_response, "%d %llu", &response_pid, &response);
            syslog(LOG_DAEMON | LOG_ERR, "A: %d", response_pid);
            if(own_pid == response_pid){
                syslog(LOG_DAEMON | LOG_ERR, "pid: %d has received memory", getpid());
                sprintf(shm_request, "-1 ");

                //  END OF CRITICAL SECTION
                pthread_mutex_unlock(a_mutex);

                syslog(LOG_DAEMON | LOG_ERR, "pid: %d has UNlocked mutex", getpid());
                return x + response;
            }
        }
}

void get_block_of_memory(){
    void * shm = open_memory(ALLOCATED_SPACE, O_RDWR, 0644, 
                             getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED);
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

    // open up the system log
    openlog(LOGNAME, LOG_PID, LOG_DAEMON);

    // turn this process into a daemon
    if( become_daemon(0) ){
        syslog(LOG_USER | LOG_ERR, "error at starting daemon");
        closelog();
        return EXIT_FAILURE;
    }

    // we are now a daemon!
    // printf now will go to /dev/null

    if( !another_allocator_exists() ) {
        syslog(LOG_DAEMON | LOG_INFO, "no other running allocator found, this allocator will start");
        write_allocator_pid();
    }
    else {
        err_exit("found another allocator process running!");
    }

    //  openning a chunck of memory
    //  at first only 1 page
    const int pagesize = getpagesize();
    
    void * shm_requests = open_memory(MEM_REQUEST, O_RDWR | O_CREAT | O_TRUNC, 0644,
                                      REQUEST_SIZE, PROT_READ, MAP_SHARED);

    void * shm_responses = open_memory(MEM_RESPONSE, O_RDWR | O_CREAT | O_TRUNC, 0644,
                                      REQUEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED);

    void * memory_ptr = open_memory(ALLOCATED_SPACE, O_RDWR | O_CREAT, 0644,
                                    pagesize, PROT_READ, MAP_SHARED);

    pthread_mutex_t * mutex = create_mutex();
    //create_mutex();

    // run forever in the background
    pid_t pid;
    unsigned long long nr_bytes;
    while(1) {
        sscanf(shm_requests, "%d %lld", &pid, &nr_bytes);
        syslog(LOG_USER | LOG_ERR, "R: %d", pid);
        if(pid != -1){
            sprintf(shm_responses, "%d %lld", pid, 0);
        }
        sleep(0.01);
    }

    closelog();
    exit(EXIT_FAILURE);
}
