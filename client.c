#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8080
#define UNIX_SOCKET_PATH "/tmp/unix_socket"

// Функція для вимірювання часу
double time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Функція для встановлення неблокуючого режиму сокета
int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Використання: %s <тип сокету: INET/UNIX> <blocking/non-blocking> <розмір пакету> <кількість пакетів>\n", argv[0]);
        return -1;
    }

    char *socket_type = argv[1];
    char *blocking_type = argv[2];
    int packet_size = atoi(argv[3]);
    int packet_count = atoi(argv[4]);

    int sock = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_un unix_addr;
    char *message = malloc(packet_size);
    memset(message, 'A', packet_size);
    char buffer[packet_size];

    struct timespec start_time, end_time, connection_start, connection_end, socket_start, socket_end, close_start, close_end;

    // Вимірювання часу створення сокета
    clock_gettime(CLOCK_MONOTONIC, &socket_start);
    if (strcmp(socket_type, "INET") == 0) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
    } else if (strcmp(socket_type, "UNIX") == 0) {
        sock = socket(AF_UNIX, SOCK_STREAM, 0);
        unix_addr.sun_family = AF_UNIX;
        strcpy(unix_addr.sun_path, UNIX_SOCKET_PATH);
    } else {
        printf("Неправильний тип сокету. Використовуйте INET або UNIX.\n");
        return -1;
    }
    clock_gettime(CLOCK_MONOTONIC, &socket_end);

    if (strcmp(blocking_type, "non-blocking") == 0) {
        set_nonblocking(sock);
    }

    // Вимірювання часу з'єднання
    clock_gettime(CLOCK_MONOTONIC, &connection_start);
    int connect_result;
    if (strcmp(socket_type, "INET") == 0) {
        connect_result = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    } else {
        connect_result = connect(sock, (struct sockaddr *)&unix_addr, sizeof(unix_addr));
    }
    clock_gettime(CLOCK_MONOTONIC, &connection_end);

    if (connect_result < 0 && errno == EINPROGRESS) {
        // З'єднання ще в процесі встановлення
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);
        struct timeval timeout;
        timeout.tv_sec = 5;  // таймаут на з'єднання
        timeout.tv_usec = 0;

        // Очікуємо завершення з'єднання
        int select_result = select(sock + 1, NULL, &write_fds, NULL, &timeout);
        if (select_result > 0) {
            // Перевіряємо, чи завершилось з'єднання успішно
            int sock_error;
            socklen_t len = sizeof(sock_error);
            getsockopt(sock, SOL_SOCKET, SO_ERROR, &sock_error, &len);
            if (sock_error != 0) {
                printf("Помилка з'єднання: %s\n", strerror(sock_error));
                return -1;
            }
        } else {
            printf("Час очікування на з'єднання вичерпано або помилка select()\n");
            return -1;
        }
    } else if (connect_result < 0) {
        // Інша помилка підключення
        printf("Помилка підключення: %s\n", strerror(errno));
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (int i = 0; i < packet_count; i++) {
        send(sock, message, packet_size, 0);
        int bytes_read;
        if (strcmp(blocking_type, "non-blocking") == 0) {
            do {
                bytes_read = recv(sock, buffer, packet_size, MSG_DONTWAIT);
            } while (bytes_read == -1 && errno == EAGAIN);
        } else {
            bytes_read = recv(sock, buffer, packet_size, 0);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    // Вимірювання часу закриття сокета
    clock_gettime(CLOCK_MONOTONIC, &close_start);
    close(sock);
    clock_gettime(CLOCK_MONOTONIC, &close_end);

    double socket_creation_time = time_diff(socket_start, socket_end);
    double connection_time = time_diff(connection_start, connection_end);
    double data_transfer_time = time_diff(start_time, end_time);
    double socket_closing_time = time_diff(close_start, close_end);

    printf("Час створення сокета: %.6f секунд\n", socket_creation_time);
    printf("Час встановлення з'єднання: %.6f секунд\n", connection_time);
    printf("Час передачі даних: %.6f секунд\n", data_transfer_time);
    printf("Час закриття сокета: %.6f секунд\n", socket_closing_time);
    printf("Загальний час роботи: %.6f секунд\n", socket_creation_time + connection_time + data_transfer_time + socket_closing_time);

    int total_bytes = packet_size * packet_count;
    printf("Швидкість передачі: %.2f пакетів/сек\n", packet_count / data_transfer_time);
    printf("Швидкість передачі: %.2f байт/сек\n", total_bytes / data_transfer_time);

    free(message);
    return 0;
}
