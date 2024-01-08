#include "initialization.h"

char *set_status(uint32_t status)
{
    char *msg = calloc(4, sizeof(char));

    snprintf(msg, 4, "%u\n", status);

    return msg;
}

char *list_operation()
{
    char *msg, *str;
    uint32_t bytesRead = 0, msgSize = 26;

    msg = calloc(nrFiles * LENGTH + nrFiles + msgSize, sizeof(char));
    bytesRead = msgSize;

    for (int i = 0; i < nrFiles; i ++){
        memcpy(msg + bytesRead, listFiles[i], strlen(listFiles[i]));
        bytesRead += strlen(listFiles[i]);
        msg[bytesRead] = '\0';
        bytesRead++;
    }

    bytesRead -= msgSize;

    str = calloc(msgSize, sizeof(char));
    snprintf(str, msgSize, "0; %u;", bytesRead);
    memcpy(msg, str, strlen(str));
    msg[strlen(msg)] = '\n';

    free(str);
    return msg;
}

bool check_upload(char *token, char *savePtr, uint32_t *pBytesPath, char **pFilePath, uint32_t *pBytesContent, char **pFileContent)
{
    printf("Enter check_upload!\n");
    char *filePath, *fileContent;
    uint32_t bytesPath, bytesContent;

    // check paramters and start with bytesPath

    printf("Token is: '%s'\n", token);
    token = strtok_r(NULL, ";\n", &savePtr);
    printf("Token is at bytesPath: '%s'\n", token);
    if (token == NULL)
        return false;
    
    bytesPath = atoi(token);
    if (bytesPath == 0)
        return false;

    // check filePath
    token = strtok_r(NULL, ";\n", &savePtr);
    printf("Token is at filePath: '%s'\n", token);
    if (token == NULL)
        return false;
    
    filePath = calloc((strlen(token) + 1), sizeof(char));
    memcpy(filePath, token, strlen(token));

    // check bytesContent
    token = strtok_r(NULL, ";\n", &savePtr);
    printf("Token is at bytesContent: '%s'\n", token);
    if (token == NULL){
        free(filePath);
        return false;
    }
    
    bytesContent = atoi(token);
    if (bytesContent == 0){
        free(filePath);
        return false;
    }

    // check fileContent
    token = strtok_r(NULL, ";\n", &savePtr);
    printf("Token is at fileContent: '%s'\n", token);
    if (token == NULL){
        free(filePath);
        return false;
    }

    fileContent = calloc((strlen(token) + 1), sizeof(char));
    memcpy(fileContent, token, strlen(token));

    // check if has more parameters than required
    token = strtok_r(NULL, ";\n", &savePtr);
    if (token != NULL){
        free(filePath);
        free(fileContent);
        return false;
    }

    (*pBytesPath) = bytesPath;
    (*pFilePath) = filePath;
    (*pBytesContent) = bytesContent;
    (*pFileContent) = fileContent;

    return true;
}

bool check_dir(char *filePath)
{
    printf("Enter check_dir with filePath: '%s'\n", filePath);
    char *str, *token, *savePtr;
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

    DIR *dir = opendir(token);

    if (dir == NULL){
        if (errno == ENOENT){
            int status = mkdir(token, 0777);

            if (status == -1)
                perror("mkdir");
        }

        else 
            perror("opendir");
    }

    closedir(dir);
    free(str);
    return true;
}

char *upload_operation(char *token, char *savePtr)
{
    printf("Enter upload_operation!\n");
    char *msg, *filePath, *fileContent;
    uint32_t bytesPath, bytesContent;

    if (!check_upload(token, savePtr, &bytesPath, &filePath, &bytesContent, &fileContent)){
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    if (!check_dir(filePath)){
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    // create file descriptor
    filePath++;
    FILE *f = fopen(filePath, "w");

    // write the content
    fprintf(f, "%s", fileContent);
    filePath--;
    fclose(f);

    // save the file
    memcpy(listFiles[nrFiles], filePath, strlen(filePath));
    f = fopen(ALL_FILES, "a");

    if (nrFiles != 0)
        fprintf(f, "\n");

    fprintf(f, "%s", listFiles[nrFiles]);
    nrFiles++;
    fclose(f);

    free(filePath);
    free(fileContent);
    msg = set_status(SUCCESS);
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
        operation = 10;
    }

    else{
        str = calloc((strlen(buff) + 1), sizeof(char));
        memcpy(str, buff, strlen(buff));

        token = strtok_r(str, ";\n", &savePtr);
        operation = atoi(token);
        printf("Token is: '%s'\n", token);

        if (operation == 0 && strncmp(token,"0", strlen(token)) != 0)
            operation = 10;
    }

    switch (operation)
    {
        case 0:
        {
            msg = list_operation();
            break;
        }

        case 2:
        {
            msg = upload_operation(token, savePtr);
            break;
        }
    
        case 10:
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