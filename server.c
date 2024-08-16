#include <linux/ip.h>   // iphdr
#include <signal.h>     // signal
#include <arpa/inet.h>  // inet_pton, inet_ntop
#include <netinet/in.h> // sockaddr_in, htons, ntohs
#include <stdio.h>      // fprintf, printf, scanf
#include <stdlib.h>     // malloc, free, exit
#include <string.h>     // string, strlen
#include <sys/socket.h> // socket
#include <unistd.h>     // read, write, close

#define SERVER_IP "192.168.1.6"
#define CLIENT_IP "192.168.1.7"

struct sockaddr_in *server_address = NULL; // pointer to the server address structure
struct sockaddr_in *client_address = NULL; // pointer to the client address structure

// socket file descripter (unique id) used to reference and manipulate the socket
int socket_file_desc;

// server will run on this port number
int port_number;

void handle_error(const char *msg)
{
    // perror will print the value of errno which was got from the last system call and also print the msg we passed
    perror(msg);
    exit(EXIT_FAILURE);
}

/* When ever you press Ctrl + C, socket should be closed and memory should be freed */
void interrupt_handler(int signum)
{
    close(socket_file_desc);
    free(server_address);
    free(client_address);
    printf("\nSocket closed");
    exit(EXIT_SUCCESS);
}

