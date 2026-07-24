#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void run_child(const char *binary) {
    printf("[Child] PID: %d\n", getpid());
    printf("[Child] Parent PID: %d\n", getppid());
    printf("[Child] Executing: %s\n", binary);

    char *args[] = {(char *)binary, NULL};
    char *env[] = {NULL};

    execve(binary, args, env);

    /* If execve returns, it failed */
    perror("execve failed");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <binary>\n", argv[0]);
        exit(1);
    }

    const char *binary = argv[1];
    pid_t child_pid;
    int status;

    printf("=== Sandbox Controller ===\n");
    printf("[Sandbox] PID: %d\n", getpid());
    printf("[Sandbox] Target binary: %s\n", binary);

    /* Fork child process */
    child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (child_pid == 0) {
        /* We are the child */
        run_child(binary);
    }

    /* We are the parent - supervise the child */
    printf("[Sandbox] Child process created with PID: %d\n", child_pid);
    printf("[Sandbox] Supervising child...\n");

    /* Wait for child to finish */
    waitpid(child_pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("[Sandbox] Child exited with status: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[Sandbox] Child killed by signal: %d\n", WTERMSIG(status));
    }

    printf("[Sandbox] Sandbox session complete\n");
    return 0;
}
