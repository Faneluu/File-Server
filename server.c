#include <stdio.h>
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

void func(int connfd)
{
    char buff[MAX];
    int n;

    // infinite loop
    while (1){
        bzero(buff, MAX);

        // read the message from client and copy it in buffer
        read(connfd, buff, sizeof(buff));

        // print buffer
        printf("From client: %s\t To client: ", buff);
        bzero(buff, MAX);
        n = 0;

        // copy server message
        while ((buff[n++] = getchar()) != '\n')
            ;

        // send buffer to client
        write(connfd, buff, sizeof(buff));

        // quit
        if (strncmp("quit", buff, 4) == 0){
            printf("Server quit...\n");
            break;
        }

    }
}

int main()
{
    int sockfd, connfd, len;
    struct sockaddr_in servaddr, cli;

    // create socket and verify
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        printf("Socket creation failed...\n");
        exit(EXIT_FAILURE);
    }
    else 
        printf("Socket succesfully created..\n");
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);

    // binding newly created socket to given IP and verify
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0){
        printf("Socket binf failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully binded...\n");

    // now server is ready to listen and verify
    if ((listen(sockfd, 5)) != 0){
        printf("Listen failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server listening...\n");
    len = sizeof(cli);

    // accept the data packet from client and verify
    connfd = accept(sockfd, (SA*)&cli, &len);
    if (connfd < 0){
        printf("Server accept failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server accept the client...\n");

    // function for chatting between client and server
    func(connfd);

    // after chatting close the socket
    close(sockfd);

    return EXIT_SUCCESS;
}