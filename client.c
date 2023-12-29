#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX 80
#define PORT 8080
#define SA struct sockadd

void func(int sockfd)
{
    char buff[MAX];
    int n;

    while (1){
        bzero(buff, sizeof(buff));
        
        printf("Enter the string: ");
        n = 0;

        // read from buffer
        while ((buff[n++] = getchar()) != '\n')
            ;

        // write to server
        write(sockfd, buff, sizeof(buff));
        bzero(buff, sizeof(buff));

        // read from server
        read(sockfd, buff, sizeof(buff));
        printf("From server: %s", buff);

        // quit
        if (strncmp(buff, "quit", 4) == 0){
            printf("Client quit...\n");
            break;
        }
    }
}

int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // socket crate and verify
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        printf("Socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully created...\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0){
        printf("Connection with server failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Connected to server..\n");

    // function for chat
    func(sockfd);

    // close the socket
    close(sockfd);

    return EXIT_SUCCESS;
}