#include "../includes/initialization.h"

struct sockaddr_in cli;
struct epoll_event ev, ret_ev, events[EVENTS];

volatile __sig_atomic_t terminate = 0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER, logMtx = PTHREAD_MUTEX_INITIALIZER, indexMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t indexCond = PTHREAD_COND_INITIALIZER, listenCond = PTHREAD_COND_INITIALIZER;
pthread_t listenThread, terminatorThread, indexingThread;

thread_local int in_fd;
thread_local struct stat fileStats;
thread_local bool canDownload = false, isMoveOperation = false;
thread_local char sendToLog[LENGTH] = {0};

int listener, len, epfd, nrThreads = 0, nrSearchFiles = 0;
char listFiles[MAX_FILES + 1][LENGTH] = {0};
bool canIndex = false, printMaxClientsReached = false;

files searchFiles[MAX_FILES] = {0};

void end_signals(int sig){

    if (sig == SIGINT){
        printf("GOT SIGINT!\n");
        terminate = 1;
        pthread_cond_signal(&indexCond);
    }
    else if (sig == SIGTERM){
        printf("GOT SIGTERM!\n");
        terminate = 1;
        pthread_cond_signal(&indexCond);
    }
    else
        printf("Signal unknown...\n");
}

void *handle_end(void *args)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = end_signals;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

void *handle_client(void *args)
{
    char buff[BUFFER_SIZE] = {0}, *msg;
    int myEpfd, flags, clientSocket = *((int*)args);
    struct epoll_event threadEv = {0}, threadRetEv = {0};

    myEpfd = epoll_create(1);

    flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags |O_NONBLOCK);

    threadEv.data.fd = clientSocket;
    threadEv.events = EPOLLIN | EPOLLET;
    epoll_ctl(myEpfd, EPOLL_CTL_ADD, clientSocket, &threadEv);

    // infinite loop
    while (!terminate){
        int numEvents = epoll_wait(myEpfd, &threadRetEv, 1, TIMEOUT);

        if (threadRetEv.data.fd == clientSocket && (threadRetEv.data.fd & EPOLLIN) != 0){
            int bytesRead = read(clientSocket, buff, sizeof(buff));
            if(bytesRead <= 0){
                if (errno == EAGAIN || errno == EWOULDBLOCK){
                    continue;
                }                           

                else{
                    break;
                }
            }

            else{
                
                // quit
                if (strncmp(buff, "quit\n", strlen(buff)) == 0)
                    break;

                
                if (strncmp(buff, "help\n", strlen(buff)) == 0)
                    msg = show_instructions();

                else 
                    msg = select_command(buff);

                // download
                if (canDownload){
                    off_t offset = 0;
                    int cork = 1;

                    // set TCP_CORK to put it on a stack
                    setsockopt(clientSocket, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));

                    // send buffer to client
                    write(clientSocket, msg, strlen(msg));

                    ssize_t results = sendfile(clientSocket, in_fd, &offset, fileStats.st_size);

                    if (results < 0){
                        perror("sendfile");
                    }

                    close(in_fd);
                    canDownload = false;

                    cork = 0;
                    setsockopt(clientSocket, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));
                }

                else {
                    // send buffer to client
                    write(clientSocket, msg, strlen(msg));
                }

                memset(buff, '\0', strlen(buff));
                free(msg);
            }
        }
    }

    pthread_mutex_lock(&mtx);

    nrThreads--;
    printMaxClientsReached = false;
    close(myEpfd);
    close(clientSocket);

    pthread_mutex_unlock(&mtx);

    pthread_cond_signal(&listenCond);

    pthread_exit(NULL);
}

void *handle_indexing(void *args)
{
    indexFiles();

    while (!terminate){
        pthread_mutex_lock(&indexMtx);

        while (!canIndex && !terminate)
            pthread_cond_wait(&indexCond, &indexMtx);

        if (!terminate)
            indexFiles();

        canIndex = false;
        pthread_mutex_unlock(&indexMtx);
    }
}

void *handle_connections(void *args)
{
    pthread_attr_t threadAttr;
    char buff[LENGTH] = {0};
    uint32_t stats, net_stats;

    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    while (!terminate){
        int numEvents = epoll_wait(epfd, &ret_ev, EVENTS, TIMEOUT);

        if (numEvents == -1){
            printf("Error in epoll_wait\n");
            break;
        }

        if (ret_ev.data.fd == listener && (ret_ev.events & EPOLLIN) != 0){
            // accept the data packet from client and verify
            int newSocket = accept(listener, (SA*)&cli, &len);
            if (newSocket < 0){
                printf("Server accept failed...\n");
                pthread_kill(listenThread, SIGTERM);
                break;
            }

            ret_ev.data.fd = newSocket;
            ret_ev.events = EPOLLIN;
            epoll_ctl(epfd, EPOLL_CTL_ADD, newSocket, &ret_ev);

            // max clients
            if (nrThreads == CLIENTS){
                if (!printMaxClientsReached){
                    printf("Max clients reached!\n");
                    printMaxClientsReached = true;
                }
        
                stats = SERVER_BUSY;
                net_stats = htonl(stats);
                write(newSocket, &net_stats, sizeof(net_stats));
                close(newSocket);
            }
            else{
                printf("Server accept the client..\n");

                stats = 1;
                net_stats = htonl(stats);
                write(newSocket, &net_stats, sizeof(net_stats));

                pthread_t clientThread;
                pthread_create(&clientThread, &threadAttr, handle_client, &newSocket);
                
                nrThreads++;
            }
        }

        else if (ret_ev.data.fd == STDIN_FILENO && (ret_ev.events & EPOLLIN) != 0){
            memset(buff, '\0', LENGTH);
            int bytesRead = read(ret_ev.data.fd, buff, sizeof(buff));
            printf("Read from input: %s", buff);

            if (strncmp(buff, "quit\n", strlen(buff)) == 0){
                pthread_kill(listenThread, SIGTERM);
            }
        }
    }

    
    while (nrThreads)
        pthread_cond_wait(&listenCond, &mtx);

    pthread_attr_destroy(&threadAttr);
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

    // create root folder if doesn t exist
    make_dir(ROOT);

    // initialise listFiles
    FILE *f = fopen(ALL_FILES, "r");

    if (f == NULL)
        return sockfd;

    char buff[LENGTH + 1] = {0};
    int count = 0;

    while (fgets(buff, LENGTH, f) != NULL){
        buff[strcspn(buff, "\n")] = '\0';
        memcpy(listFiles[count], buff, strlen(buff));
        count++;
        memset(buff, '\0', strlen(buff));
    }
    fclose(f);
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
    pthread_create(&indexingThread, NULL, handle_indexing, NULL);
    
    pthread_join(listenThread, NULL);
    pthread_join(terminatorThread, NULL);
    pthread_join(indexingThread, NULL);

    close(listener);
    close(epfd);

    printf("Exiting...\n");

    return EXIT_SUCCESS;
}