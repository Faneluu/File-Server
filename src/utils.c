#include "../includes/initialization.h"

bool add_file(char *filePath)
{
    // save the file
    FILE *f = fopen(ALL_FILES, "a");

    if (filePath[0] != '\0')
        fprintf(f, "%s\n", filePath);
    
    fclose(f);
    return true;
}

char *add_root(char *filePath)
{
    char *root;

    root = calloc((strlen(ROOT) + strlen(filePath) + 2), sizeof(char));
    memcpy(root, ROOT, strlen(ROOT));

    if (filePath[0] != '/')
        root[strlen(root)] = '/';

    memcpy(root + strlen(root), filePath, strlen(filePath));

    return root;
}

bool make_dir(const char *dirName)
{
    DIR *dir = opendir(dirName);

    if (dir == NULL){
        if (errno == ENOENT){
            int status = mkdir(dirName, 0777);

            if (status == -1){
                perror("mkdir");
                return false;    
            }

            return true;
        }

        else {
            perror("opendir");
            return false;
        }
    }

    closedir(dir);
    return true;
}

char *set_status(uint32_t status)
{
    char *msg = calloc(4, sizeof(char));

    snprintf(msg, 4, "%u\n", status);

    return msg;
}

bool check_dir(char *filePath)
{
    char *str, *token, *savePtr, *realPath;
    int i = 1, nrDir = 0;

    // check path
    if (filePath[0] != '/')
        return false;

    // see if has dir
    while (filePath[i] != '\0'){
        if (filePath[i] == '/'){
            nrDir++;
            break;
        }
        i++;
    }

    // don t have dir
    if (!nrDir)
        return true;

    // check dir
    str = calloc((strlen(filePath) + 1), sizeof(char));
    memcpy(str, filePath, strlen(filePath));
    token = strtok_r(str, "/", &savePtr);

    realPath = add_root(token);

    make_dir(realPath);

    free(realPath);
    free(str);
    return true;
}

bool check_five_parameters(char *token, char *savePtr, uint32_t *pBytesPath, uint32_t *pOffset, uint32_t *pByteContent, char **pFilePath, char **pNewContent)
{
    char *filePath, *newContent;
    uint32_t bytesPath, offset, bytesContent;

    // check paramters and start with bytesPath
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL)
        return false;
    
    bytesPath = atoi(token);
    if (bytesPath == 0)
        return false;

    // check filePath
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL)
        return false;
    
    filePath = calloc(bytesPath + 1, sizeof(char));
    if (bytesPath > strlen(token))
        memcpy(filePath, token, strlen(token));
    else 
        memcpy(filePath, token, bytesPath);

    // check offset
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL){
        free(filePath);
        return false;
    }
    
    offset = atoi(token);
    if (offset == 0){
        free(filePath);
        return false;
    }

    // check bytesContent
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL){
        free(filePath);
        return false;
    }
    
    bytesContent = atoi(token);
    if (bytesContent == 0){
        free(filePath);
        return false;
    }

    // check newContent
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL){
        free(filePath);
        return false;
    }

    newContent = calloc(bytesContent + 1, sizeof(char));
    if (bytesContent > strlen(token))
        memcpy(newContent, token, strlen(token));
    else 
        memcpy(newContent, token, bytesContent);

    // check if has more parameters than required
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token != NULL){
        free(filePath);
        free(newContent);
        return false;
    }

    (*pBytesPath) = bytesPath;
    (*pOffset) = offset;
    (*pByteContent) = bytesContent;
    (*pFilePath) = filePath;
    (*pNewContent) = newContent;

    return true;
}

