#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#define TIME_LIMIT 10  /* seconds */

pid_t child_pid = -1;

void timeout_handler(int sig) {
    if (child_pid > 0) {
        printf("\n[Sandbox] TIME LIMIT EXCEEDED (%d seconds)\n", TIME_LIMIT);
        printf("[Sandbox] Sending SIGKILL to child PID: %d\n", child_pid);
        kill(child_pid, SIGKILL);
    }
}

void run_child(const char *binary) {
    printf("[Child] PID: %d\n", getpid());
    printf("[Child] Parent PID: %d\n", getppid());
    printf("[Child] Executing: %s\n", binary);

    char *args[] = {(char *)binary, NULL};
    char *env[] = {NULL};

    execve(binary, args, env);

    perror("execve failed");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <binary>\n", argv[0]);
        exit(1);
    }

    const char *binary = argv[1];
    int status;
    struct timespec start, end;
    double elapsed;

    printf("=== Sandbox Controller ===\n");
    printf("[Sandbox] PID: %d\n", getpid());
    printf("[Sandbox] Target binary: %s\n", binary);
    printf("[Sandbox] Time limit: %d seconds\n", TIME_LIMIT);

    /* Set up alarm signal for time enforcement */
    signal(SIGALRM, timeout_handler);

    /* Record start time */
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Fork child process */
    child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (child_pid == 0) {
        run_child(binary);
    }

    /* Parent - set timer and supervise */
    printf("[Sandbox] Child process created with PID: %d\n", child_pid);
    printf("[Sandbox] Starting execution timer...\n");

    /* Set alarm - will fire after TIME_LIMIT seconds */
    alarm(TIME_LIMIT);

    /* Wait for child */
    waitpid(child_pid, &status, 0);

    /* Cancel alarm if child finished in time */
    alarm(0);

    /* Record end time */
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) +
              (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("[Sandbox] Execution time: %.3f seconds\n", elapsed);

    if (WIFEXITED(status)) {
        printf("[Sandbox] Child exited normally with status: %d\n", 
               WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[Sandbox] Child killed by signal: %d\n", WTERMSIG(status));
        if (WTERMSIG(status) == SIGKILL) {
            printf("[Sandbox] Reason: Time limit exceeded\n");
        }
    }

    printf("[Sandbox] Sandbox session complete\n");
    return 0;
}
