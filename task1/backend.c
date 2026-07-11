#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#define SOCKET_PATH "/tmp/auth.sock"
#define MAX_LEN 256

#define VALID_USER "sugam"
#define VALID_PASS "secure123"

void drop_privileges() {
    uid_t nobody_uid = 65534;
    gid_t nobody_gid = 65534;

    printf("[Backend] Current UID before drop: %d\n", getuid());
    printf("[Backend] Current EUID before drop: %d\n", geteuid());

    if (setresgid(nobody_gid, nobody_gid, nobody_gid) < 0) {
        perror("setresgid failed");
        exit(1);
    }

    if (setresuid(nobody_uid, nobody_uid, nobody_uid) < 0) {
        perror("setresuid failed");
        exit(1);
    }

    if (geteuid() != nobody_uid) {
        fprintf(stderr, "[Backend] CRITICAL: Privilege drop failed!\n");
        exit(1);
    }

    printf("[Backend] Privileges dropped successfully\n");
    printf("[Backend] New UID: %d\n", getuid());
    printf("[Backend] New EUID: %d\n", geteuid());
}

int validate_credentials(const char *username, const char *password) {
    int user_ok = (strncmp(username, VALID_USER, MAX_LEN) == 0);
    int pass_ok = (strncmp(password, VALID_PASS, MAX_LEN) == 0);
    return user_ok && pass_ok;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_un addr;
    char buffer[MAX_LEN * 2 + 2];
    char username[MAX_LEN];
    char password[MAX_LEN];
    char *separator;

    printf("=== Authentication Backend ===\n");
    printf("[Backend] PID: %d\n", getpid());
    printf("[Backend] Starting as UID: %d\n", getuid());

    drop_privileges();

    unlink(SOCKET_PATH);

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_sock);
        exit(1);
    }

    chmod(SOCKET_PATH, 0600);

    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        exit(1);
    }

    printf("[Backend] Listening on %s\n", SOCKET_PATH);
    printf("[Backend] Waiting for authentication requests...\n\n");

    while (1) {
        client_sock = accept(server_sock, NULL, NULL);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        printf("[Backend] Received authentication request\n");

        memset(buffer, 0, sizeof(buffer));
        if (recv(client_sock, buffer, sizeof(buffer) - 1, 0) < 0) {
            perror("recv");
            close(client_sock);
            continue;
        }

        separator = strchr(buffer, ':');
        if (separator == NULL) {
            send(client_sock, "INVALID_FORMAT", 14, 0);
            close(client_sock);
            continue;
        }

        *separator = '\0';
        strncpy(username, buffer, MAX_LEN - 1);
        strncpy(password, separator + 1, MAX_LEN - 1);

        printf("[Backend] Validating credentials for user: %s\n", username);

        if (validate_credentials(username, password)) {
            printf("[Backend] Authentication SUCCESS for user: %s\n", username);
            send(client_sock, "AUTH_SUCCESS: Access Granted", 28, 0);
        } else {
            printf("[Backend] Authentication FAILED for user: %s\n", username);
            send(client_sock, "AUTH_FAILED: Access Denied", 26, 0);
        }

        explicit_bzero(buffer, sizeof(buffer));
        explicit_bzero(username, sizeof(username));
        explicit_bzero(password, sizeof(password));
        printf("[Backend] Sensitive data cleared from memory\n\n");

        close(client_sock);
    }

    close(server_sock);
    unlink(SOCKET_PATH);
    return 0;
}
