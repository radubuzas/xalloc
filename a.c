#include "./lib/include/allocator.h"
#include <sys/types.h>
       #include <sys/wait.h>

int main(){
    int n;

    pid_t pid = fork();
    char *p = (char *) request_memory(1000);
    free_memory(p);
    for(int i = 0; i < 1000/8 ; ++i)
        p[i] = 'a';
    printf("p=%p pid=%d ppid=%d\n", p, getpid(), getppid());
    wait(NULL);
    return 0;
}
