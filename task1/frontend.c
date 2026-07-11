#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/auth.sock"
#define MAX_LEN 256

int main() {
    int sock;
    struct sockaddr_un addr;
    char username[MAX_LEN];
    char password[MAX_LEN];
    char result[MAX_LEN];
    char message[MAX_LEN * 2 + 2];

    printf("=== Secure Authentication System ===\n");
    printf("Frontend PID: %d\n", getpid());
    printf("Running as UID: %d\n", getuid());

    /* Get username */
    printf("\nUsername: ");
    if (fgets(username, MAX_LEN, stdin) == NULL) {
        fprintf(stderr, "Error reading username\n");
        exit(1);
    }
    username[strcspn(username, "\n")] = 0;

    /* Get password securely */
    printf("Password: ");
    if (fgets(password, MAX_LEN, stdin) == NULL) {
        fprintf(stderr, "Error reading password\n");
        exit(1);
    }
    password[strcspn(password, "\n")] = 0;

    /* Create UNIX domain socket */
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    /* Connect to backend */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect - is backend running?");
        close(sock);
        exit(1);
    }

    printf("\n[*] Connected to authentication backend\n");

    /* Format and send credentials */
    snprintf(message, sizeof(message), "%s:%s", username, password);
    if (send(sock, message, strlen(message), 0) < 0) {
        perror("send");
        close(sock);
        exit(1);
    }

    printf("[*] Credentials sent to backend for validation\n");

    /* Receive result */
    memset(result, 0, sizeof(result));
    if (recv(sock, result, sizeof(result) - 1, 0) < 0) {
        perror("recv");
        close(sock);
        exit(1);
    }

    printf("[*] Backend response: %s\n", result);

    /* Securely clear sensitive data from memory */
    explicit_bzero(password, sizeof(password));
    explicit_bzero(message, sizeof(message));
    printf("[*] Sensitive data cleared from memory\n");

    close(sock);
    return 0;
}
