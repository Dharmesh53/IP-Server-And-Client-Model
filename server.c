#include <arpa/inet.h>  // inet_pton, inet_ntop
#include <errno.h>      // errno, perror
#include <linux/ip.h>   // iphdr
#include <linux/udp.h>  // uphdr
#include <netinet/in.h> // sockaddr_in, htons, ntohs
#include <signal.h>     // signal, sigaction
#include <stdio.h>      // fprintf, printf, scanf
#include <stdlib.h>     // malloc, free, exit
#include <sys/socket.h> // socket, bind, sendto, recvfrom
#include <unistd.h>     // read, write, close

struct sockaddr_in *server_address = NULL; // pointer to the server address structure
struct sockaddr_in *client_address = NULL; // pointer to the client address structure

// socket file descripter (unique id) used to reference and manipulate the socket
int socket_file_desc;

// server will run on this port number
int port_number;

void handle_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    // if the port number is not given
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <server port number> \n", argv[0]);
        exit(EXIT_FAILURE); // EXIT_FAILURE = any non zero value
    }

    // if we get the port it must be in string, we need to change it in integer
    char *port  = argv[1];
    port_number = atoi(port);

    // port number(16 bit) only exist from 0 to 65535  in which 0 - 2000  is already in use by system
    if (port_number <= 2000 || port_number > 65535)
    {
        fprintf(stderr, "Invalid Port Number: %s\n, must be in range of 2000 - 65535", port);
        exit(EXIT_FAILURE);
    }

    // malloc is allocating us space for our server_address struct to store and  return us a pointer to that location's first bit
    // we also need to typecast the void pointer to sockaddr_in pointer
    server_address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    // if there was any error in allocation, exit the proocess
    if (server_address == NULL)
    {
        handle_error("Failed to allocate memory for server address");
    }

    // malloc is allocating us space for our client_address struct to store and return us a pointer to that location's first bit
    // we also need to typecast the void pointer to sockaddr_in pointer
    client_address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    // if there was any error in allocation, exit the proocess
    if (client_address == NULL)
    {
        handle_error("Failed to allocate memory for server address");
    }

    printf("These are the address locations %p and %p\n", server_address, client_address);

    return EXIT_SUCCESS; // EXIT_SUCCESS = zero
}
