#include <stdio.h>
#include <unistd.h>

int main() {
    printf("[TestBinary] Started - running infinite loop...\n");
    fflush(stdout);
    while(1) {
        sleep(1);
        printf("[TestBinary] Still running...\n");
        fflush(stdout);
    }
    return 0;
}
