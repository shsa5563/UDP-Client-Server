/**********************************************************************
*** This is a util.h file used at both client and the server 
*** it has common data shared between client and server
*** Autor: Shekhar 
*** REferences: Websites like Stackoverflow etc..
**********************************************************************/
#ifndef UTIL_H
#define UTIL_H

#define MAXLIMIT  10*1024
#define PUT_COMMAND "put"
#define GET_COMMAND "get"
#define LIST_COMMAND "ls"
#define EXIT_COMMAND "exit" 
#define DELETE_COMMAND "delete"
#define OTHER_COMMAND "other"
#define COMMAN_ACK "CommandAck"
#define DELIMITER "*********************************************************\n"
#define DELIMITER_1 "--------------------------------\n"
#define DELIMITER_2 "\n\t"
#define FILE_NOT_EXISTS 55455
#define PASS_KEY 11
#define ERROR_COUNTER_LIMIT 20

//if debug is required #define DEBUG_ON shoudl be defined
#ifdef DEBUG_ON
#define DEBUG(x) x
#else
#define DEBUG(x) 
#endif

//the declaration of the common fucntions in util.c
int strToInteger(char *str);
int fileExist(const char * filename);
char* listFileInCurrentDirectory();
void encryptDecryptData(char * data, unsigned int len);

//the strct udpPacketInfo holds the value of the Packet INformations
typedef struct udpPacketInfo{
unsigned int packetInfoNotpacket;
char command[MAXLIMIT]; 
char fileName[MAXLIMIT];
unsigned int fileSize;
unsigned int frameNumber;
}udpPacketInfo;

//the strct udpPacket  is the packet by itself, it contains the data and the frame numver
typedef struct udpPacket{
unsigned int packetInfoNotpacket;
char bufferData[MAXLIMIT];
unsigned int frameNumber;
unsigned int readBytes; 
}udpPacket;

//the struct udpPacketAck hols thevalues of the ack; it has the previous and current ack information
typedef struct udpPacketAck{
unsigned int packetInfoNotpacket;
unsigned int frameNumber;
unsigned int previousFrameNumber;
}udpPacketAck;


#endif
