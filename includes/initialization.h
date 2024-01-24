#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>    
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <stdatomic.h>
#include <threads.h>
#include <time.h>

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <sys/epoll.h>
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
#define OUT_OF_MEMORY 4
#define SERVER_BUSY 8
#define UNKNOWN_OPERATION 10
#define BAD_ARGUMENTS 20
#define OTHER_ERROR 40

#define BUFFER_SIZE 1024
#define MAX_FILES 10
#define LENGTH 100
#define PATH_LENGTH 20
#define CLIENTS 1
#define EVENTS (CLIENTS + 1)
#define FIRST_WORDS 10

#define ALL_FILES "files/all_files.txt"
#define ROOT "root"
#define LOG_FILE "files/log.txt"
#define INSTRUCTIONS_FILE "includes/instructions_file.txt"

typedef struct{
    char word[LENGTH];
    int count;
}wordsFile;

typedef struct{
    wordsFile freqWords[FIRST_WORDS];
    char filename[LENGTH];
}files;

extern struct sockaddr_in cli;
extern struct epoll_event ev, ret_ev, events[EVENTS];

extern volatile __sig_atomic_t terminate;
extern pthread_t listenThread, terminatorThread, indexingThread;
extern pthread_mutex_t mtx, logMtx, indexMtx;
extern pthread_cond_t indexCond, listenCond;

extern thread_local int in_fd;
extern thread_local struct stat fileStats;
extern thread_local bool canDownload, isMoveOperation;
extern thread_local char sendToLog[LENGTH];

extern int listener, len, epfd, nrThreads, nrSearchFiles;
extern char listFiles[MAX_FILES + 1][LENGTH];
extern bool canIndex, printMaxClientsReached;
extern files searchFiles[MAX_FILES];

// in operations
char *select_command(char *buff);
char *upload_operation(char *token, char *savePtr);
char *delete_operation(char *token, char *savePtr);

// in utils
bool add_file(char *filePath);
char* add_root(char *filePath);
bool make_dir(const char *dirName);
char* set_status(uint32_t status);

bool check_dir(char *filePath);
bool check_five_parameters(char *token, char *savePtr, uint32_t *pBytesPath, uint32_t *pOffset, uint32_t *pByteContent, char **pFilePath, char **pNewContent);
bool check_four_parameters(char *token, char *savePtr, uint32_t *pBytesPath, char **pFilePath, uint32_t *pBytesContent, char **pFileContent);
bool check_two_parameters(char *token, char *savePtr, uint32_t *pBytesPath, char **pFilePath);
bool check_length(const char *f1, const char *f2);
bool find_file(const char *filePath);

bool send_upload_operation(uint32_t bytesOutFile, char *inFilePath, char *outFilePath);
char *send_delete_operation(uint32_t bytesInFile, char *inFilePath);
bool write_log();
char *show_instructions();

void indexFiles();
files getSearchFile(char *file);
int first_appearance(wordsFile **words, int size, char *wordName);
bool check_nr_files();
int index_listFiles();