#include "initialization.h"

bool add_file(char *filePath)
{
    // save the file
    memcpy(listFiles[nrFiles], filePath, strlen(filePath));
    FILE *f = fopen(ALL_FILES, "a");

    if (nrFiles != 0)
        fprintf(f, "\n");

    fprintf(f, "%s", listFiles[nrFiles]);
    nrFiles++; 
    fclose(f);
    return true;
}

char *add_root(char *filePath)
{
    printf("Enter add_root() with filePath: '%s'\n", filePath);
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
    printf("Enter make_dir(), with dirnName: '%s'\n", dirName);

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

bool check_dir(char *filePath)
{
    printf("Enter check_dir with filePath: '%s'\n", filePath);
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
    printf("Do not have dir!\n");
    if (!nrDir)
        return true;

    // check dir
    str = calloc((strlen(filePath) + 1), sizeof(char));
    memcpy(str, filePath, strlen(filePath));
    token = strtok_r(str, "/", &savePtr);

    printf("dir to check is: '%s'\n", token);

    realPath = add_root(token);

    make_dir(realPath);

    free(realPath);
    free(str);
    return true;
}

char *set_status(uint32_t status)
{
    char *msg = calloc(4, sizeof(char));

    snprintf(msg, 4, "%u\n", status);

    return msg;
}

char *list_operation()
{
    printf("Enter list_operation!\n");
    char *msg, *str;
    uint32_t msgSize = 20;

    str = calloc(nrFiles * LENGTH + nrFiles + 1, sizeof(char));
    msg = calloc(sizeof(str) + msgSize, sizeof(char));

    for (int i = 0; i < nrFiles; i ++){
        memcpy(str + strlen(str), listFiles[i], strlen(listFiles[i]));
        str[strlen(str)] = '\0';
    }
     
    snprintf(msg, (strlen(str) + msgSize), "0; %d; %s\n", strlen(str), str);
    printf("msg is: %s\n", msg);
    
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

char *upload_operation(char *token, char *savePtr)
{
    printf("Enter upload_operation!\n");
    char *msg, *filePath, *fileContent, *realPath;
    uint32_t bytesPath, bytesContent;
    bool fileExist = false;

    if (!check_upload(token, savePtr, &bytesPath, &filePath, &bytesContent, &fileContent)){
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    if (!check_dir(filePath)){
        msg = set_status(BAD_ARGUMENTS);
        return msg;
    }

    // create file descriptor
    realPath = add_root(filePath);

    printf("My breakpoint 1: '%s'!\n", realPath);

    FILE *f = fopen(realPath, "w");

    printf("My breakpoint 2!\n");

    // write the content
    fprintf(f, "%s", fileContent);
    fclose(f);

    // check if already exists
    for (int i = 0; i < nrFiles; i ++){
        if (strncmp(filePath, listFiles[i], strlen(filePath)) == 0){
            fileExist = true;
            break;
        }
    }

    if (!fileExist)
        add_file(filePath);

    // free memory
    free(realPath);
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