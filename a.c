#include "./lib/include/allocator.h"
#include <sys/types.h>
       #include <sys/wait.h>

int main(){
    int n;

    pid_t pid = fork();
    request_memory(1);
    if(pid > 0)
        wait(NULL);


    return 0;
}
