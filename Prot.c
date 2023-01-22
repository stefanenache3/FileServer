#include "Prot.h"

const char *commands[] = {"LIST", "GET",  "PUT", "DELETE",  "UPDATE",  "SEARCH",  "CONNECT"};

const char* EnumToStrConverter(CMD command)
{
  return commands[command];
}

CMD StrToCmdConverter(const char *command)
{
    
    for(int i=0;i<sizeof(commands)/sizeof(commands[0]);i++)
    {
        if(strcmp(commands[i],command)==0)
            return i;
    }
}

char *ErrorString(const char* errorMsg)
{
    char buf[256];
    memset(buf,0,sizeof(buf));
    appendToString(buf,"ERROR");
    appendToString(buf,errorMsg);

    return buf;
}
void appendToString(char* line,const char* app)
{
    if(strlen(line)==0)
        {
            strcpy(line,app);
            strcat(line,PROT_DELIMITER);
        }
        else 
        {
            strcat(line,app);
            strcat(line,PROT_DELIMITER);
        }
}
char *STR_ListFiles()
{
    char *line=(char*)malloc(64);    
    appendToString(line,"LIST");

    return line;
}

 char *STR_GetFiles(const char *fname)
{
    char* line=(char*)malloc(256);
    char len[16];

    sprintf(len,"%d",strlen(fname));
    appendToString(line,"GET");
    appendToString(line,len);
    appendToString(line,fname);

    return line;

}


 char *STR_Put(char *fname, int len)
{
    char* line=(char*)malloc(256);
    char nlen[16];
    char flen[256];
    memset(line,0,sizeof(line));
    sprintf(nlen,"%d",strlen(fname));
    sprintf(flen,"%d",len);
    appendToString(line,"PUT");
    appendToString(line,nlen);
    appendToString(line,fname);
    appendToString(line,flen);
    return line;
    
}
