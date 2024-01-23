#include "../includes/initialization.h"

struct sockaddr_in cli;
struct epoll_event ev, ret_ev, events[EVENTS];

volatile __sig_atomic_t terminate = 0;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER, logMtx = PTHREAD_MUTEX_INITIALIZER, indexMtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t indexCond = PTHREAD_COND_INITIALIZER;
pthread_t listenThread, terminatorThread, indexingThread;

thread_local int in_fd;
thread_local struct stat fileStats;
thread_local bool canDownload = false;
thread_local char sendToLog[LENGTH] = {0};

int listener, newSocket, len, epfd, nrThreads = 0, nrFiles = 0, nrSearchFiles = 0;
char listFiles[MAX_FILES][LENGTH] = {0};
bool canIndex = false;

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
    //printf("Inside handle_end!\n");

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = end_signals;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    //printf("Finish handle_end\n");
}

void *handle_client(void *args)
{
    char buff[BUFFER_SIZE] = {0}, *msg;
    params clientThreadParam = *((params*)args);
    struct epoll_event threadEv, threadRetEv = {0};
    int myEpfd;

    //printf("Hello from thread= %d with fd= %d\n", clientThreadParam.index, clientThreadParam.connfd);
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
                    //perror("read");
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

                // print buffer
                //printf("From client: %s\tTo client: %s", buff, msg);

                // download
                if (canDownload){
                    //printf("\nDownloaded from server with clientfd= %d and filefd= %d with size= %d\n", clientThreadParam.connfd, in_fd, fileStats.st_size);

                    off_t offset = 0;
                    int cork = 1;

                    setsockopt(clientThreadParam.connfd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));

                    // send buffer to client
                    write(clientThreadParam.connfd, msg, strlen(msg));

                    ssize_t results = sendfile(clientThreadParam.connfd, in_fd, &offset, fileStats.st_size);

                    //printf("Bytes sent to client= %d\n", results);

                    if (results < 0){
                        perror("sendfile");
                    }

                    close(in_fd);
                    canDownload = false;

                    cork = 0;
                    setsockopt(clientThreadParam.connfd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));
                }

                else {
                    // send buffer to client
                    write(clientThreadParam.connfd, msg, strlen(msg));
                }

                memset(buff, '\0', strlen(buff));
                free(msg);
            }
        }

        //printf("Terminate is: %d\n", terminate);
    }

    //printf("Thread = %d with fd= %d exit!", clientThreadParam.index, clientThreadParam.connfd);
    

    //epoll_ctl(epfd, EPOLL_CTL_DEL, clientThreadParam.connfd, &ev);

    pthread_mutex_lock(&mtx);
    nrThreads--;
    pthread_mutex_unlock(&mtx);

    printf("\tHave %d threads\n", nrThreads);

    close(myEpfd);
    close(clientThreadParam.connfd);
    pthread_exit(NULL);
}

void *handle_indexing(void *args)
{
    //printf("Create thread for handling indexing!\n");

    indexFiles();

    while (!terminate){
        pthread_mutex_lock(&indexMtx);

        while (!canIndex && !terminate)
            pthread_cond_wait(&indexCond, &indexingThread);

        //printf("Work with indexing\n");

        indexFiles();

        canIndex = false;
        pthread_mutex_unlock(&indexMtx);
    }

    //printf("Exit indexingThread\n");
}

void *handle_connections(void *args)
{
    //printf("Thread for connections created!\n");
    bool printMaxClientsReached = false;
    pthread_attr_t threadAttr;

    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);

    while (!terminate){

        int numEvents = epoll_wait(epfd, &ret_ev, EVENTS, TIMEOUT);

        if (numEvents == -1){
            printf("Error in epoll_wait\n");
            break;
        }

        if (nrThreads == CLIENTS && ret_ev.data.fd == listener  && (ret_ev.events & EPOLLIN) != 0 && !printMaxClientsReached){
            printf("Max clients reached!\n");
            close(ret_ev.data.fd);
            printMaxClientsReached = true;
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

                params threadParameters;
                pthread_t threadClient;
                
                threadParameters.connfd = newSocket;
                threadParameters.index  = nrThreads;

                pthread_create(&threadClient, &threadAttr, handle_client, &threadParameters);
                
                nrThreads++;
            }
        }

        else if (ret_ev.data.fd == STDIN_FILENO && (ret_ev.events & EPOLLIN) != 0){
            char buff[LENGTH] = {0};
            int bytesRead = read(ret_ev.data.fd, buff, sizeof(buff));
            printf("Read from input: %s", buff);

            if (strncmp(buff, "quit\n", strlen(buff)) == 0){
                pthread_kill(listenThread, SIGTERM);
            }
        }

        else {
        }
    }

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
    if ((listen(sockfd, 10 * CLIENTS)) != 0){
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

    char buff[LENGTH] = {0};

    while (fgets(buff, LENGTH, f) != NULL){
        buff[strcspn(buff, "\n")] = '\0';
        memcpy(listFiles[nrFiles], buff, strlen(buff));
        nrFiles++;
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

    //printf("Prepare to join listenThread\n");
    pthread_join(listenThread, NULL);

    //printf("Prepare to join terminatorThread\n");
    pthread_join(terminatorThread, NULL);

    //printf("Prepare to join indexingThread\n");
    pthread_join(indexingThread, NULL); 

    close(listener);

    //printf("Joinned listenThread\n");
    //printf("Joinned terminatorThread\n");
    //printf("Joinned indexingThread\n");

    printf("Exiting...\n");

    return EXIT_SUCCESS;
}