#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

// This one has to be global so we can close it in the signal handler
int server_socket;
#define PORT 4040
#define BUFFER_SIZE 1024
#define SAMPLE_RESPONSE "Hello world!\r\n"

void sigint_handler(int sig) {
    (void)sig; // suppress warning
    puts("Recieved SIGNINT. Sutting down...");
    close(server_socket);
    exit(1);
}


void *handle_client(void *client_socket_ptr) {
    int client_socket = *(int *)client_socket_ptr;

    free(client_socket_ptr);



    int bytes_read;
    char line[BUFFER_SIZE+1];
    char buffer[BUFFER_SIZE+1];
    signal(SIGINT, sigint_handler);

    // change the above loop to read line by line
    while (1) {
        bytes_read = recv(client_socket, line, BUFFER_SIZE, 0);

        if (bytes_read == 0) {
            puts("Connection closed by peer\n");
            break;
        }
        if (bytes_read < 0) {
            perror("Error reading from socket");
            break;
        }

        buffer[bytes_read] = '\0';
        printf("rec1: %s\n", buffer);
        char *new_line_pos = strchr(line, '\n');

        if (new_line_pos) {
            size_t bytes_to_copy = new_line_pos - buffer+1;
            // BUFFER OVERFLOW: I think in the edge case of there being
            // multiple buffers without a newline this could overflow the
            // line buffer. It will need to be confirmed and fixed.
            strncat(line, buffer, bytes_to_copy);
            printf("-> %s", line);
            break;
        } else {
            // if ther is no newline in the buffer, just append the buffer to the line
            strncat(line, buffer, bytes_read);
        }
    }


    #ifdef DEBUG
    printf("Connection closed\n");
    #endif


    // send a sample response
    send(client_socket, SAMPLE_RESPONSE, strlen(SAMPLE_RESPONSE), 0);

    close(client_socket);

    return NULL;
}


int main() {
    int new_socket;

    // define server address
    struct sockaddr_in server_address;
    int address_len = sizeof(server_address);

    // create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(1);
    }

    // bind server socket to address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);


    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding socket to address");
        exit(1);
    }

    // listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening for incoming connections");
        exit(1);
    }

    // register signal handler for SIGINT
    signal(SIGINT, sigint_handler);


    // accept incoming connections
    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&server_address, (socklen_t *)&address_len);
        if (new_socket == -1) {
            close(server_socket);
            perror("Error accepting incoming connection");
            exit(1);
        }
        // create a new thread to handle the client
        pthread_t thread;
        int *client_socket_ptr = malloc(sizeof(int));
        *client_socket_ptr = new_socket;

        // create a new thread to handle the client
        if (pthread_create(&thread, NULL, handle_client, client_socket_ptr) != 0) {
            perror("Error creating thread");
            close(server_socket);
            free(client_socket_ptr);
            exit(1);
        }

        pthread_detach(thread);
    }

    close(server_socket);
    return 0;
}