#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#define MAX 1024
#define PORT 8080
#define SA struct sockadd

void client_func(int sockfd)
{
    char buff[MAX] = {0};
    int n;
    bool isDownload = false;

    while (1){
        
        if (isDownload){
            printf("\n");
            isDownload = false;
        }
        
        printf("Enter the string: ");
        n = 0;

        // read from buffer
        while ((buff[n++] = getchar()) != '\n')
            ;

        // write to server
        write(sockfd, buff, sizeof(buff));

        // quit
        if (strncmp(buff, "quit\n", strlen(buff)) == 0){
            printf("Client quit...\n");
            break;
        }

        else if (buff[0] == '1' && buff[1] != '0')
            isDownload = true;

        memset(buff, '\0', strlen(buff));

        // read from server
        if (read(sockfd, buff, sizeof(buff)) <= 0)
            break;
        
        printf("From server: %s", buff);

        // if (isDownload){

        //     //operation was succeed
        //     if (buff[0] == '0'){
        //         memset(buff, '\0', strlen(buff));

        //         printf("Preapre to read file\n");
        //         while (read(sockfd, buff, sizeof(buff)) > 0){
        //             printf("Content: %s", buff);

        //             if (buff[strlen(buff) - 1] == '\n')
        //                 break;

        //             memset(buff, '\0', strlen(buff));
        //             printf("Wait to read again\n");
        //         }
        //         printf("\n");
        //     }
        //     isDownload = false;
        // }
        
        memset(buff, '\0', strlen(buff));
    }
}

int main()
{
    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;

    // socket create and verify
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        printf("Socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully created..\n");
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
    client_func(sockfd);

    // close the socket
    close(sockfd);

    return EXIT_SUCCESS;
}