#include <pthread.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include "Prot.h"
#define BUFFLEN 2000
#define BACKLOG 15
#define NO_THREADS 15
#define MAX_FILES_NO 200
#define WORD_SIZE 64
#define MAX_WORDS 500
#define logFile "server.log"
pthread_mutex_t fList_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lst = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lstMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t logMutex=PTHREAD_MUTEX_INITIALIZER;
int noFiles = 0;

FILE *fptr;
typedef struct Data
{
    int socketFd;
    char fname[64];
    int flen;
    CMD command;
    int fIndex;
    /*update*/
    int start;
    int len;
    char *data;
    /******************/
    char sWord[64];
} Data;
typedef struct Word_
{
    char text[WORD_SIZE];
    int count;
} Word_;
typedef struct File
{
    char fname[256];
    pthread_mutex_t fmutex;
    pthread_mutex_t readersMutex;
    pthread_mutex_t searchMutex;
    pthread_cond_t rd;
    int readers;
    Word_ words[10];
    int wc; // numarul de cuvinte inregistrate

    // structura cuvinte
} File;

__thread Data thData;
File files[MAX_FILES_NO];
void cleanData(void *data)
{
    free(data);
}


void logAction(const Data logData)
{
    pthread_mutex_lock(&logMutex);
    FILE* fptr=fopen(logFile,"a");
    if(fptr==NULL)
    {
        perror("logFile");
        exit(-1);
    }

    time_t nTime=time(NULL);
    char buff[64];
    char line[100];
    strftime(buff,sizeof(buff),"%Y-%m-%d %H-%M",localtime(&nTime));
    strcpy(line,buff);
    strcat(line,"  ");
    strcat(line,EnumToStrConverter(logData.command));
    strcat(line,"  ");


    if(logData.command != LIST && logData.command !=SEARCH)
        strcat(line,logData.fname);

    strcat(line,"  ");
    
    if(logData.command==SEARCH)
        strcat(line,logData.sWord);

    strcat(line,"\n");
    fprintf(fptr,"%s",line);
    fclose(fptr);
    pthread_mutex_unlock(&logMutex);


}
int getFileIndex(char *fname)
{

    for (int i = 0; i < noFiles; i++)
        if (strcmp(files[i].fname, fname) == 0)
            return i;

    return -1;
}
int getAvailableFiles()
{
    DIR *dir;
    struct dirent *file;

    dir = opendir("files");
    if (dir == NULL)
    {
        fprintf(stderr, "Cant open the directory");
        return -1;
    }

    while ((file = readdir(dir)) != NULL)
    {
        if (file->d_name[0] != '.' && file->d_type == DT_REG)
        {
            strcpy(files[noFiles].fname, "files/");
            strcat(files[noFiles++].fname, file->d_name);
        }
    }

    return 0;
}

