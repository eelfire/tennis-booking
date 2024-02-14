// server.c
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// network includes
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_COURTS 4
#define MAX_PLAYERS_IN_QUEUE 101
#define MAX_WAIT_TIME 30
#define SINGLES_M_GAME 10
#define SINGLES_F_GAME 5
#define DOUBLES_M_GAME 15
#define DOUBLES_F_GAME 10

typedef struct {
    int id;
    int arrival_time;
    double current_time;
    char gender;
    char preference;
    int player_socket;
} Player;

typedef struct {
    int id;
    int occupied;
    int end_time;
    Player players[];
} Court;

double get_current_time() {
    clock_t current_time = clock();
    double nanoseconds = (double)current_time / CLOCKS_PER_SEC * 1e9;
    // printf("Current time: %.2f nanoseconds\n", nanoseconds);
    return nanoseconds;
}

void insert_player_sorted(Player *queue, Player player) {
    int i = 0;
    while (queue[i].id != 0 && queue[i].arrival_time < player.arrival_time) {
        i++;
    }
    int j = i;
    while (queue[j].id != 0) {
        j++;
    }
    while (j > i) {
        queue[j] = queue[j - 1];
        j--;
    }
    queue[i] = player;
}

void insert_player_sorted_current_time(Player *queue, Player player) {
    int i = 0;
    while (queue[i].id != 0 && queue[i].current_time < player.current_time) {
        i++;
    }
    int j = i;
    while (queue[j].id != 0) {
        j++;
    }
    while (j > i) {
        queue[j] = queue[j - 1];
        j--;
    }
    queue[i] = player;
}

void update_courts(Court *courts, Player *singles_queue, Player *doubles_queue,
                   Player player, char *response, int *assigned) {
    // check if the singles queue has atleast two player
    if ((player.preference == 'S' || player.preference == 'b') &&
        singles_queue[0].id != 0 && singles_queue[1].id != 0) {
        // assign the court to the players
        int court_id = -1;
        for (int i = 0; i < MAX_COURTS; i++) {
            if (courts[i].occupied == 0) {
                court_id = i;
                break;
            }
        }
        if (court_id != -1) {
            // game_start_time = max(player1.current_time, player2.current_time)
            int game_start_time = singles_queue[0].arrival_time;
            if (singles_queue[1].arrival_time > game_start_time) {
                game_start_time = singles_queue[1].arrival_time;
            }
            int game_end_time = game_start_time + SINGLES_M_GAME;
            // assign the court to the players
            courts[court_id].occupied = 1;
            courts[court_id].end_time = game_end_time;

            // sending the response to singles_queue[0]
            sprintf(response, "%d,%d,%d,%d,S", singles_queue[0].id,
                    game_start_time, game_end_time, court_id);
            send(singles_queue[0].player_socket, response, strlen(response), 0);
            close(singles_queue[0].player_socket);
            // sending the response to singles_queue[1]
            sprintf(response, "%d,%d,%d,%d,S", singles_queue[1].id,
                    game_start_time, game_end_time, court_id);
            send(singles_queue[1].player_socket, response, strlen(response), 0);
            close(singles_queue[1].player_socket);

            // remove the players from the queue
            for (int i = 0; i < MAX_PLAYERS_IN_QUEUE - 1; i++) {
                singles_queue[i] = singles_queue[i + 2];
            }

            *assigned = 1;
        }
    }

    // check if the doubles queue has atleast four players
    if ((player.preference == 'D' || player.preference == 'B') &&
        doubles_queue[0].id != 0 && doubles_queue[1].id != 0 &&
        doubles_queue[2].id != 0 && doubles_queue[3].id != 0) {

        // assign the court to the players
        int court_id = -1;
        for (int i = 0; i < MAX_COURTS; i++) {
            if (courts[i].occupied == 0) {
                court_id = i;
                break;
            }
        }
        if (court_id != -1) {
            int game_start_time = doubles_queue[0].arrival_time;
            if (doubles_queue[1].arrival_time > game_start_time) {
                game_start_time = doubles_queue[1].arrival_time;
            }
            if (doubles_queue[2].arrival_time > game_start_time) {
                game_start_time = doubles_queue[2].arrival_time;
            }
            if (doubles_queue[3].arrival_time > game_start_time) {
                game_start_time = doubles_queue[3].arrival_time;
            }
            int game_end_time = game_start_time + DOUBLES_M_GAME;
            // assign the court to the players
            courts[court_id].occupied = 1;
            courts[court_id].end_time = game_end_time;

            // sending the response to doubles_queue[0]
            sprintf(response, "%d,%d,%d,%d,D", doubles_queue[0].id,
                    game_start_time, game_end_time, court_id);
            send(doubles_queue[0].player_socket, response, strlen(response), 0);
            close(doubles_queue[0].player_socket);
            // sending the response to doubles_queue[1]
            sprintf(response, "%d,%d,%d,%d,D", doubles_queue[1].id,
                    game_start_time, game_end_time, court_id);
            send(doubles_queue[1].player_socket, response, strlen(response), 0);
            close(doubles_queue[1].player_socket);
            // sending the response to doubles_queue[2]
            sprintf(response, "%d,%d,%d,%d,D", doubles_queue[2].id,
                    game_start_time, game_end_time, court_id);
            send(doubles_queue[2].player_socket, response, strlen(response), 0);
            close(doubles_queue[2].player_socket);
            // sending the response to doubles_queue[3]
            sprintf(response, "%d,%d,%d,%d,D", doubles_queue[3].id,
                    game_start_time, game_end_time, court_id);
            send(doubles_queue[3].player_socket, response, strlen(response), 0);
            close(doubles_queue[3].player_socket);

            // remove the players from the queue
            for (int i = 0; i < MAX_PLAYERS_IN_QUEUE - 1; i++) {
                doubles_queue[i] = doubles_queue[i + 4];
            }

            *assigned = 1;
        }
    }
}

