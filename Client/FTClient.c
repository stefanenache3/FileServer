#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "Prot.h"
#define BUFFLEN 2000
#define SERVER_PORT 1000
#define SERVER_NAME "0.0.0.0"

int sock_fd;
void listFiles(char *str)
{
    printf("Available files: \n");
    char delim[] = "!";
    char *token;
    token = strtok(str, delim);
    while (token)
    {
        printf("%s\n", token);
        token = strtok(NULL, delim);
    }
}
void checkResponse(char *resp, char *command)
{
    char delim[] = "!";
    char *token;
    token = strtok(resp, delim);
    if (strcmp(token, "ERROR") == 0)
    {
        printf("Error: ");
        token = strtok(NULL, delim);
        if (token == NULL)
            fprintf(stderr, "Invalid response format");
        printf("%s\n", token);
    }
    else if (strcmp(token, "SUCCESS") == 0)
    {

        CMD cmd = StrToCmdConverter(command);
        switch (cmd)
        {
        case LIST:
            listFiles(strtok(NULL, ""));
            break;

        default:
            break;
        }
    }
    else
    {
        fprintf(stderr, "Invalid response format");
    }
}
void putFile(char *fname)
{
    struct stat fileSt;
    int fd;
    int ret;
    fd = open(fname, O_RDONLY);
    if (fd == -1)
    {
        perror("Open file");
        return;
    }

    if (fstat(fd, &fileSt) < 0)
    {
        perror("fstat");
        return;
    }
    int len = fileSt.st_size;
    char buff[256];
    memset(buff, 0, sizeof(buff));
    strcpy(buff, STR_Put(fname, len));
    ret = send(sock_fd, buff, sizeof(buff), 0);
    if (ret < 0)
    {
        perror("send");
        return;
    }
    ret = sendfile(sock_fd, fd, NULL, len);
    if (ret == -1)
    {
        perror("send file");
        return;
    }
}
int FServerConnect()
{
    struct sockaddr_in serverAddr;

    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_NAME, &serverAddr.sin_addr) != 1)
    {
        perror("inet_pton");
        return -1;
    }

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        perror("socket");
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        perror("connect");
        return -1;
    }
    return sock_fd;
}

void processCommand(const char* input)
{
    char delim[]=" \n";
    char* p=NULL;
    p=strtok(input,delim);
    char* cmd;
  
  
    if(strcmp(p,"LIST")==0)
    {
        cmd=STR_ListFiles();
    }
    else if(strcmp(p,"GET")==0)
    {
        p=strtok(NULL,delim);
        cmd=STR_GetFiles(p);
    }
    else if(strcmp(p,"PUT")==0)
    {

    }
    else if(strcmp(p,"DELETE")==0)
    {

    }
    else if(strcmp(p,"UPDATE")==0)
    {

    }
    else if(strcmp(p,"SEARCH")==0)
    {

    }
    else
    {
        printf("INVALID COMMAND\n");
    }
    printf("COMMAND: %s\n",cmd);
    if(send(sock_fd,cmd,BUFFLEN,0)==-1)
    {
        perror("send");
    }
    free(cmd);

}
int main()
{

    printf("Type \"exit\" to end the program\n\n");
    char line[256];
    char resp[BUFFLEN];
    while (1)
    {
       sock_fd = FServerConnect();
        if (sock_fd == -1)
        {
            printf("Error!COULDN'T CONNECT TO SERVER, PLEASE TRY AGAIN LATER");
            return -1;
        }
        printf(">");
        memset(line,0,sizeof(line));
        memset(resp,0,sizeof(resp));
        fgets(line,sizeof(line),stdin);
        processCommand(line);
        if(recv(sock_fd,resp,sizeof(resp),0)==-1)
        {
            perror("recv");
        }
        printf("\n%s",resp);
        printf("\n");
        close(sock_fd);

    }

    /*  TEST: */

    /*      char buff[BUFFLEN];
          memset(buff,0,sizeof(buff));
          appendToString(buff,"LIST");
          send(sock_fd,buff,BUFFLEN,0);
          memset(buff,0,sizeof(buff));
          recv(sock_fd,buff,BUFFLEN,0);
          checkResponse(buff,"LIST");
      */
    // putFile("file.txt");
    while (1)
    {

        close(sock_fd);
    }
    return 0;
}