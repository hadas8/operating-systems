#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#define BUF_SIZE 1024

// A method for sending all data over a socket
void sendall(int sock, void *buffer, ssize_t len) {
    char *buff = (char *)buffer;
    ssize_t  send_ret;
    uint32_t byte_sent = 0;

    while(byte_sent < 4) {
        send_ret = send(sock, &buff[byte_sent], len - byte_sent, 0);
        if(send_ret == -1) {
            fprintf(stderr, "failed sending data: %s\n", strerror(errno));
            exit(1);
        }
        byte_sent += send_ret;
    }
}

// A method for sending messages to the server
uint32_t tcp_connection(int sock, uint32_t N, FILE *file) {
    char buff[BUF_SIZE];
    uint32_t C, bytes_sent = 0, N_net = htonl(N);
    uint32_t bytes_read;

    // Send file size to the server
    sendall(sock, &N_net, sizeof (N_net));

    // Send the file to the server
    do {
        bytes_read = fread(buff, 1, BUF_SIZE, file);
        if (bytes_read == -1) {
            fprintf(stderr, "can't read file: %s\n", strerror(errno));
            exit(1);
        }
        sendall(sock, buff, bytes_read);
        bytes_sent += bytes_read;
    } while (bytes_read && bytes_sent < N);

    if(bytes_sent < N) {
        fprintf(stderr, "file was shorter than file size");
        exit(1);
    }

    //receive number of printable bytes from the server
    if(recv(sock, &C, sizeof(C), 0) == -1) {
        fprintf(stderr, "failed reading from socket: %s\n", strerror(errno));
        exit(1);
    }

    return ntohl(C);
}


int main(int argc, char **argv) {
    char *ip_address, *file_path;
    int sock;
    uint16_t port;
    uint32_t N, C;
    FILE * input_file;
    struct sockaddr_in connection_details;

    if (argc != 4) {
        fprintf(stderr, "Must provide 3 arguments");
        exit(1);
    }

    // Load input parameters (ip, port, file path)
    ip_address = argv[1];
    port = (uint16_t)atoi(argv[2]);
    file_path = argv[3];

    // Open the specified file for reading
    input_file  = fopen(file_path, "r");
    if (input_file == NULL) {
        fprintf(stderr, "can't open input file: %s\n", strerror(errno));
        exit(1);
    }

    // Determine the file size
    fseek(input_file, 0L, SEEK_END);
    N = ftell(input_file);
    rewind(input_file);

    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        fprintf(stderr, "socket creation failed: %s\n",strerror(errno));
        exit(1);
    }

    // Build connection details sockaddr_in struct
    connection_details.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address, &connection_details.sin_addr);
    connection_details.sin_port = htons(port);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&connection_details, sizeof(connection_details)) != 0) {
        fprintf(stderr, "connection to server failed: %s\n", strerror(errno));
        exit(1);
    }
    C = tcp_connection(sock, N, input_file);
    printf("# of printable characters: %u\n", C);

    // Close the socket
    close(sock);

    // Close the file
    fclose(input_file);

    exit(0);
}