void process_clients(int server_socket) {
    // setting up courts and singles/doubles queues
    Court *courts = (Court *)malloc(MAX_COURTS * sizeof(Court));
    for (int i = 0; i < MAX_COURTS; i++) {
        courts[i].id = i;
        courts[i].occupied = 0;
        courts[i].end_time = 0;
    }
    Player *singles_queue =
        (Player *)malloc(MAX_PLAYERS_IN_QUEUE * sizeof(Player));
    Player *doubles_queue =
        (Player *)malloc(MAX_PLAYERS_IN_QUEUE * sizeof(Player));
    // Player *queue = (Player *)malloc(MAX_PLAYERS_IN_QUEUE * sizeof(Player));

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
                    int id, arrival_time;
                    double current_time = get_current_time();
                    char gender, preference;
                    sscanf(line, "%d,%d,%c,%c", &id, &arrival_time, &gender,
                           &preference);
                    Player player = {id,     arrival_time, current_time,
                                     gender, preference,   client_socket};
                    // insert_player(queue, player);
                    if (preference == 'S' || preference == 'b') {
                        // insert_player_sorted(singles_queue, player);
                        insert_player_sorted_current_time(singles_queue,
                                                          player);
                    } else {
                        // insert_player_sorted(doubles_queue, player);
                        insert_player_sorted_current_time(doubles_queue,
                                                          player);
                    }

                    int assigned = 0, k = 0;
                    // assign court to the player if available using single
                    // OpenMP critical section
                    char response[256];
                    sprintf(response, "%d,-1,%d", player.id,
                            player.arrival_time + MAX_WAIT_TIME);
                    while (!assigned) {
                        if (k > 1000000) {
                            break;
                        }
#pragma omp critical
                        {
                            update_courts(courts, singles_queue, doubles_queue,
                                          player, response, &assigned);
                        }
                        k++;
                    }

                    if (!assigned) {
                        send(client_socket, response, sizeof(response), 0);
                        close(client_socket);
                    }
                }
            }
        }
    }

    free(courts);
    free(singles_queue);
    free(doubles_queue);
    return;
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