// Generic checksum function
unsigned short checksum(void *buffer, int len)
{
    unsigned short *data_buffer = buffer;
    unsigned int sum            = 0;
    unsigned short result;

    while (len > 1)
    {
        sum += *data_buffer++;
        len -= 2;
    }

    if (len == 1)
    {
        sum += *(unsigned char *)data_buffer;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    result = ~sum;

    return result;
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

    signal(SIGINT, interrupt_handler);
    signal(SIGTERM, interrupt_handler);

    // malloc is assigning a memory address to our server_address, making it point to a struct sockaddr_in object
    // we also need to typecast the void pointer returned by malloc to sockaddr_in pointer
    server_address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    // if there was any error in allocation, exit the proocess
    if (server_address == NULL)
    {
        handle_error("Failed to allocate memory for server address");
    }

    // malloc is assigning a memory address to our client_address, making it point to a struct sockaddr_in object
    // we also need to typecast the void pointer returned by malloc to sockaddr_in pointer
    client_address = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

    // if there was any error in allocation, exit the proocess
    if (client_address == NULL)
    {
        handle_error("Failed to allocate memory for server address");
    }

    /* create the socket */
    socket_file_desc = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (socket_file_desc < 0)
    {
        handle_error("Socket creation failed");
    }

    /* filling up the server object */

    // assign address family i.e: in which domain we are going to communicate , is it a internet(ipv4, ipv6) or local(unix) or bluetooth
    server_address->sin_family = AF_INET;

    // assign the port number on which server will be running
    // First we need to convert port number's byte order to network byte order (big endian) using htons
    // htons : host byte order to network byte order short
    server_address->sin_port = htons(port_number);

    // here sin_addr is a ` struct in_addr ` and in that structure we have a uint32_t s_addr in which we are assigning our IP
    // In in_addr, uint32_t is  typedef to in_addr_t
    // inet_addr converts ip from numbers and dot format to binary format in network byte order
    server_address->sin_addr.s_addr = inet_addr(SERVER_IP);

    // here we associate our socket to a specific network interface and port number that is called binding
    // in our case, I already have created 2 virtual network interfaces, one for server and other for client
    // It tells our OS that what ever you receive at this particular address and port, send that to me
    if (bind(socket_file_desc, (struct sockaddr *)server_address, sizeof(struct sockaddr_in)) < 0)
    {
        handle_error("Bind failed");
    }

    printf("Server is listening on %s:%d\n", SERVER_IP, port_number);

    // Buffer for incomming data
    char buffer[4096];

    // used by recvfrom() to know size of client_address
    socklen_t address_length = sizeof(struct sockaddr_in);

    while (1)
    {
        // clears the buffer by setting it to 0
        memset(buffer, 0, sizeof(buffer));

        // receive data from the socket using its file descriptor that is send by someone on this socket and store that data in buffer
        // And store the sender's address in the client address structure
        // the address_length is used by recevfrom() to know maximum size of the address information it can write in client_address after the function call its value will be changed to the acutal size of address information it wrote
        int byte_received = recvfrom(socket_file_desc, buffer, sizeof(buffer), 0, (struct sockaddr *)client_address, &address_length);

        // In case recvfrom() got any error
        if (byte_received < 0)
        {
            handle_error("Error receiving data");
        }

        // here buffer will receive a packet with IP header + Data concatenated
        // so we need to get that data seperated, that can be done by just adding that length of IP header(ihl) to the buffer address
        struct iphdr *received_ip_header = (struct iphdr *)buffer;

        // ihl value was stored in words so converting it into bytes by multiplying by 4
        char *data = buffer + (received_ip_header->ihl * 4);

        // inet_ntoa : changes address from binary to ascii , ntohs : network byte order to host short
        printf("Received packet from %s\n", inet_ntoa(client_address->sin_addr));
        printf("Data: %s\n", data);

        // just a string that client is going to receive in the reply packet
        char *reply = "What do you want ??";

        // as the buffer already have the received packet's data, we need to clear the buffer and copy our reply string to it
        memset(buffer, 0, sizeof(buffer));
        strncpy(buffer, reply, sizeof(buffer));

        // added null character at the end to terminate the string
        buffer[sizeof(buffer) - 1] = '\0';

        /* we need to create a reply packet for client */

        // it is a pointer of type iphdr to a memory address which is going to store our ip header
        struct iphdr *sending_ip_header = (struct iphdr *)malloc(sizeof(struct iphdr));

        // in case memory allocation failed
        if (sending_ip_header == NULL)
        {
            handle_error("Failed during memory allocation");
        }

        // Allocate memory for a packet consisting of an IP header (iphdr) and buffer.
        char *packet = (char *)malloc(sizeof(struct iphdr) + sizeof(buffer));

        // in case memory allocation failed
        if (packet == NULL)
        {
            handle_error("Failed during memory allocation");
        }

        // In case there is alreadyy something in the packet memory , we need to ensure that its all 0
        memset(packet, 0, sizeof(struct iphdr) + sizeof(buffer));

        /* setting up IP header here */
        // ihl: ip header length and 5 = 5 words = 5 * 4 bytes = 20 bytes
        sending_ip_header->ihl = 5;

        // ip address version either it is a ipv4 or ipv6
        sending_ip_header->version = 4;

        // it indicates that the packet is containing raw IP data
        sending_ip_header->protocol = IPPROTO_RAW;

        // total length of the packet
        sending_ip_header->tot_len = htons(sizeof(struct iphdr) + strlen(buffer));

        sending_ip_header->ttl = 255;

        // source address to equal to client_ip that we defined as micro on top
        // inet_addr conver the ip from dots and number to binary format
        sending_ip_header->saddr = inet_addr(SERVER_IP);

        // destination address to equal to serrver_ip that we defined as micro on top
        sending_ip_header->daddr = inet_addr(CLIENT_IP);

        // here is the final check that will store a unsigned short which is calculated by some certain mathematical operation by combining all the data of sending_ip_header;
        // At server what ever the header it will receive, will also calculate the checksum number
        // if we got the same number after calculation at server as the sending_ip_header->check it will mean that all the information is correctly transmitted
        // this number is just used to double verify things
        sending_ip_header->check = checksum((unsigned short *)sending_ip_header, sending_ip_header->tot_len);

        // copying the IP header to packet
        memcpy(packet, sending_ip_header, sizeof(struct iphdr));

        // after the IP header we need to copy out reply message to packet
        memcpy(packet + sizeof(struct iphdr), buffer, strlen(buffer));

        // sending the packet
        int byte_send = sendto(socket_file_desc, packet, ntohs(sending_ip_header->tot_len), 0, (struct sockaddr *)client_address, address_length);

        // in case packet failed to send;
        if (byte_send < 0)
        {
            handle_error("Error sending reply");
        }

        printf("Reply sent to client\n");
        printf("---------------------------------------\n");

        free(sending_ip_header);
        free(packet);
    }

    interrupt_handler(SIGKILL);
    return EXIT_SUCCESS; // EXIT_SUCCESS = 0
}