void handleGET()
{
    pthread_mutex_lock(&fList_mutex);
    int index = getFileIndex(thData.fname);
    if (thData.fIndex == -1)
    {
        // ERROR nu exista fisierul, trimite inapoi
    }
    pthread_mutex_unlock(&fList_mutex);
    thData.fIndex = index;

    pthread_mutex_lock(&(files[index].fmutex));
    pthread_mutex_lock(&(files[index].readersMutex));
    files[index].readers++;
    pthread_mutex_unlock(&(files[index].readersMutex));
    pthread_mutex_unlock(&(files[index].fmutex));

    int ret;
    struct stat fileSt;
    int fd = open(thData.fname, O_RDONLY);
    if (fd == -1)
    {
        perror("GET: open");
    }

    if (fstat(fd, &fileSt) < 0)
    {
        perror("Cant get file length");
        // ERROR
    }
    int totalBytes = 0;
    ret = sendfile(thData.socketFd, fd, NULL, fileSt.st_size);
    logAction(thData);
    if (ret < 0)
    {
        perror("sendfile");
        // ERROR
    }
    // FUNCTION

    pthread_mutex_lock(&(files[index].readersMutex));
    files[index].readers--;
    if (files[index].readers == 0)
        pthread_cond_signal(&(files[index].rd));
    pthread_mutex_unlock(&(files[index].readersMutex));
}
int handleUpdate()
{
    pthread_mutex_lock(&fList_mutex);
    int index = getFileIndex(thData.fname);
    if (thData.fIndex == -1)
    {
        // ERROR nu exista fisierul, trimite inapoi
    }
    pthread_mutex_unlock(&fList_mutex);
    thData.fIndex = index;
    pthread_mutex_lock(&(files[index].fmutex));
    pthread_mutex_lock(&(files[index].readersMutex));
    while (files[index].readers > 0)
        pthread_cond_wait(&(files[index].rd), &(files[index].readersMutex));
    pthread_mutex_unlock(&(files[index].readersMutex));
    int fd = open(thData.fname, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        // TRANSMITE ERROARE
    }
    if (lseek(fd, thData.start, SEEK_SET) == -1) // ECHEC ERROR
    {
        perror("seek");
        close(fd);
        // ERROR
    }

    if (write(fd, thData.data, thData.len) < 0)
    {
        perror("write");
        close(fd);
    }
    logAction(thData);
    close(fd);
    pthread_mutex_lock(&lstMutex);
    pthread_cond_signal(&lst);
    pthread_mutex_unlock(&lstMutex);

    pthread_mutex_unlock(&(files[index].fmutex));
}
int handlePUT()
{
    int ret;
    int fd;
    fd = open(thData.fname, O_CREAT | O_WRONLY, 0666);
    if (fd == -1)
    {
        perror("file creation");
        return -1;
    }
    char buff[BUFFLEN];
    int totalBytes = 0;
    while (totalBytes < thData.flen)
    {
        ret = recv(thData.socketFd, buff + totalBytes, thData.flen - totalBytes, 0);
        if (ret < 0)
        {
            perror("Recv");
            return -1;
        }
        totalBytes += ret;
    }
    write(fd, buff, ret);
}
int handleSearch()
{
    char response[BUFFLEN];
    memset(response, 0, BUFFLEN);
    appendToString(response, "SUCCESS");
    int nMatches = 0;
    pthread_mutex_lock(&fList_mutex);
    for (int i = 0; i < noFiles; i++)
    {
        for (int j = 0; j < files[i].wc; j++)
        {
            if (strcmp(thData.sWord, files[i].words[j].text) == 0)
            {
                nMatches++;
                appendToString(response, files[i].fname);
            }
        }
    }
    if (nMatches == 0)
        appendToString(response, "No matches");
    pthread_mutex_unlock(&fList_mutex);
    logAction(thData);
    send(thData.socketFd, response, BUFFLEN, 0);
}
int countCMP(const void *a, const void *b)
{
    return ((Word_ *)b)->count - ((Word_ *)a)->count;
}
void *FilesList_Thread(void *param)
{
    Word_ words[MAX_WORDS];
    memset(words, 0, MAX_WORDS);
    char aux[WORD_SIZE];
    int nwords = 0;
    pthread_mutex_lock(&lstMutex);
    while (1)
    {
        pthread_cond_wait(&lst, &lstMutex); // lst mutex lock/unlock
        pthread_mutex_lock(&fList_mutex);
        fprintf(stderr, "STARTTT: %d", noFiles);

        for (int index = 0; index < noFiles; index++)
        {

            /*FILE READER LOGIC*/
            pthread_mutex_lock(&(files[index].fmutex));
            pthread_mutex_lock(&(files[index].readersMutex));
            files[index].readers++;
            pthread_mutex_unlock(&(files[index].readersMutex));
            pthread_mutex_unlock(&(files[index].fmutex));
            /*                                                */
            FILE *fptr = fopen(files[index].fname, "r");
            /*Fisierul nu mai exista, trebuie stearsa intrarea */
            if (!fptr)
            {
                continue;
            }
            while (fscanf(fptr, "%s", aux) != EOF)
            {

                int i;
                for (i = 0; i < nwords; i++)
                {
                    if (strcmp(words[i].text, aux) == 0)
                    {
                        words[i].count++;
                        break;
                    }
                }

                if (i == nwords)
                {
                    strcpy(words[i].text, aux);
                    words[i].count = 1;
                    nwords++;
                }
            }

            /*FILE READER LOGIC*/
            pthread_mutex_lock(&(files[index].readersMutex));
            files[index].readers--;
            if (files[index].readers == 0)
            {
                pthread_cond_signal(&(files[index].rd));
            }
            pthread_mutex_unlock(&(files[index].readersMutex));
            /*                                                */

            qsort(words, nwords, sizeof(Word_), countCMP);
            pthread_mutex_lock(&(files[index].searchMutex));

            if (nwords >= 10)
            {
                files[index].wc = 10;
            }
            else
                files[index].wc = nwords;
            for (int j = 0; j < 10; j++)
            {
                if (j > nwords - 1)
                    break;
                strcpy(files[index].words[j].text, words[j].text);
                files[index].words[j].count = words[j].count;
            }
            pthread_mutex_unlock(&(files[index].searchMutex));
        }
        pthread_mutex_unlock(&fList_mutex);
    }
    pthread_mutex_unlock(&lstMutex);
}