bool check_four_parameters(char *token, char *savePtr, uint32_t *pBytes1, char **pStr1, uint32_t *pBytes2, char **pStr2)
{
    char *str1, *str2;
    uint32_t bytes1, bytes2;

    // check paramters and start with bytesPath
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL)
        return false;
    
    bytes1 = atoi(token);
    if (bytes1 == 0)
        return false;

    // check filePath
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL)
        return false;
    
    str1 = calloc(bytes1 + 1, sizeof(char));
    if (bytes1 > strlen(token))
        memcpy(str1, token, strlen(token));
    else 
        memcpy(str1, token, bytes1);

    // check bytesContent
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL){
        free(str1);
        return false;
    }
    
    bytes2 = atoi(token);
    if (bytes2 == 0){
        free(str1);
        return false;
    }

    // check fileContent
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL){
        free(str1);
        return false;
    }

    str2 = calloc(bytes2 + 1, sizeof(char));
    if (bytes2 > strlen(token))
        memcpy(str2, token, strlen(token));
    else 
        memcpy(str2, token, bytes2);

    // check if has more parameters than required
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token != NULL){
        free(str1);
        free(str2);
        return false;
    }

    (*pBytes1) = bytes1;
    (*pStr1) = str1;
    (*pBytes2) = bytes2;
    (*pStr2) = str2;

    return true;
}

bool check_two_parameters(char *token, char *savePtr, uint32_t *pBytesPath, char **pFilePath)
{
    char *filePath;
    uint32_t bytesPath;

    // check bytesPath
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL)
        return false;

    bytesPath = atoi(token);
    if (bytesPath == 0)
        return false;

    // check filePath
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token == NULL)
        return false;

    filePath = calloc(bytesPath + 1 , sizeof(char));
    if (bytesPath > strlen(token))
        memcpy(filePath, token, strlen(token));
    else 
        memcpy(filePath, token, bytesPath);

    // check nr parameters
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token != NULL){
        free(filePath);
        return false;
    }

    (*pBytesPath) = bytesPath;
    (*pFilePath) = filePath;

    return true;
}

bool check_length(const char *f1, const char *f2)
{
    if (strncmp(f1, f2, strlen(f1)) == 0 && strncmp(f1, f2 , strlen(f2)) == 0)
        return true;

    return false;
}

bool find_file(const char *filePath)
{
    // find the file
    for (int i = 0; i < MAX_FILES; i++){
        if (check_length(listFiles[i], filePath)){
            return true;
        }
    }
    return false;
}

bool send_upload_operation(uint32_t bytesOutFile, char *inFilePath, char *outFilePath)
{
    char *msg, *newToken, *newSavePtr, *str, *inRealPath;
    int fd;
    struct stat inFileStats;

    // lock mutex
    pthread_mutex_lock(&mtx);

    // send upload command for the second path
    str = calloc(BUFFER_SIZE, sizeof(char));
    inRealPath = add_root(inFilePath);
    FILE *f = fopen(inRealPath, "r");

    fd = fileno(f);
    fstat(fd, &inFileStats);

    snprintf(str, LENGTH, "2;%d;%s;%d;", bytesOutFile, outFilePath, inFileStats.st_size);

    char buff[LENGTH] = {0};

    while (fgets(buff, LENGTH, f) != NULL){
        buff[strcspn(buff, "\n")] = '\0';
        memcpy(str + strlen(str), buff, strlen(buff));
        memset(buff, '\0', strlen(buff));
    }
    str[strlen(str)] = '\n'; 

    fclose(f);
    free(inRealPath);

    isMoveOperation = true;

    // unlock mutex
    pthread_mutex_unlock(&mtx);

    newToken = strtok_r(str, ";\n", &newSavePtr);

    msg = upload_operation(newToken, newSavePtr);

    free(str);
    
    // send error status
    if (msg[0] != '0'){
        free(msg);
        return false;
    }

    free(msg);
    return true;
}

char *send_delete_operation(uint32_t bytesInFile, char *inFilePath)
{
    char *msg, *str, *newToken, *newSavePtr;

    str = calloc(BUFFER_SIZE, sizeof(char));
    snprintf(str, LENGTH, "4;%d;%s\n", bytesInFile, inFilePath);

    newToken = strtok_r(str, ";\n", &newSavePtr);
    msg = delete_operation(newToken, newSavePtr);

    free(str);
    return msg;
}

bool write_log()
{
    pthread_mutex_lock(&logMtx);

    time_t currentTime = time(NULL);
    struct tm localTime;
    char *timeStr;

    localtime_r(&currentTime, &localTime);

    timeStr = calloc((LENGTH + strlen(sendToLog)), sizeof(char));
    strftime(timeStr, LENGTH, "%d-%m-%Y, %H:%M:%S, ", &localTime);
    
    memcpy(timeStr + strlen(timeStr), sendToLog, strlen(sendToLog));

    FILE *f = fopen(LOG_FILE, "a");
    fprintf(f, "%s\n", timeStr);
    fclose(f);

    free(timeStr);

    pthread_mutex_unlock(&logMtx);

    return true;
}

