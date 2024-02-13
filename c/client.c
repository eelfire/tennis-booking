// client.c
#define _GNU_SOURCE
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// network includes
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int MAX_LINES = 1000;

void client(FILE *file) {
    int num_lines = 0;
    char *nline = NULL;
    size_t nlen = 0;
    ssize_t nread;

    while ((nread = getline(&nline, &nlen, file)) != -1) {
        num_lines++;
    }

    // reset file pointer to start of file
    fseek(file, 0, SEEK_SET);

    // create an array of strings to store each line
    char **lines = (char **)malloc(num_lines * sizeof(char *));
    for (int i = 0; i < num_lines; i++) {
        lines[i] = (char *)malloc(256 * sizeof(char));
    }

    // read each line into the array
    int i = 0;
    while ((nread = getline(&nline, &nlen, file)) != -1) {
        strcpy(lines[i], nline);
        i++;
    }
    printf("Read %d lines\n", num_lines);

#pragma omp parallel for
    for (int i = 0; i < num_lines; i++) {
        // print thread id
        // printf("Thread %d\n", omp_get_thread_num());

        char *line = lines[i];
        size_t len = strlen(line);
        // if first character is not a number then skip the line
        if (line[0] < '0' || line[0] > '9') {
            continue;
        }

        printf("Sending %zu size: %s", len, line);

        // create a socket
        int client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            printf("Error: Could not create socket\n");
            exit(EXIT_FAILURE);
        }

        // connect to the server
        struct sockaddr_in server_address;
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(1156);
        server_address.sin_addr.s_addr = INADDR_ANY;
        int connection_status =
            connect(client_socket, (struct sockaddr *)&server_address,
                    sizeof(server_address));
        if (connection_status == -1) {
            printf("Error: Could not connect to server\n");
            exit(EXIT_FAILURE);
        }

        // send the line to the server
        send(client_socket, line, len, 0);

        // receive data from the server
        char server_response[256];
        recv(client_socket, &server_response, sizeof(server_response), 0);

        // print out the server's response
        printf("Server: %s\n", server_response);

        // close the socket
        close(client_socket);

        free(line);
    }

    // free(line);
    fclose(file);

    return;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }

    printf("Hello from OpenMP powered client\n");

    // take file path as input for csv file containing players
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Error: File not found\n");
        return 1;
    }

    client(file);

    return 0;
}