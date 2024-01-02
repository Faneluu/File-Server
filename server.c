#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>

#define LENGTH 80
#define PORT 8080
#define CLIENTS 1
#define EVENTS (CLIENTS + 1)
#define SA struct sockadd

typedef struct{
    int index;
    int connfd;
}params;

int listener, newSocket, len, epfd, nrThreads = 0;
struct sockaddr_in cli;
struct epoll_event ev, events[CLIENTS];
char msg[] = "Hello from server\n";

void *handle_client(void *param)
{
    char buff[LENGTH] = {0};
    params threadParams = *((params*)param);

    printf("Hello from thread= %d with fd= %d\n", threadParams.index, threadParams.connfd);
    printf("Have %d threads at start\n", nrThreads);
    // infinite loop
    while (read(threadParams.connfd, buff, sizeof(buff)) > 0){
        
        // print buffer
        printf("From client: %s\tTo client: %s", buff, msg);

        // send buffer to client
        write(threadParams.connfd, msg, sizeof(msg));
        memset(buff, '\0', strlen(buff));
    }

    close(threadParams.connfd);    

    printf("Thread = %d with fd= %d exit!\n", threadParams.index, threadParams.connfd);
    
    nrThreads--;
    printf("Have %d threads at end\n", nrThreads);
    pthread_exit(NULL);
}

int initialiaseServer()
{
    int sockfd;
    struct sockaddr_in servaddr;

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
        printf("Socket bind failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Socket successfully binded..\n");

    // now server is ready to listen and verify
    if ((listen(sockfd, CLIENTS)) != 0){
        printf("Listen failed...\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("Server listening..\n");

    return sockfd;
}

int main()
{
    epfd = epoll_create(CLIENTS);

    ev.data.fd = STDIN_FILENO;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    listener = initialiaseServer();
    len = sizeof(cli);

    ev.data.fd = listener;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev);

    while (1){

        int numEvents = epoll_wait(epfd, &ev, EVENTS, -1);

        if (numEvents == -1){
            printf("Error in epoll_wait\n");
            break;
        }

        if (ev.data.fd == listener && nrThreads == CLIENTS && (ev.events & EPOLLIN) != 0){
            printf("Max clients reached!\n");
            close(ev.data.fd);
        }

        else if (ev.data.fd == listener && (ev.events & EPOLLIN) != 0){
            // accept the data packet from client and verify
            newSocket = accept(listener, (SA*)&cli, &len);
            if (newSocket < 0){
                printf("Server accept failed...\n");
                exit(EXIT_FAILURE);
            }
            else{
                printf("Server accept the client..\n");
                
                ev.data.fd = newSocket;
                ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, newSocket, &ev);

                pthread_t threadID;
                params threadParams;

                threadParams.connfd = newSocket;
                threadParams.index = nrThreads;

                pthread_create(&threadID, NULL, handle_client, &threadParams);
                nrThreads++;
            }
        }

        else if (ev.data.fd == STDIN_FILENO && (ev.events & EPOLLIN) != 0){
            char buff[LENGTH] = {0};
            int bytesRead = read(ev.data.fd, buff, sizeof(buff));
            printf("Read from input: %s", buff);

            if (strncmp("quit", buff, 4) == 0){
                break;
            }
        }
    }
    
    printf("Exiting...\n");


    // after chatting close the socket
    close(listener);

    return EXIT_SUCCESS;
}