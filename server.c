#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>

#define PORT 8080
#define UNIX_SOCKET_PATH "/tmp/unix_socket"
#define BUFFER_SIZE 1024

double time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

typedef struct {
    int socket;
    int is_nonblocking;
} connection_data;

void* handle_connection(void* data) {
    connection_data* conn = (connection_data*)data;
    int new_socket = conn->socket;
    int is_nonblocking = conn->is_nonblocking;
    free(conn);

    char buffer[BUFFER_SIZE] = {0};
    int total_received_bytes = 0;
    int total_packets = 0;

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (1) {
        int bytes_read;
        if (is_nonblocking) {
            do {
                bytes_read = recv(new_socket, buffer, BUFFER_SIZE, MSG_DONTWAIT);
            } while (bytes_read == -1 && errno == EAGAIN);
        } else {
            bytes_read = recv(new_socket, buffer, BUFFER_SIZE, 0);
        }

        if (bytes_read <= 0) {
            break;
        }

        total_received_bytes += bytes_read;
        total_packets++;

        char *message = "OK";
        send(new_socket, message, strlen(message), 0);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double data_transfer_time = time_diff(start_time, end_time);
/*
    printf("Час передачі даних: %.6f секунд\n", data_transfer_time);
    printf("Всього отримано %d байт\n", total_received_bytes);
    printf("Кількість пакетів: %d\n", total_packets);
    printf("Швидкість передачі: %.2f пакетів/сек\n", total_packets / data_transfer_time);
    printf("Швидкість передачі: %.2f байт/сек\n", total_received_bytes / data_transfer_time);
*/
    close(new_socket);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Використання: %s <тип сокету: INET/UNIX> <blocking/non-blocking> <sync/async>\n", argv[0]);
        return -1;
    }

    char *socket_type = argv[1];
    char *blocking_type = argv[2];
    char *sync_type = argv[3];

    int server_fd, new_socket;
    struct sockaddr_in address;
    struct sockaddr_un unix_addr;
    int addrlen = sizeof(address);

    if (strcmp(socket_type, "INET") == 0) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);
        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Помилка прив'язки");
            exit(EXIT_FAILURE);
        }
    } else if (strcmp(socket_type, "UNIX") == 0) {
        server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        unix_addr.sun_family = AF_UNIX;
        strcpy(unix_addr.sun_path, UNIX_SOCKET_PATH);
        unlink(UNIX_SOCKET_PATH);
        if (bind(server_fd, (struct sockaddr *)&unix_addr, sizeof(unix_addr)) < 0) {
            perror("Помилка прив'язки");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Неправильний тип сокету. Використовуйте INET або UNIX.\n");
        return -1;
    }

    if (strcmp(blocking_type, "non-blocking") == 0) {
        set_nonblocking(server_fd);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Помилка прослуховування");
        exit(EXIT_FAILURE);
    }

    printf("Сервер очікує підключення...\n");

    while (1) {
        if (strcmp(socket_type, "INET") == 0) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        } else {
            new_socket = accept(server_fd, (struct sockaddr *)&unix_addr, (socklen_t*)&addrlen);
        }

//        if (new_socket < 0) {
//            perror("Помилка прийняття підключення");
//            continue;
//        }

        connection_data* conn = malloc(sizeof(connection_data));
        conn->socket = new_socket;
        conn->is_nonblocking = (strcmp(blocking_type, "non-blocking") == 0);

        if (strcmp(sync_type, "async") == 0) {
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, handle_connection, (void*)conn);
            pthread_detach(thread_id);
        } else {
            handle_connection((void*)conn);
        }
    }

    return 0;
}
