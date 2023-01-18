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
                   const unsigned size, const int protection, const int map_flag) {
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
    a_mutex = (pthread_mutex_t *) open_memory(CRITICAL_SECTION, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR,
                                              sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED);

    int err;
    pthread_mutexattr_t attr;
    err = pthread_mutexattr_init(&attr); if (err) err_exit("eroare la mutexattr_init, %d", err);
    err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED); if (err) err_exit("eroare la mutexattr_setpshared, %d", err);
    pthread_mutex_init(a_mutex, &attr);

    return a_mutex;
}

void * shm_request;
void * shm_response;
void * x;
pthread_mutex_t * a_mutex;

//  Request memory from daemon
void * request_memory(unsigned long long nr_bytes){
    openlog("xalloc", LOG_PID, LOG_DAEMON);
    syslog(LOG_DAEMON | LOG_ERR, "pid: %d has requested memory", getpid());

    if (!a_mutex && !shm_request && !shm_response && !x) {
    a_mutex = (pthread_mutex_t *) open_memory(CRITICAL_SECTION, O_RDWR, S_IRUSR | S_IWUSR,
                                              sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED);

    shm_request = open_memory(MEM_REQUEST, O_RDWR, 0644,
                                         REQUEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED);
    //syslog(LOG_DAEMON | LOG_INFO, "proces %d requesting memory from daemon, mutex = %p", getpid(), shm_lock);
       
    shm_response = open_memory(MEM_RESPONSE, O_RDWR, 0644,
                                          RESPONSE_SIZE, PROT_READ, MAP_SHARED);

    x = open_memory(ALLOCATED_SPACE, O_RDWR, 0666,
                           getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED);
    }

    pid_t own_pid, response_pid;
    own_pid = getpid();

    if(pthread_mutex_lock(a_mutex) != 0) {
        printf("error in pthread_mutex_lock\n");
        err_exit("error in request_memory() at pthread_mutex_lock()");
    }
    //  START OF CRITIZAL SECTION
        syslog(LOG_DAEMON | LOG_INFO, "pid: %d has locked mutex", getpid());
        //  the caller proces sends its pid and the requiested bytes
        sprintf(shm_request, "%d %llu", own_pid, nr_bytes);

        unsigned long long response;
        while(1){   //  waiting for response
            sscanf(shm_response, "%d %llu", &response_pid, &response);
            syslog(LOG_DAEMON | LOG_ERR, "A: %d", response_pid);
            if(own_pid == response_pid){
                sprintf(shm_request, "-1 ");

                //  END OF CRITICAL SECTION
                if(pthread_mutex_unlock(a_mutex) != 0)
                    err_exit("error in request_memory() at pthread_mutex_unlock()");
                syslog(LOG_DAEMON | LOG_INFO, "pid: %d has UNlocked mutex", getpid());
                return x + response;
            }
            sleep(1);
        }
}


