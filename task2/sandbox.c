#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>

#define TIME_LIMIT 10
#define MONITOR_INTERVAL 1

/* Shared state between threads - using atomic for safety */
atomic_int child_running = 1;
pid_t child_pid = -1;

/* Log file */
FILE *logfile;

/* Mutex for log writing */
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(const char *message) {
    pthread_mutex_lock(&log_mutex);
    if (logfile) {
        fprintf(logfile, "%s\n", message);
        fflush(logfile);
    }
    printf("%s\n", message);
    pthread_mutex_unlock(&log_mutex);
}

void timeout_handler(int sig) {
    if (child_pid > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg),
            "[Sandbox] TIME LIMIT EXCEEDED (%d seconds) - Killing PID: %d",
            TIME_LIMIT, child_pid);
        write_log(msg);
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

/* Thread 1: Monitor execution time */
void *time_monitor(void *arg) {
    int elapsed = 0;
    char msg[256];

    while (atomic_load(&child_running)) {
        sleep(MONITOR_INTERVAL);
        elapsed += MONITOR_INTERVAL;

        snprintf(msg, sizeof(msg),
            "[TimeMonitor] Elapsed: %d seconds / %d seconds limit",
            elapsed, TIME_LIMIT);
        write_log(msg);

        if (elapsed >= TIME_LIMIT) {
            write_log("[TimeMonitor] Time limit reached - signaling termination");
            if (child_pid > 0) {
                kill(child_pid, SIGKILL);
            }
            break;
        }
    }

    write_log("[TimeMonitor] Thread exiting");
    return NULL;
}

/* Thread 2: Monitor CPU and memory usage */
void *resource_monitor(void *arg) {
    char path[64];
    char line[256];
    char msg[512];
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/status", child_pid);

    while (atomic_load(&child_running)) {
        sleep(MONITOR_INTERVAL);

        fp = fopen(path, "r");
        if (!fp) break;

        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "VmRSS:", 6) == 0 ||
                strncmp(line, "VmSize:", 7) == 0) {
                line[strcspn(line, "\n")] = 0;
                snprintf(msg, sizeof(msg),
                    "[ResourceMonitor] %s", line);
                write_log(msg);
            }
        }
        fclose(fp);
    }

    write_log("[ResourceMonitor] Thread exiting");
    return NULL;
}

/* Thread 3: Monitor process status */
void *status_monitor(void *arg) {
    char path[64];
    char msg[256];

    snprintf(path, sizeof(path), "/proc/%d/status", child_pid);

    while (atomic_load(&child_running)) {
        sleep(MONITOR_INTERVAL * 2);

        if (access(path, F_OK) != 0) {
            write_log("[StatusMonitor] Child process no longer exists");
            break;
        }

        snprintf(msg, sizeof(msg),
            "[StatusMonitor] Child PID %d is still running", child_pid);
        write_log(msg);
    }

    write_log("[StatusMonitor] Thread exiting");
    return NULL;
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
    pthread_t tid_time, tid_resource, tid_status;

    /* Open log file */
    logfile = fopen("sandbox.log", "w");
    if (!logfile) {
        perror("fopen logfile");
        exit(1);
    }

    printf("=== Sandbox Controller ===\n");
    printf("[Sandbox] PID: %d\n", getpid());
    printf("[Sandbox] Target binary: %s\n", binary);
    printf("[Sandbox] Time limit: %d seconds\n", TIME_LIMIT);

    signal(SIGALRM, timeout_handler);
    clock_gettime(CLOCK_MONOTONIC, &start);

    child_pid = fork();

    if (child_pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (child_pid == 0) {
        run_child(binary);
    }

    printf("[Sandbox] Child PID: %d\n", child_pid);
    write_log("[Sandbox] Starting concurrent monitoring threads");

    /* Start monitoring threads */
    pthread_create(&tid_time, NULL, time_monitor, NULL);
    pthread_create(&tid_resource, NULL, resource_monitor, NULL);
    pthread_create(&tid_status, NULL, status_monitor, NULL);

    alarm(TIME_LIMIT);

    /* Wait for child */
    waitpid(child_pid, &status, 0);

    /* Signal threads to stop */
    atomic_store(&child_running, 0);
    alarm(0);

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) +
              (end.tv_nsec - start.tv_nsec) / 1e9;

    char msg[256];
    snprintf(msg, sizeof(msg),
        "[Sandbox] Total execution time: %.3f seconds", elapsed);
    write_log(msg);

    if (WIFEXITED(status)) {
        snprintf(msg, sizeof(msg),
            "[Sandbox] Child exited normally with status: %d",
            WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        snprintf(msg, sizeof(msg),
            "[Sandbox] Child killed by signal: %d (%s)",
            WTERMSIG(status),
            WTERMSIG(status) == SIGKILL ? "SIGKILL - Time limit enforced" : "Other signal");
    }
    write_log(msg);

    /* Wait for threads to finish */
    pthread_join(tid_time, NULL);
    pthread_join(tid_resource, NULL);
    pthread_join(tid_status, NULL);

    write_log("[Sandbox] All monitoring threads stopped");
    write_log("[Sandbox] Sandbox session complete");

    fclose(logfile);
    printf("[Sandbox] Log saved to sandbox.log\n");
    return 0;
}