char *show_instructions()
{
    char *msg, *buff;
    struct stat instructionsStats;
    uint32_t buffSize = 2 * LENGTH;

    stat(INSTRUCTIONS_FILE, &instructionsStats);

    msg = calloc(instructionsStats.st_size + 1, sizeof(char));
    buff = calloc(buffSize, sizeof(char));

    pthread_mutex_lock(&mtx);

    FILE * f = fopen(INSTRUCTIONS_FILE, "r");

    while (fgets(buff, buffSize, f) != NULL){
        memcpy(msg + strlen(msg), buff, strlen(buff));
        memset(buff, '\0', strlen(buff));
    }

    fclose(f);

    pthread_mutex_unlock(&mtx);

    free(buff);
    return msg;
}

int first_appearance(wordsFile **words, int size, char *wordName)
{
    for (int i = 0; i < size; i ++){
        if (check_length((*words)[i].word, wordName))
            return i;
    }

    //printf("Exit first_appearance()!\n");
    return -1;
}

int compareWords(const void *a, const void *b)
{
    wordsFile wordA = *(wordsFile *)a;
    wordsFile wordB = *(wordsFile *)b;

    return wordB.count - wordA.count;
}

files getSearchFile(char *file)
{
    files searchFile = {0};
    wordsFile *words;
    int nrWords = 0, index;
    char *buff, *str, *token, *savePtr, *filePath;
    
    buff = calloc(LENGTH + 1, sizeof(char));
    str = calloc(LENGTH + 1, sizeof(char));
    words = calloc(LENGTH, sizeof(wordsFile));

    filePath = add_root(file);
    FILE *f = fopen(filePath, "r");

    while (fgets(buff, BUFFER_SIZE, f) != NULL){
        memcpy(str, buff, strlen(buff));

        // take every word
        token = strtok_r(str, " ,?!.\n", &savePtr);
        while (token != NULL){
            index = first_appearance(&words, nrWords, token);
            
            // is first appearance
            if (index == -1){
                memcpy(words[nrWords].word, token, strlen(token));
                words[nrWords].count++;
                nrWords++;
            }
            else
                words[index].count++;

            token = strtok_r(NULL, " ,?!.\n", &savePtr);
        }
        memset(str, '\0', strlen(str));
    }

    qsort(words, nrWords, sizeof(wordsFile), compareWords);

    for (int i = 0; i < FIRST_WORDS; i ++){
        memcpy(searchFile.freqWords[i].word, words[i].word, strlen(words[i].word));
        searchFile.freqWords[i].count = words[i].count;
    }

    fclose(f);
    free(buff);
    free(words);
    free(str);
    free(filePath);

    return searchFile;
}

void indexFiles()
{
    nrSearchFiles = 0;


    for (int i = 0; i < MAX_FILES + 1; i ++){

        if (listFiles[i][0] != '\0'){
            memset(searchFiles[nrSearchFiles].filename, '\0', strlen(searchFiles[nrSearchFiles].filename));
            searchFiles[nrSearchFiles] = getSearchFile(listFiles[i]);
            memcpy(searchFiles[nrSearchFiles].filename, listFiles[i], strlen(listFiles[i]));
            nrSearchFiles++;
        }
    }
}

bool check_nr_files()
{
    int count = 0;
    for (int i = 0; i < MAX_FILES + 1; i ++){
        if (listFiles[i][0] != '\0')
            count++;
    }

    printf("Count is: %d and MAX_FILES: %d\n", count, MAX_FILES);

    if (count == MAX_FILES && isMoveOperation){
        printf("First case!!\n");
        return true;}

    if (count >= MAX_FILES){
        printf("Second case!!\n");
        return false;}

    return true;
}

int index_listFiles()
{
    for (int i = 0; i < MAX_FILES + 1; i ++){
        if (listFiles[i][0] == '\0'){
            printf("Free index: %d\n", i);
            return i;
        }
    }

    return 0;
}