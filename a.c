#include "./lib/include/allocator.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <memory.h>

int main(){
    pid_t pid = fork();
    char *p = (char *) request_memory(1000);
    printf("p=%p\n", p);
    free_memory(p);
    // for(int i = 0; i < 1000/8 ; ++i)
    //     p[i] = 'a';
    printf("p=%p pid=%d ppid=%d\n", p, getpid(), getppid());

    if(pid > 0) {
        wait(NULL);
        p = request_memory(100);
        sprintf(p, "Testez-testez-si-iar-testez");
        char *q = request_memory(100);
        printf("p=%p q=%p\n", p, q);
        sscanf(p, "%s", q);
        printf("q=%s\n", q);
    }
    if(pid == 0) {
        char *v = request_memory(500);
        printf("v=%p\n", v);
        int cf = 0;
        for(int i = 1; i <= 100; i++) {
            sprintf(v + cf, "%d ", i);
            if(i < 10) cf += 2;
            else if(i < 100) cf += 3;
            else cf += 4;
        }
        int *x = request_memory(4);
        printf("x=%p\n", x);
        cf = 0;
        for(int i = 1; i <= 100; i++) {
            sscanf(v + cf, "%d ", x);
            printf("%d ", *x);
            if(i < 10) cf += 2;
            else if(i < 100) cf += 3;
            else cf += 4;
        }
    }

    printf("pid %d will terminate\n", getpid());

    return 0;
}
