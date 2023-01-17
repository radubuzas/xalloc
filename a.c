#include "./lib/include/allocator.h"
#include <sys/types.h>
       #include <sys/wait.h>

int main(){
    int n;

    pid_t pid = fork();
    void *p = request_memory(1000);
    printf("p=%p pid=%d ppid=%d\n", p, getpid(), getppid());
    wait(NULL);
    return 0;
}
