#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 4040
#define BUFFER_SIZE 1024
#define SAMPLE_RESPONSE "Hello world!\r\n"


void *handle_client(void *client_socket_ptr) {
    int client_socket = *(int *)client_socket_ptr;

    free(client_socket_ptr);

    send(client_socket, SAMPLE_RESPONSE, strlen(SAMPLE_RESPONSE), 0);
    printf("Sent response to client\n");


    close(client_socket);
    printf("Closed connection to client\n");

    return NULL;
}



int main() {
    int new_socket;
    int server_socket;

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

    // accept incoming connections
    while (1) {
        new_socket = accept(server_socket, (struct sockaddr *)&server_address, (socklen_t *)&address_len);
        if (new_socket == -1) {
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
            free(client_socket_ptr);
            exit(1);
        }

        pthread_detach(thread);
    }

    close(server_socket);
    return 0;

}