#include "./lib/include/allocator.h"
#include <sys/types.h>
       #include <sys/wait.h>

int main(){
    pid_t pid = fork();
    if(request_memory(1))
        printf("%d\n", pid);
    return 0;
}
