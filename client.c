#include <signal.h>     // signal
#include <errno.h>      // errno, perror
#include <arpa/inet.h>  // inet_pton, inet_ntop
#include <netinet/in.h> // sockaddr_in, htons, ntohs
#include <linux/ip.h>   // iphdr
#include <stdio.h>      // fprintf, printf, scanf
#include <stdlib.h>     // malloc, free, exit
#include <string.h>     // string, strlen
#include <sys/socket.h> // socket
#include <unistd.h>     // read, write, close

#define SERVER_IP "192.168.1.6"
#define CLIENT_IP "192.168.1.7"

struct sockaddr_in *server_address = NULL; // pointer to the server address structure

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
    printf("Socket closed");
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

    // memory for storing our message
    char message[4096];

    // a pointer to the memory in which IP header is stored before transmitting to socket
    struct iphdr *ip_header = (struct iphdr *)malloc(sizeof(struct iphdr));

    // in case memory can't be allocated
    if (ip_header == NULL)
    {
        handle_error("Failed during memory allocation");
    }

    // a pointer to the memory in which complete packet that we are going to send throught socket is stored
    char *packet = (char *)malloc(sizeof(struct iphdr) + sizeof(message));

    // in case memory can't be allocated
    if (packet == NULL)
    {
        handle_error("Failed during memory allocation");
    }

    // used by recvfrom() to know size of client_address
    socklen_t address_length = sizeof(struct sockaddr_in);

    while (1)
    {
        printf("Enter message to send: ");

        // it will get the data from the stdin data stream and store it in the message array
        fgets(message, sizeof(message), stdin);

        // strcspn() will return the index of first '\n' and putting that index in message and assigning it to 0 (NULL)
        // Just to prevent '\n' line from transmitting
        message[strcspn(message, "\n")] = 0;

        // setted the whole packet memory to 0
        // memset is taking sizeof(struct ipdhr) + sizeof(message) as the length of packet which it going to set 0
        memset(packet, 0, sizeof(struct iphdr) + sizeof(message));

        /* setting up IP header here */

        // ihl: ip header length and 5 = 5 words = 5 * 4 bytes = 20 bytes
        ip_header->ihl = 5;

        // ip address version either it is a ipv4 or ipv6
        ip_header->version = 4;

        // it indicates that the packet is containing raw IP data
        ip_header->protocol = IPPROTO_RAW;

        // total length of the packet
        ip_header->tot_len = sizeof(struct iphdr) + strlen(message);

        // source address to equal to client_ip that we defined as micro on top
        // inet_addr conver the ip from dots and number to binary format
        ip_header->saddr = inet_addr(CLIENT_IP);

        // destination address to equal to serrver_ip that we defined as micro on top
        ip_header->daddr = inet_addr(SERVER_IP);

        // here is the final check that will store a unsigned short which is calculated by some certain mathematical operation by combining all the data of ip_header;
        // At server what ever the header it will receive, will also calculate the checksum number
        // if we got the same number after calculation at server as the ip_header->check it will mean that all the information is correctly transmitted
        // this number is just used to double verify things
        ip_header->check = checksum((unsigned short *)ip_header, ip_header->tot_len);

        // first we copy the ip_headers in the packet
        memcpy(packet, ip_header, sizeof(struct iphdr));

        // then we copy out payload to the packet
        memcpy(packet + sizeof(struct iphdr), message, strlen(message) + 1);

        // here we send our packet to server_address using socket
        // ntohs: network to host byte order short
        int byte_send = sendto(socket_file_desc, packet, ntohs(ip_header->tot_len), 0, (struct sockaddr *)server_address, address_length);

        // in case sendto failed to send packet
        if (byte_send < 0)
        {
            handle_error("Error sending message");
        }

        printf("Raw packet send to server\n");

        // After sending our message, we need to wait for the server reply for that we need to loop through recvfrom() until we get something
        // If we don't run this loop, recvfrom() we again read the meesage that you have send and copy it in buffer (that is some messed up shit, you can try it by yourself in case you don't understand what i'm saying)
        while (1)
        {
            // clearing the message to receive the server reply data
            memset(message, 0, sizeof(message));

            // receiving the packet
            int byte_received = recvfrom(socket_file_desc, message, sizeof(message), 0, NULL, NULL);

            // in case failed to receive packet
            if (byte_received < 0)
            {
                handle_error("Error receiving reply");
            }

            // Cast the message pointer to a struct iphdr* to access the IP header fields.
            // This allows us to interpret the data in 'message' as an IP header.
            struct iphdr *recv_ip_header = (struct iphdr *)message;

            // Calculate the address of the data following the IP header in the message.
            // The length of the IP header is given in 32-bit words (ihl), so we multiply by 4 to get the byte offset.
            char *data = message + (recv_ip_header->ihl * 4);

            // Check if this packet is from the server (not our own packet)
            if (recv_ip_header->saddr == inet_addr(SERVER_IP))
            {
                printf("Received reply from server: %s\n", data);
                printf("----------------------------------------\n");
                break; // Exit the inner while loop to send next message
            }
        }
    }

    interrupt_handler(SIGKILL);
    return EXIT_SUCCESS; // EXIT_SUCCESS = 0
}
