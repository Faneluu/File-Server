#include "../includes/initialization.h"

char *list_operation()
{
    printf("Enter list_operation!\n");
    char *msg, *str;
    uint32_t msgSize = 20;

    str = calloc(nrFiles * PATH_LENGTH + nrFiles + 1, sizeof(char));
    msg = calloc(nrFiles * PATH_LENGTH + nrFiles + 1 + msgSize, sizeof(char));

    // lock mutex
    pthread_mutex_lock(&mtx);

    for (int i = 0; i < nrFiles; i ++){
        memcpy(str + strlen(str), listFiles[i], strlen(listFiles[i]));
        str[strlen(str)] = ' ';
    }

    
    //printf("str is: %s\n", str);
    snprintf(msg, (strlen(str) + msgSize), "0; %d; %s\n", strlen(str), str);
    //printf("msg is: %s\n", msg);

    // unlock mutex
    pthread_mutex_unlock(&mtx);
    
    memset(sendToLog, '\0', strlen(sendToLog));
    snprintf(sendToLog, LENGTH, "LIST");
    write_log();

    free(str);
    return msg;
}

char *download_operation(char *token, char *savePtr)
{
    printf("Enter download_operation()!\n");
    char *msg, *filePath, *realPath;
    uint32_t bytesPath;

    // lock mutex
    pthread_mutex_lock(&mtx);

    // check parameters
    if (!check_two_parameters(token, savePtr, &bytesPath, &filePath)){
        pthread_mutex_unlock(&mtx);
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    // find file
    if (find_file(filePath)){
        realPath = add_root(filePath);
        printf("Realpath is: '%s'\n", realPath);

        in_fd = open(realPath, O_RDONLY);

        if(fstat(in_fd, &fileStats) < 0){
            perror("fstat");
            close(in_fd);
        }

        canDownload = true;
        free(realPath);
        
        msg = calloc(10, sizeof(char));
        snprintf(msg, 10, "%d; %d; ", SUCCESS, fileStats.st_size);
    }
    else
        msg = set_status(FILE_NOT_FOUND);

    // unlock mutex
    pthread_mutex_unlock(&mtx);

    memset(sendToLog, '\0', strlen(sendToLog));
    snprintf(sendToLog, LENGTH, "DOWNLOAD");
    write_log();

    free(filePath);
    return msg;
}

char *upload_operation(char *token, char *savePtr)
{
    printf("Enter upload_operation!\n");
    char *msg, *filePath, *fileContent, *realPath;
    uint32_t bytesPath, bytesContent;

    // lock mutex
    pthread_mutex_lock(&mtx);

    if (!check_four_parameters(token, savePtr, &bytesPath, &filePath, &bytesContent, &fileContent)){
        msg = set_status(BAD_ARGUMENTS);
        pthread_mutex_unlock(&mtx);
        return msg;
    }

    if (!check_dir(filePath)){
        msg = set_status(BAD_ARGUMENTS);
        pthread_mutex_unlock(&mtx);
        return msg;
    }

    // create file descriptor
    realPath = add_root(filePath);

    FILE *f = fopen(realPath, "w");

    // write the content
    fprintf(f, "%s", fileContent);
    fclose(f);

    // add if doesn t exist
    if (!find_file(filePath))
        add_file(filePath);

    canIndex = true;

    // unlock mutex
    pthread_mutex_unlock(&mtx);
    
    pthread_cond_signal(&indexCond);

    memset(sendToLog, '\0', strlen(sendToLog));
    snprintf(sendToLog, LENGTH, "UPLOAD, %s", filePath);
    write_log();

    // free memory
    free(realPath);
    free(filePath);
    free(fileContent);
     
    msg = set_status(SUCCESS);
    printf("Exit upload_operation() with msg: '%s'!\n", msg);
    return msg;
}

char *delete_operation(char *token, char *savePtr)
{
    printf("Enter delete_operation()!\n");
    char *msg, *filePath, *realPath;
    uint32_t bytesPath, allFiles;
    bool changeFiles = false;

    // lock mutex
    pthread_mutex_lock(&mtx);

    if (!check_two_parameters(token, savePtr, &bytesPath, &filePath)){
        pthread_mutex_unlock(&mtx);
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    printf("Check file\n");
    // see if file exists and remove it
    for (int i = 0; i < nrFiles; i ++){
        if (check_length(listFiles[i], filePath)){
            memset(listFiles[i], '\0', strlen(listFiles[i]));
            changeFiles = true;
            break;
        }
    }   
    
    printf("Update file\n");
    // update the file
    if (changeFiles){
        realPath = add_root(filePath);
        remove(realPath);

        FILE *f = fopen(ALL_FILES, "w");
        fclose(f);

        allFiles = nrFiles;
        nrFiles = 0;

        for (int i = 0; i < allFiles; i ++)
            add_file(listFiles[i]);

        free(realPath);
        msg = set_status(SUCCESS);
    }
    else 
        msg = set_status(FILE_NOT_FOUND);

    canIndex = true;

    // unlock mutex
    pthread_mutex_unlock(&mtx);

    pthread_cond_signal(&indexCond);

    memset(sendToLog, '\0', strlen(sendToLog));
    snprintf(sendToLog, LENGTH, "DELETE, %s", filePath);
    write_log();

    free(filePath);
    return msg;
}

char *move_operation(char *token, char *savePtr)
{
    printf("Enter move_operation()!\n");
    char *msg, *inFilePath, *outFilePath;
    uint32_t bytesInFile, bytesOutFile;

    if (!check_four_parameters(token, savePtr, &bytesInFile, &inFilePath, &bytesOutFile, &outFilePath)){
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    if (!find_file(inFilePath)){
        msg = set_status(FILE_NOT_FOUND);
        free(inFilePath);
        free(outFilePath);
        return msg;
    }

    // upload second file path
    if (!send_upload_operation(bytesOutFile, inFilePath, outFilePath)){
        free(inFilePath);
        free(outFilePath);
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    // delete first file path
    msg = send_delete_operation(bytesInFile, inFilePath);

    memset(sendToLog, '\0', strlen(sendToLog));
    snprintf(sendToLog, LENGTH, "MOVE, %s, %s", inFilePath, outFilePath);
    write_log();
   
    free(inFilePath);
    free(outFilePath);
    return msg;
}

char *update_operation(char *token, char *savePtr)
{
    printf("Enter update_operation()!\n");
    char *msg, *filePath, *newContent, *realPath;
    uint32_t bytesPath, offset, bytesContent;

    // lock mutex
    pthread_mutex_lock(&mtx);

    if (!check_five_parameters(token, savePtr, &bytesPath, &offset, &bytesContent, &filePath, &newContent)){
        pthread_mutex_unlock(&mtx);
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    if (find_file(filePath)){
        realPath = add_root(filePath);

        FILE *f = fopen(realPath, "r+");

        fseek(f, offset, SEEK_SET);
        fprintf(f, "%s", newContent);
        

        fclose(f);
        free(realPath);
        msg = set_status(SUCCESS); 
    }
    else 
        msg = set_status(FILE_NOT_FOUND);

    canIndex = true;

    // unlock mutex
    pthread_mutex_unlock(&mtx);

    pthread_cond_signal(&indexCond);

    memset(sendToLog, '\0', strlen(sendToLog));
    snprintf(sendToLog, LENGTH, "UPDATE, %s", filePath);
    write_log();

    printf("Exit update_operation() with msg: '%s'!\n", msg);
    return msg;
}

char *search_operation(char *token, char *savePtr)
{
    printf("Enter search_operation!\n");
    char *msg, *str, *word;
    uint32_t msgSize = 20, bytesWord;

    // lock mutex
    pthread_mutex_lock(&mtx);

    if (!check_two_parameters(token, savePtr, &bytesWord, &word)){
        pthread_mutex_unlock(&mtx);
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    str = calloc(nrSearchFiles * LENGTH  + 1, sizeof(char));
    msg = calloc(nrSearchFiles * LENGTH + nrFiles + 1 + msgSize, sizeof(char));

    for (int i = 0; i < nrSearchFiles; i ++){
        for (int j = 0; j < FIRST_WORDS; j ++){
            if (check_length(word, searchFiles[i].freqWords[j].word)){
                //printf("Find '%s' in '%s' with path '%s'\n", word, searchFiles[i].freqWords[j].word, searchFiles[i].filename);
                memcpy(str + strlen(str), searchFiles[i].filename, strlen(searchFiles[i].filename));
                str[strlen(str)] = ' ';
            }
        }
    }

    //printf("str is: %s\n", str);
    snprintf(msg, (strlen(str) + msgSize), "0; %d; %s\n", strlen(str), str);
    //printf("msg is: %s\n", msg);

    // unlock mutex
    pthread_mutex_unlock(&mtx);
    
    memset(sendToLog, '\0', strlen(sendToLog));
    snprintf(sendToLog, LENGTH, "SEARCH, %s", word);
    write_log();

    free(word);
    free(str);
    return msg;
}

char *select_command(char *buff)
{
    printf("Enter select_command with buf: '%s'\n", buff);
    char *msg, *str, *token, *savePtr;
    uint32_t operation;
    bool isEnd = false;

    if (strncmp(buff, "\n", strlen(buff)) == 0){
        isEnd = true;
        operation = 99;
    }

    else{
        str = calloc((strlen(buff) + 1), sizeof(char));
        memcpy(str, buff, strlen(buff));

        token = strtok_r(str, " ;\n", &savePtr);
        operation = atoi(token);
        printf("Token is: '%s'\n", token);

        if (operation == 0 && strncmp(token,"0", strlen(token)) != 0)
            operation = 99;
    }

    switch (operation)
    {
        case 0:
        {
            msg = list_operation();
            break;
        }

        case 1:
        {
            msg = download_operation(token, savePtr);
            break;
        }

        case 2:
        {
            msg = upload_operation(token, savePtr);
            break;
        }

        case 4:
        {
            msg = delete_operation(token, savePtr);
            break;
        }

        case 8:
        {   msg = move_operation(token, savePtr);
            break;
        }

        case 10:
        {
            msg = update_operation(token, savePtr);
            break;
        }

        case 20:
        {
            msg = search_operation(token, savePtr);
            break;
        }

        case 99:
        {
            msg = set_status(UNKNOWN_OPERATION);
            break;
        }

        default:
        {
            msg = set_status(OTHER_ERROR);
            break;
        }
    }

    if (!isEnd)
        free(str);

    return msg;
}