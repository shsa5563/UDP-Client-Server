/**********************************************************************
*** This is a util.c file used at both client and the server 
*** it has common functions shared between client and server
*** Autor: Shekhar 
*** REferences: Websites like Stackoverflow etc..
**********************************************************************/
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include "util.h"

/**********************************************************************
*** strToInteger function is to get to know if the str can be converted to an integer
*** if not it provides appropriate message before it exits the program 
*** str: the argumet is the string which is to be tested - if its an integer/not 
**********************************************************************/
int strToInteger(char * str) {
  char * endptr;
  errno = 0;
  long l = strtol(str, & endptr, 0);
  if (errno == ERANGE || * endptr != '\0' || str == endptr) {
    printf("Invalid Port Number\n");
    exit(-1);
  }
  // Only needed if sizeof(int) < sizeof(long)
  if (l < INT_MIN || l > INT_MAX) {
    printf("Invalid Port Number\n");
    exit(-1);
  }
  return (int) l;
}


/**********************************************************************
*** fileExist function  is to check if the file exists in the current directory or not
*** filename: is the filename which needs to be check if it exists in the cuurrent directory
**********************************************************************/
int fileExist(const char * filename) {
  DIR * dirp;
  struct dirent * dp;
  long len = strlen(filename);
  dirp = opendir(".");
  if ((dirp = opendir(".")) != NULL) {
    while ((dp = readdir(dirp)) != NULL) {
      if (strcmp(dp->d_name, filename) == 0) {
        (void) closedir(dirp);
        return 1;
      }
    }
    (void) closedir(dirp);
    printf("the file name :%s does not exist in current directory\n", filename);
  } else {
    printf("Can not open the current directory\n");
  }
  return 0;
}



/**********************************************************************
*** listFileInCurrentDirectory function  displays all the files in the current directory
*** dosenot take any argumenets
**********************************************************************/
char* listFileInCurrentDirectory(){
  DIR * dirp;
  struct dirent * dp;fflush(stdout);
  char *listFiles = (char*)malloc(MAXLIMIT);
  unsigned int NoFileExists =1;
  memset(listFiles,'\0',sizeof(listFiles));
  dirp = opendir(".");
  if ((dirp = opendir(".")) != NULL) {
    while ((dp = readdir(dirp)) != NULL) {
      //listFiles = (char *)realloc(listFiles,strlen(dp->d_name)+strlen(listFiles)+1);
      strcat(listFiles,dp->d_name);
      strcat(listFiles,"\n");
      NoFileExists =0;
    }
    (void) closedir(dirp);
  } else {
    strcat(listFiles,"Can not open the current directory\n");
  }
  if(NoFileExists)
  {
    strcat(listFiles,"No File Exists\n");
  }
  return listFiles;  
}



/**********************************************************************
*** encryptDecryptData function encrypts caeser cipher the data with a key (knwon to both server and the client )
*** data : is the data to be encrypted
*** len : is the length of the data which need to be encrpted
**********************************************************************/
void encryptDecryptData(char * data, unsigned int len)
{
    unsigned int i;
    for(i=0;i<len;i++)
    {
        data[i] = data[i] ^ PASS_KEY;
    }
}


