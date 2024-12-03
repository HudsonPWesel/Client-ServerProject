#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9999        // Port the server listens on
#define BUFFER_SIZE 1024 // Buffer size for communication
#define MAX_LINES 100
#define MAX_LEN 1000
// Define the structure for trivia questions
typedef struct
{
    char question[BUFFER_SIZE];
    char answer[BUFFER_SIZE];
} Trivia;

typedef struct HandleClientArgs
{
    int client_socket;
    char *client_ip;
} HandleClientArgs;

Trivia trivia[4];
int trivia_count = 4; // Number of trivia questions

// Function to handle communication with a single client
void *handle_client(void *ClientArgs)
{

    char buffer[BUFFER_SIZE];            // Buffer for communication
    char leaderboardBuffer[BUFFER_SIZE]; // Buffer for communication
    int score = 0;                       // Client's score

    struct HandleClientArgs *CurrentArgs = (struct HandleClientArgs *)ClientArgs;

    char *client_ip = CurrentArgs->client_ip;
    int client_socket = CurrentArgs->client_socket;

    FILE *file;
    int line = 0;

    file = fopen("leaderboard.txt", "r");
    while (!feof(file) && !ferror(file))
        if (fgets(leaderboardBuffer, BUFFER_SIZE, file) != NULL)
        {
            send(client_socket, leaderboardBuffer, strlen(leaderboardBuffer), 0);
            line++;
        }

    for (int i = 0; i < trivia_count; i++)
    {
        // Send the current question to the client
        strcpy(buffer, trivia[i].question);

        send(client_socket, buffer, strlen(buffer), 0);

        // Allow time for the client to process the question

        // Receive the client's answer
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0)
        {
            printf("Client @ %s disconnected or error occurred.\n", client_ip);
            break;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the received data

        printf("Received from client @ %s: '%s'\n Correct Answer: %s\n ", client_ip, buffer, trivia[i].answer);

        // Remove trailing newline if present
        buffer[strcspn(buffer, "\n")] = 0;
        trivia[i].answer[strcspn(trivia[i].answer, "\n")] = 0;

        // Check the client's answer and send feedback (case insensitve)
        if (strcasecmp(buffer, trivia[i].answer) == 0)
        {
            score++;
            send(client_socket, "Correct!\n", 9, 0);
        }
        else
        {
            send(client_socket, "Wrong!\n", 7, 0);
        }

        // Allow time for the client to process the feedback
        usleep(100000); // 100ms delay
    }

    // Send the final score to the client
    sprintf(buffer, "Your final score is: %d\n", score);
    send(client_socket, buffer, strlen(buffer), 0);

    FILE *leaderboard = fopen("leaderboard.txt", "a");
    fprintf(leaderboard, "%s|%d\n", client_ip, score);
    fclose(leaderboard);

    close(client_socket); // Close the connection with the client
    return NULL;

    // My attempt to update scores inplace

    // int nthCol = 0;
    // int i = 0;
    // int isSameAddress = 1;
    // while (leaderboardData[0][nthCol])
    // {
    //     while (leaderboardData[i][nthCol] != '|')
    //     {
    //         isSameAddress = leaderboardData[i][nthCol] == client_ip[i];
    //     }
    //     if (isSameAddress){
    //         leaderboardData[i + 1][nthCol] = score;
    //         break;
    //     }
    //     nthCol++;
    // }
}

int main()
{
    int server_socket, client_socket;            // Socket descriptors
    struct sockaddr_in server_addr, client_addr; // Address structures
    socklen_t addr_len = sizeof(client_addr);
    char client_ip[INET_ADDRSTRLEN]; // Buffer to store the client's IP address

    // Create the server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure the server's address
    server_addr.sin_family = AF_INET;         // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any IP
    server_addr.sin_port = htons(PORT);       // Port

    // Bind the server socket to the configured address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    listen(server_socket, 5);
    printf("Server is running on port %d...\n", PORT);

    // Question from File
    FILE *file;
    char data[MAX_LINES][MAX_LEN];
    int isQuestion = 1;
    int line = 0;

    file = fopen("questions.txt", "r");

    while (!feof(file) && !ferror(file))
        if (isQuestion)
        {
            fgets(trivia[line].question, BUFFER_SIZE, file);
            isQuestion = 0;
        }
        else
        {
            fgets(trivia[line].answer, BUFFER_SIZE, file);
            isQuestion = 1;
            line++;
        }
    fclose(file);

    while (1)
    {
        // Accept a client connection
        // client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        pthread_t thread_id;
        int *client_socket_ptr = malloc(sizeof(int));
        *client_socket_ptr = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);

        if (*client_socket_ptr < 0)
        {
            perror("Accept failed");
            continue;
        }

        // Convert the client's IP address to a hkuuman-readable format
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Client connected: %s\n", client_ip);

        HandleClientArgs currentClientArgs;
        currentClientArgs.client_socket = *client_socket_ptr;
        currentClientArgs.client_ip = client_ip;

        // handle_client(client_socket, client_ip);
        pthread_create(&thread_id, NULL, handle_client, (void *)&currentClientArgs);
        // pthread_detach(thread_id);
    }

    close(server_socket); // Close the server socket
    return 0;
}
