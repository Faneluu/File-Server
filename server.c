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
#include <stdbool.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <fcntl.h>
#include <errno.h>

#define LENGTH 80
#define PORT 8080
#define CLIENTS 2
#define EVENTS (CLIENTS + 1)
#define SA struct sockadd
#define TIMEOUT 5000

typedef struct{
    int index;
    int connfd;
}params;

struct sockaddr_in cli;
struct epoll_event ev, ret_ev, events[EVENTS];

volatile __sig_atomic_t terminate = 0;
params threadParams[CLIENTS];
pthread_attr_t attr[CLIENTS];
pthread_t threadID[CLIENTS], listenThread, terminatorThread;

int listener, newSocket, len, epfd, nrThreads = 0;
char msg[] = "Hello from server\n";

void handle_signals(int sig){

    if (sig == SIGINT){
        printf("I used SIGINT!\n");
        terminate = 1;
    }
    else if (sig == SIGTERM){
        printf("I used SIGTERM!\n");
        terminate = 1;
    }
    else
        printf("Signal unknown...\n");
}

void *handle_end(void *args)
{
    printf("Inside handle_end!\n");

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signals;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    printf("Finish handle_end\n");
}

void *handle_client(void *param)
{
    char buff[LENGTH] = {0};
    params clientThreadParam = *((params*)param);
    struct epoll_event threadEv, threadRetEv = {0};
    int myEpfd;

    printf("Hello from thread= %d with fd= %d\n", clientThreadParam.index, clientThreadParam.connfd);
    printf("Have %d threads at start\n", nrThreads);

    myEpfd = epoll_create(1);

    int flags = fcntl(clientThreadParam.connfd, F_GETFL, 0);
    fcntl(clientThreadParam.connfd, F_SETFL, flags |O_NONBLOCK);

    threadEv.data.fd = clientThreadParam.connfd;
    threadEv.events = EPOLLIN | EPOLLET;
    epoll_ctl(myEpfd, EPOLL_CTL_ADD, clientThreadParam.connfd, &threadEv);

    // infinite loop
    while (!terminate){
        int numEvents = epoll_wait(myEpfd, &threadRetEv, 1, TIMEOUT);

        if (threadRetEv.data.fd == clientThreadParam.connfd && (threadRetEv.data.fd & EPOLLIN) != 0){
            int bytesRead = read(clientThreadParam.connfd, buff, sizeof(buff));
            if(bytesRead <= 0){
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    //printf("No data available right now!\n");
                    continue;
                }   

                else{
                    perror("read");
                }
            }

            else{
                // print buffer
                printf("From client: %s\tTo client: %s", buff, msg);

                // send buffer to client
                write(clientThreadParam.connfd, msg, sizeof(msg));
                memset(buff, '\0', strlen(buff));
        
            }
        }

        //printf("Terminate is: %d\n", terminate);
    }

    printf("Thread = %d with fd= %d exit!", clientThreadParam.index, clientThreadParam.connfd);
    printf("\tHave %d threads at end\n", nrThreads);

    close(myEpfd);
    close(clientThreadParam.connfd);
    pthread_exit(NULL);
}

void *handle_connections(void *args)
{
    printf("Thread for connections created!\n");

    while (!terminate){

        int numEvents = epoll_wait(epfd, &ret_ev, EVENTS, TIMEOUT);

        if (numEvents == -1){
            printf("Error in epoll_wait\n");
            break;
        }

        if (nrThreads == CLIENTS && ret_ev.data.fd == listener  && (ret_ev.events & EPOLLIN) != 0){
            printf("Max clients reached!\n");
            close(ret_ev.data.fd);
        }

        else if (ret_ev.data.fd == listener && (ret_ev.events & EPOLLIN) != 0){
            // accept the data packet from client and verify
            newSocket = accept(listener, (SA*)&cli, &len);
            if (newSocket < 0){
                printf("Server accept failed...\n");
                exit(EXIT_FAILURE);
            }
            else{
                printf("Server accept the client..\n");

                ret_ev.data.fd = newSocket;
                ret_ev.events = EPOLLIN;
                epoll_ctl(epfd, EPOLL_CTL_ADD, newSocket, &ret_ev);

                threadParams[nrThreads].connfd = newSocket;
                threadParams[nrThreads].index = nrThreads;

                pthread_create(&threadID[nrThreads], NULL, handle_client, &threadParams[nrThreads]);
                nrThreads++;
            }
        }

        else if (ret_ev.data.fd == STDIN_FILENO && (ret_ev.events & EPOLLIN) != 0){
            char buff[LENGTH] = {0};
            int bytesRead = read(ret_ev.data.fd, buff, sizeof(buff));
            printf("Read from input: %s", buff);

            if (strncmp("quit", buff, 4) == 0){
                pthread_kill(listenThread, SIGTERM);
            }
        }

        else {
            //printf("None of this!\n");
        }
    }

    for (int i = 0; i < nrThreads; i ++){
        printf("Prepare to join: %d\n", i);
        pthread_join(threadID[i], NULL);
        printf("Joinned: %d\n", i);
    }
}

int initialiseServer()
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

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        printf("setsockopt");
    }

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

    listener = initialiseServer();
    len = sizeof(cli);

    ev.data.fd = listener;
    ev.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listener, &ev);

    pthread_create(&listenThread, NULL, handle_connections, NULL);
    pthread_create(&terminatorThread, NULL, handle_end, NULL);

    printf("Prepare to join listenThread\n");
    pthread_join(listenThread, NULL);

    printf("Prepare to join terminatorThread\n");
    pthread_join(terminatorThread, NULL);

    close(listener);

    printf("Joinned listenThread\n");
    printf("Joinned terminatorThread\n");
    printf("Exiting...\n");

    return EXIT_SUCCESS;
}