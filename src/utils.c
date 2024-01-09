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

char *set_status(uint32_t status)
{
    char *msg = calloc(4, sizeof(char));

    snprintf(msg, 4, "%u\n", status);

    return msg;
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

bool check_path(char *token, char *savePtr, uint32_t *pBytesPath, char **pFilePath)
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

    filePath = calloc((strlen(token) + 1), sizeof(char));
    memcpy(filePath, token, strlen(token));

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