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
    char *new_line_pos;
    char line[BUFFER_SIZE+1];
    char buffer[BUFFER_SIZE+1];
    memset(line, 0, sizeof(line));
    memset(buffer, 0, sizeof(buffer));
    signal(SIGINT, sigint_handler);
    



    // Read the request in chunks incase its larger than our buffer size
    while (1) {
        bytes_read = recv(client_socket, buffer, BUFFER_SIZE, 0);

        if (bytes_read == 0) {
            puts("Connection closed by peer\n");
            break;
        }
        if (bytes_read < 0) {
            perror("Error reading from socket");
            break;
        }
        
        buffer[bytes_read] = '\0';
        

        // separate each line of the buffer
        while ((new_line_pos = strchr(buffer, '\n')) != NULL) {
            size_t bytes_to_copy = new_line_pos - buffer+1;

            strncat(line, buffer, bytes_to_copy);
            

            // Process each line
            printf("-> %s", line);
            memset(line, 0, sizeof(line));

            // Ensure the pointer is not past the end of the buffer
            if (buffer+ bytes_to_copy <= buffer + BUFFER_SIZE) {
                memmove(buffer, buffer+bytes_to_copy, BUFFER_SIZE-bytes_to_copy);
            }

        } 
        
        // if ther is no newline in the buffer, just append the buffer to the line
        // this line should be continued in the next buffer hopefully
        // BUFFER OVERFLOW: We dont actually check how many times things are being copied to
        // line. If the client sends a very long line, it overflows the line buffer.
        strncat(line, buffer, bytes_read);
            
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
    int optval = 1;

    // create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(1);
    }
    
    // Set SO_REUSEADDR option to allow binding to an address that is already in use
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Error setting SO_REUSEADDR");
        close(server_socket);
        exit(1);
    }

    // bind server socket to address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);


    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding socket to address");
        close(server_socket);
        exit(1);
    }

    // listen for incoming connections
    if (listen(server_socket, 5) == -1) {
        perror("Error listening for incoming connections");
        close(server_socket);
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