void handleDelete()
{
    char response[64];
    memset(response, 0, sizeof(response));
    pthread_mutex_lock(&fList_mutex);
    int index = getFileIndex(thData.fname);
    if (index < 0)
    {
        // ERROR THE FILE DOESNT EXIST
    }

    pthread_mutex_lock(&(files[index].fmutex));
    while (files[index].readers > 0)
        pthread_cond_wait(&(files[index].rd), &(files[index].readersMutex));

    unlink(thData.fname);
    logAction(thData);
    noFiles--;
    appendToString(response, "SUCCESS");
    send(thData.socketFd,response,sizeof(response),0);
    pthread_mutex_lock(&lstMutex);
    pthread_cond_signal(&lst);
    pthread_mutex_unlock(&lstMutex);

    pthread_mutex_unlock(&(files[index].fmutex));

    pthread_mutex_unlock(&fList_mutex);
}
void *handleClientConn_(void *params)
{

    thData.socketFd = *(int *)params;

    int ret;
    char buff[BUFFLEN];
    fprintf(stderr, "DA");
    ret = recv(thData.socketFd, buff, BUFFLEN, 0);
    printf("RECIEVED: %s\n",buff);
    char delim[] = "!";
    char *token;
    char *state;

    token = strtok_r(buff, delim, &state);
    // CMD cmd=StrToCmdConverter(token);

    char response[BUFFLEN];
    memset(response, 0, sizeof(response));
    CMD cmd = StrToCmdConverter((const char *)token);
    thData.command = cmd;
    fprintf(stderr, "cmd :%d", cmd);
    switch (cmd)
    {
    case LIST:
        appendToString(response, "SUCCESS");
        pthread_mutex_lock(&fList_mutex);
        for (int i = 0; i < noFiles; i++)
        {
            appendToString(response, files[i].fname);
        }
        fprintf(stderr, "DA");
        pthread_mutex_unlock(&fList_mutex);
        fprintf(stderr, "DA");
        send(thData.socketFd, response, BUFFLEN, 0);
        break;
    case GET:
        token = strtok_r(NULL, delim, &state);
        token = strtok_r(NULL, delim, &state);
        strcpy(thData.fname, "files/");
        strcat(thData.fname, token);
        handleGET();
        close(thData.socketFd);
        break;
    case PUT:
        token = strtok_r(NULL, delim, &state);
        token = strtok_r(NULL, delim, &state);
        strcpy(thData.fname, "files/");
        strcat(thData.fname, token);
        token = strtok_r(NULL, delim, &state);
        thData.flen = atoi(token);
        handlePUT();
        close(thData.socketFd);
        break;
    case DELETE:
        token = strtok_r(NULL, delim, &state);
        token = strtok_r(NULL, delim, &state);
        strcpy(thData.fname, "files/");
        strcat(thData.fname, token);
        handleDelete();
        close(thData.socketFd);
        break;
    case UPDATE:
        token = strtok_r(NULL, delim, &state);
        token = strtok_r(NULL, delim, &state);
        strcpy(thData.fname, "files/");
        strcat(thData.fname, token);
        token = strtok_r(NULL, delim, &state);
        thData.start = atoi(token);
        token = strtok_r(NULL, delim, &state);
        thData.len = atoi(token);
        token = strtok_r(NULL, delim, &state);
        thData.data = token;
        handleUpdate();
        close(thData.socketFd);
        break;
    case SEARCH:
        token = strtok_r(NULL, delim, &state);
        token = strtok_r(NULL, delim, &state);
        strcpy(thData.sWord, token);
        handleSearch();
        close(thData.socketFd);
        break;
    case CONNECT:
        break;
    }
}

int main(int argc, char *argv[])
{
    int port;
    int serverSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr;

    if (argc < 2)
    {
        fprintf(stderr, "Invalid number of arguments: provide port number\n");
        return -1;
    }

    port = atoi(argv[1]);
    if (port == 0)
    {
        fprintf(stderr, "Invalid port, please enter a valid number\n");
        return -1;
    }

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == -1)
    {
        perror("Server sock creation");
        return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    int yes = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("Binding server socket");
        return -1;
    }

    if (listen(serverSock, BACKLOG) == -1)
    {
        perror("Listening on server");
        return -1;
    }

    if (getAvailableFiles() == -1)
        return -1;

    pthread_t tids[NO_THREADS];
    int t_index = 0;
    int ret;

    pthread_create(&tids[t_index], NULL, FilesList_Thread, NULL);
    t_index++;
    sleep(1);       //este importanta secventierea, ne asiguram ca nu se pierde semnalul
    pthread_mutex_lock(&lstMutex);
    pthread_cond_signal(&lst);
    pthread_mutex_unlock(&lstMutex);
    while (1)
    {
        socklen_t clientLength = sizeof(clientAddr);
        clientSock = accept(serverSock, (struct sockaddr *)&clientAddr, &clientLength);

        fprintf(stderr, "DA");
        ret = pthread_create(&tids[t_index], NULL, handleClientConn_, (void *)&clientSock);
        if (ret)
        {
            perror("pthread create");
            return -1;
        }

        if (pthread_join(tids[t_index], NULL))
        {
            perror("pthread join");
            return -1;
        }

        t_index++;
    }
}