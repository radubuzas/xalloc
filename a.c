#include "./lib/include/allocator.h"
#include <sys/types.h>
#include <sys/wait.h>

int main(){
    pid_t pid = fork();

    if(pid > 0) {
        wait(NULL);
        char *p = request_memory(100);
        sprintf(p, "Testez-testez-si-iar-testez");
        char *q = request_memory(100);
        printf("p=%p q=%p\n", p, q);
        sscanf(p, "%s", q);
        printf("q=%s\n", q);
    }
    else {
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

    return 0;
}
