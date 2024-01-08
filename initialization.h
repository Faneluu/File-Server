#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <pthread.h>    
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signalfd.h>

#define LENGTH 80
#define PORT 8080
#define CLIENTS 2
#define EVENTS (CLIENTS + 1)
#define SA struct sockadd
#define TIMEOUT 5000
#define MAX_FILES 10

#define SUCCESS 0
#define FILE_NOT_FOUND 1
#define UNKNOWN_OPERATION 10
#define BAD_ARGUMENTS 20
#define OTHER_ERROR 40

#define ALL_FILES "all_files.txt"

typedef struct{
    int index;
    int connfd;
}params;

extern struct sockaddr_in cli;
extern struct epoll_event ev, ret_ev, events[EVENTS];

extern volatile __sig_atomic_t terminate;
extern params threadParams[CLIENTS];
extern pthread_attr_t attr[CLIENTS];
extern pthread_t threadID[CLIENTS], listenThread, terminatorThread;

extern int listener, newSocket, len, epfd, nrThreads, nrFiles;
extern char listFiles[MAX_FILES][LENGTH];

char *select_command(char *buff);
