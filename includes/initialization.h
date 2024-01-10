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
#include <stdatomic.h>
#include <threads.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <sys/sendfile.h>

#define PORT 8080
#define SA struct sockadd
#define TIMEOUT 5000

#define SUCCESS 0
#define FILE_NOT_FOUND 1
#define UNKNOWN_OPERATION 10
#define BAD_ARGUMENTS 20
#define OTHER_ERROR 40

#define BUFFER_SIZE 1024
#define MAX_FILES 10
#define LENGTH 100
#define PATH_LENGTH 20
#define CLIENTS 2
#define EVENTS (CLIENTS + 1)

#define ALL_FILES "files/all_files.txt"
#define ROOT "root"

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

extern thread_local int in_fd;
extern thread_local struct stat fileStats;
extern thread_local bool canDownload;

extern int listener, newSocket, len, epfd, nrThreads, nrFiles;
extern char listFiles[MAX_FILES][LENGTH];

// in operations
char *select_command(char *buff);
char *upload_operation(char *token, char *savePtr);
char *delete_operation(char *token, char *savePtr);
char *list_operation();

// in utils
bool add_file(char *filePath);
char *add_root(char *filePath);
bool make_dir(const char *dirName);
char *set_status(uint32_t status);

bool check_dir(char *filePath);
bool check_four_parameters(char *token, char *savePtr, uint32_t *pBytesPath, char **pFilePath, uint32_t *pBytesContent, char **pFileContent);
bool check_two_parameters(char *token, char *savePtr, uint32_t *pBytesPath, char **pFilePath);
bool check_length(const char *f1, const char *f2);
bool find_file(const char *filePath);

bool send_upload_operation(uint32_t bytesOutFile, char *inFilePath, char *outFilePath);
char *send_delete_operation(uint32_t bytesInFile, char *inFilePath);