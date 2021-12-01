#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#define BUF_SIZE 1024
#define PCC_SIZE (126 - 31)
#define PRINTABLE(x) ((x >= 32) && (x <= 126))

uint32_t pcc_total[PCC_SIZE] = {0};
uint32_t pcc[PCC_SIZE];
int accept_con = -1;
int flag = 1;

// A method for sending all data over a socket
int sendall(int sock, void *buffer, ssize_t len) {
    char *buff = (char *)buffer;
    ssize_t  send_ret;
    uint32_t byte_sent = 0;

    while(byte_sent < 4) {
        send_ret = send(sock, &buff[byte_sent], len - byte_sent, 0);
        if(send_ret == -1) {
            fprintf(stderr, "failed sending data: %s\n", strerror(errno));
            return 0;
        }
        byte_sent += send_ret;
    }

    return 1;
}

// A method for receiving all data over a socket and counting all printable characters
uint32_t count_printable() {
    char buff[BUF_SIZE];
    uint32_t N_net, N, bytes_read = 0, bytes_rec, C_net, C = 0;

    //initializing pcc array
    memset(pcc, 0, sizeof(pcc));

    // Read file size from client
    if(recv(accept_con, &N_net, sizeof(N_net), 0) == -1) {
        fprintf(stderr, "failed reading from socket: %s\n", strerror(errno));
        return -1;
    }
    N = ntohl(N_net);

    do {
        // Read data into buffer
        bytes_rec = recv(accept_con, buff, BUF_SIZE, 0);
        if (bytes_rec == -1) {
            fprintf(stderr, "failed reading from socket: %s\n", strerror(errno));
            return -1;
        }

        for (int i = 0; i < bytes_rec; i++) {
            if (PRINTABLE(buff[i])) {
                C++;
                pcc[buff[i] - 32]++;
            }
        }
        bytes_read += bytes_rec;
    } while (bytes_rec && bytes_read < N);

    if (bytes_read < N) {
        fprintf(stderr, "file was shorter than file size");
        return -1;
    }

    // Send C to the client
    C_net = htonl(C);
    if(!sendall(accept_con, &C_net, sizeof (C_net))) {
        return -1;
    }

    // Update pcc_total
    for(int i = 0; i < PCC_SIZE; i++) {
        pcc_total[i] += pcc[i];
    }

    // Return the number of printable characters found
    return C;
}

// Print total printable character count
void print_count() {
    for(int i = 0; i < PCC_SIZE; i++) {
        printf("char '%c' : %u times\n", (i + 32), pcc_total[i]);
    }
    exit(0);
}

// SIGINT handler
void my_handler(){
    if(accept_con == -1) {
        print_count();
    } else {
       flag = 0;
    }
}

int main(int argc, char **argv) {
    int sock, sockopt = 1;
    uint16_t  port;
    uint32_t C;
    struct  sockaddr_in connection_details;
    struct sigaction sa;
    sa.sa_handler = &my_handler;
    sa.sa_flags = SA_RESTART;

    // Validate argument count
    if (argc != 2) {
        fprintf(stderr, "Must provide 1 argument");
        exit(1);
    }

    // Set signal handler
    if(sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "Failed setting signal handler: %s\n", strerror(errno));
        exit(1);
    }

    // Load the parameter port
    port = (uint16_t)atoi(argv[1]);

    // Create a socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        fprintf(stderr, " Socket creation failed: %s\n", strerror(errno));
        exit(1);
    }
    // Enable reusable port
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(int)) == -1) {
        fprintf(stderr, " setsockopt error: %s\n", strerror(errno));
        exit(1);
    }

    // Build connection details sockaddr_in struct
    connection_details.sin_family = AF_INET;
    connection_details.sin_addr.s_addr = htonl(INADDR_ANY);
    connection_details.sin_port = htons(port);

    // Bind and listen
    if(bind( sock,(struct sockaddr*) &connection_details, sizeof(connection_details)) != 0)
    {
        fprintf(stderr, "Bind failed: %s\n", strerror(errno));
        exit(1);
    }
    if(listen(sock, 10 ) != 0 )
    {
        fprintf(stderr, "Listen failed: %s\n", strerror(errno));
        exit(1);
    }

    while(flag) {
        // Accept a connection
        accept_con = accept(sock, NULL, NULL);
        if (accept_con == -1) {
            fprintf(stderr, "Accept failed: %s\n", strerror(errno));
            exit(1);
        }

        C = count_printable();
        close(accept_con);
        accept_con = -1;

        if ((C == -1) && !(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE)) {
            exit(1);
        }
    }

    // SIGINT
    print_count();
}