void free_memory(void * addr){
    unsigned long long offset = addr - x;
    syslog(LOG_DAEMON | LOG_ERR, "offset: %llu", offset);
    pid_t own_pid, response_pid;
    own_pid = getpid();

    if(pthread_mutex_lock(a_mutex) != 0) {
        printf("error in pthread_mutex_lock\n");
        err_exit("error in request_memory() at pthread_mutex_lock()");
    }
    //  START OF CRITIZAL SECTION
        syslog(LOG_DAEMON | LOG_INFO, "FREE_MEM pid: %d has locked mutex", getpid());
        //  the caller proces sends its pid and the requiested bytes
        sprintf(shm_request, "%d -%llu", own_pid, offset);

        int response;
        while(1){   //  waiting for response
            sscanf(shm_response, "%d %d", &response_pid, &response);
            syslog(LOG_DAEMON | LOG_ERR, "F: %d", response_pid);
            if(own_pid == response_pid && response < 0){
                sprintf(shm_request, "-1 ");

                //  END OF CRITICAL SECTION
                if(pthread_mutex_unlock(a_mutex) != 0)
                    err_exit("error in request_memory() at pthread_mutex_unlock()");
                syslog(LOG_DAEMON | LOG_INFO, "FREE_MEM pid: %d has UNlocked mutex", getpid());
                break;
            }
            sleep(1);
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

typedef struct linkedList{
    int start_index;
    int offset;
    struct linkedList * next;
}node;

node * head = NULL;

node * add_node(const int size){
    syslog(LOG_DAEMON | LOG_ERR, "Added size %d in array ---------------------", size);
    node * new;
    new = (node *) malloc(sizeof(node));
    if(new == NULL)
        err_exit("Bad malloc!");
    if(head == NULL){                       //  if the linkedList is empty
        new -> start_index = 0;
        new -> offset = size;
        head = new;

        head -> next = NULL;

        return new;
    }
    else{                                   //  if there is space on the left side of the vector
        if(size < head -> start_index){
            new -> start_index = 0;
            new -> offset = size;

            new -> next = head;
            head = new;

            return new;
        }
        else{
            node * x;            //  looking for first fit
            for(x = head; x -> next != NULL; x = x -> next){
                syslog(LOG_DAEMON | LOG_ERR, ";(");
                if(x -> start_index + x -> offset + size - 1 < x -> next -> start_index){
                    new -> start_index = x -> start_index + x -> offset;
                    new -> offset = size;

                    new -> next = x -> next;
                    x -> next = new;
                    
                    return new;
                }
            }
            if (x -> start_index + x -> offset + size < getpagesize() * 100) {
                new -> start_index = x -> start_index + x -> offset;
                new -> offset = size;

                new -> next = NULL;

                x -> next = new;
                syslog(LOG_DAEMON | LOG_ERR, ":) %d", x -> start_index + x -> offset + size);
                return new;
            }
        }
    }
    return NULL;
}

int delete_node(const int index){
    int size;
    if (head -> start_index == index) {
        node * new = head;
        head = head -> next;
        
        size = new -> offset;
        free(new);
        return size;
    }
    
    for (node * x = head; x -> next != NULL; x = x -> next)
        if (x -> next -> start_index == index) {
            node * new = x -> next;
            x -> next = x -> next -> next;
            
            size = new -> offset;
            free(new);
            return size;
        }
    return 0;
}

node * realloc_node(const int index){
    int size = delete_node(index);
    return add_node(size);
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
                                      REQUEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED);

    sprintf(shm_requests, "-1 ");

    void * shm_responses = open_memory(MEM_RESPONSE, O_RDWR | O_CREAT | O_TRUNC, 0644,
                                      REQUEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED);

    void * memory_ptr = open_memory(ALLOCATED_SPACE, O_RDWR | O_CREAT, 0644,
                                    pagesize * 100, PROT_READ, MAP_SHARED);

    create_mutex();
    //create_mutex();

    // run forever in the background
    pid_t pid;
    long long nr_bytes;
    while(1) {
        int ret = sscanf(shm_requests, "%d %lld", &pid, &nr_bytes);
        if(ret != EOF && ret >= 1 && pid != -1){
            syslog(LOG_DAEMON | LOG_INFO, "ret=%d pid=%d nr_bytes=%d, will write response for this pid", ret, pid, nr_bytes);
            if(nr_bytes > 0){
                node * x = add_node(nr_bytes);
                if (x == NULL)
                    err_exit("OUT OF MEM!");
                sprintf(shm_responses, "%d %d", pid, x -> start_index);
            }
            else{
                syslog(LOG_DAEMON | LOG_INFO, "dealloc mem!", ret, pid);
                delete_node(-1 * nr_bytes);
                sprintf(shm_responses, "%d -1", pid);
            }
        }
        sleep(1);
    }

    closelog();
    exit(EXIT_FAILURE);
}
