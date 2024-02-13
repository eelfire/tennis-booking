// server.c
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// network includes
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_COURTS 4
#define MAX_WAIT_TIME 30

typedef struct {
    int id;
    char gender;
    char preference;
    int arrival_time;
} Player;

Player courts[MAX_COURTS];

void process_clients(int server_socket) {
#pragma omp parallel
    {
#pragma omp single
        {
            // accept requests
            struct sockaddr_in client_address;
            int client_socket;
            int client_address_size = sizeof(client_address);
            while ((client_socket = accept(
                        server_socket, (struct sockaddr *)&client_address,
                        (socklen_t *)&client_address_size))) {
#pragma omp task
                {
                    // print thread id
                    // printf("Thread %d\n", omp_get_thread_num());

                    // client ip and port
                    printf("client ip:port - %s:%d\n",
                           inet_ntoa(client_address.sin_addr),
                           ntohs(client_address.sin_port));

                    // receive data from the client
                    char client_request[256];
                    recv(client_socket, &client_request, sizeof(client_request),
                         0);
                    // read the request till the esacpe character
                    char *line = NULL;
                    int i = 0;
                    while (client_request[i] != '\r' &&
                           client_request[i] != '\n' &&
                           client_request[i] != '\0') {
                        line = realloc(line, (i + 1) * sizeof(char));
                        line[i] = client_request[i];
                        i++;
                    }
                    line[i] = '\0';
                    printf("\tReceived: %s\n", line);

                    // process the request

                    // send data to the client
                    send(client_socket, "You have reached the server", 256, 0);

                    // close the socket
                    close(client_socket);
                }
            }
        }
    }
}

int main() {
    printf("Hello from OpenMP powered server\n");

    // start listening for requests on port 1156
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Error: Could not create socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(1156);
    server_address.sin_addr.s_addr = INADDR_ANY;

    int bind_status = bind(server_socket, (struct sockaddr *)&server_address,
                           sizeof(server_address));
    if (bind_status == -1) {
        printf("Error: Could not bind socket\n");
        exit(EXIT_FAILURE);
    }

    int listen_status = listen(server_socket, 10);
    if (listen_status == -1) {
        printf("Error: Could not listen on socket\n");
        exit(EXIT_FAILURE);
    }

    process_clients(server_socket);

    return 0;
}