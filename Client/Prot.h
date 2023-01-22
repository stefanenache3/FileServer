#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define PROT_DELIMITER "!"
#define EXIT_FAIL -1
typedef enum CMD
{
    LIST,
    GET,
    PUT,
    DELETE,
    UPDATE,
    SEARCH,
    CONNECT 
} CMD;




/***********/
const char *EnumToStrConverter(CMD command);
CMD StrToCmdConverter(const char *command);

/***********/

void appendToString(char* line,const char* app);

/***Functions that generate the request string for every command****/
char *STR_ListFiles();
char *STR_GetFiles(const char *fname);
char *STR_Put(const char *fname, int len);
char *STR_DeleteFile(char *fname);
char *STR_UpdateFile(char *fname, uint start, uint len, char *newChars);
char *STR_SearchWord(char *word);
/***                                                                ***/


