/**********************************************************************
*** This is a Reliable Server UDP program with get/put/exit/delete/other 
*** command options. With encrypted data exchange for file transfers
*** Autor: Shekhar 
*** REferences: Websites like Stackoverflow etc..
**********************************************************************/

#define _GNU_SOURCE
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
#include <sys/time.h>
#include <limits.h>
#include "util.h"

/***Declaration of the Functions **/
int putFilefromClient(int sockfd, struct sockaddr * cliaddr, socklen_t clilen, int realFileSize, char * fileName, int previousFrameNumber);
void listFileServer(int sockfd, struct sockaddr * cliaddr, socklen_t clilen,  int previousFrameNumber);
int getFileFromServer(int sockfd, struct sockaddr * cliaddr, socklen_t clilen, char * fileName, int previousFrameNumber, int fileSize);
void exitGracefully(int sockfd, struct sockaddr * cliaddr, socklen_t clilen,  int previousFrameNumber);
void deleteFile(int sockfd, struct sockaddr * cliaddr, socklen_t clilen,  int previousFrameNumber, char * fileName);
void otherCommand(int sockfd, struct sockaddr * cliaddr, socklen_t clilen,  int previousFrameNumber, char * fileName);

/**********************************************************************
*** getFileFromServer : function provides the file to the server (Reliable UDP)
*** filepath : provides the file name that the user needs from server 
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
int getFileFromServer(int sockfd, struct sockaddr * cliaddr, socklen_t clilen, char * fileName, int previousFrameNumber, int fileSize)
{
    unsigned int CommandDataYetToSend = 1;
    unsigned int CorrectAckNotReceived=1;
    unsigned int DataYetToSend = 1;
    unsigned int CommandDataYetToReceive=1;
    char buf[MAXLIMIT];
    int n;
    udpPacketAck putPacketAck;
    memset(&putPacketAck,'\0',sizeof(putPacketAck));

    FILE * fileOpen = NULL;
    fileOpen = fopen(fileName, "rb+");

    int sizeCheck = 0, readBytes = 0, sentBytes = 0;
    fd_set readfds;
    struct timeval tv;
    int retval;
    socklen_t len = clilen;
    CommandDataYetToSend = 1;
    CorrectAckNotReceived = 1;
    udpPacket putUdpPacket;
    unsigned int frameCounter=0;
    memset(&putUdpPacket, '\0', sizeof(putUdpPacket));
    putUdpPacket.packetInfoNotpacket = 0;
    int errorCounter =0, errorCounterFrame=0;
    DEBUG(printf("here 0 sizecheck %d fileSize %d\n",sizeCheck,fileSize));
    while (sizeCheck < fileSize) {
        DataYetToSend = 1;
        readBytes = fread(putUdpPacket.bufferData, 1, MAXLIMIT, fileOpen);
        encryptDecryptData(putUdpPacket.bufferData,readBytes);
        putUdpPacket.frameNumber = frameCounter;
        putUdpPacket.readBytes = readBytes;
        while(DataYetToSend)
        {   
            sentBytes = sendto(sockfd, &putUdpPacket, sizeof(putUdpPacket), 0, (struct sockaddr * ) cliaddr, len);
            DEBUG(printf("sent packet with fram number: %d\n", putUdpPacket.frameNumber));
            CorrectAckNotReceived =1;
            while(CorrectAckNotReceived)
            {
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);

                /* Wait up to 20ms from the server */

                tv.tv_sec = 0;
                tv.tv_usec = 50000;
                retval = select(sockfd+1, &readfds, NULL, NULL, &tv);

                if (retval == -1)
                    perror("select()");
                else if (retval && (FD_ISSET(sockfd, &readfds)))
                {
                    DEBUG(printf("Data is available now.\n"));
                    n = recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, (struct sockaddr * ) cliaddr, &len);
                    DEBUG(printf("received packet ack\n"));
                    if(!(n>0))
                    {
                        perror("recvfrom Command Info:");
                    }
                    DEBUG(printf("\nack from server %d, current frame of client : %d\n, filesize = %d,currentfileSize : %d\n", putPacketAck.frameNumber,putUdpPacket.frameNumber, fileSize, sizeCheck));
                    if(putPacketAck.frameNumber == putUdpPacket.frameNumber) // to handle the duplicate packets sent from client
                    {
                        frameCounter++;
                        DataYetToSend = 0;
                        CorrectAckNotReceived=0;
                        putPacketAck.previousFrameNumber = putPacketAck.frameNumber;
                        putPacketAck.frameNumber = putUdpPacket.frameNumber;//ntohl(packetInfo.frameNumber);

                        sizeCheck += readBytes;
                        DEBUG(printf("The file size in incremenets readbytes => %d sizecheck = %d \n",readBytes,sizeCheck));
                        fflush(stdout);
                    }
                    FD_CLR(sockfd, &readfds);
                }
                else
                {
                    DEBUG(printf("No data within 20ms.\n"));
                    if(errorCounterFrame == putUdpPacket.frameNumber)errorCounter ++;
                    else errorCounter =0;
                    if(errorCounter == ERROR_COUNTER_LIMIT)
                    {
                        printf("There is an issue with the network, Please try again after some time\n");
                        return -1;
                    }
                    errorCounterFrame = putUdpPacket.frameNumber;
                    break;
                }
            }

        }
    }

    if (fileSize == sizeCheck) {
        printf("Success in sending the file\n");
        fflush(stdout);
    } else {
        printf("Failed to send the file\n");
        fflush(stdout);
    }
    fflush(stdout);
    return 0;
}

/**********************************************************************
*** otherCommandToServer : function handles all the non-defined commands at the client and server
*** since it is presumed that the server might have a knowledge on such unknown commands
*** the command is sent to the server, the server relays the command if not found
*** bufferData :it is the unknown command 
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/

void otherCommand(int sockfd, struct sockaddr * cliaddr, socklen_t clilen,  int previousFrameNumber, char * fileName)
{
    char buf[MAXLIMIT];
    memset(buf, '\0', MAXLIMIT);
    sprintf(buf, "The given command:%s was not understood\n",fileName);
    if(!(sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr * ) cliaddr, clilen) >0))
    {
        perror("sendto OtherCommand:");
    }
    printf("othercommand\n");
}


/**********************************************************************
*** exitServer : Gracefully ends the server with a close() and fcloseall() followed by a exit(0)
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
void exitGracefully(int sockfd, struct sockaddr * cliaddr, socklen_t clilen,  int previousFrameNumber)
{
    char buf[MAXLIMIT];
    memset(buf, '\0', MAXLIMIT);
    strcpy(buf,"Closing the Socket and all other file descriptors in Server\nThe Server is no longer available\nPlease press Cntl+C to exit the Client or Restart the Server again\n");
    if(!(sendto(sockfd, buf, MAXLIMIT, 0, (struct sockaddr * ) cliaddr, clilen) >0))
    {
        perror("sendto Packet List command:");
    }
    if(fcloseall()!=0)
    {
        printf("Error in closing all the open files\n");
    }
    if(close(sockfd)!=0)
    {
        printf("Error in closing Socket\n");
    }
    exit(0);
}


/**********************************************************************
*** deleteFileAtServer : function deletes the particular file at the server
*** if the file is not found, it will return back sayin "Error Deleteing the file"
*** filepath : The file which the user wants to delete 
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
void deleteFile(int sockfd, struct sockaddr * cliaddr, socklen_t clilen,  int previousFrameNumber, char * fileName)
{
    char buf[MAXLIMIT];
    memset(buf, '\0', MAXLIMIT);
    if(remove(fileName)!=0)
    {
        sprintf(buf, "Error deleting the fileName %s\n", fileName);
    }
    else
    {
        sprintf(buf, "Successfully deleted the file name: %s in Server Local Directory\n", fileName);
    }
    if(!(sendto(sockfd, buf, MAXLIMIT, 0, (struct sockaddr * ) cliaddr, clilen) >0))
    {
        perror("sendto Packet List command :");
    }
}

/**********************************************************************
*** listFileAtServer : function provides the file/folder names in current directory of server  
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/


void listFileServer(int sockfd, struct sockaddr * cliaddr, socklen_t clilen,  int previousFrameNumber) {
    int CommandDataYetToSend = 1;
    int CorrectAckNotReceived = 1;
    udpPacket putUdpPacket;
    udpPacketAck putPacketAck;
    memset(&putUdpPacket, '\0', sizeof(putUdpPacket));
    memset(&putPacketAck, '\0', sizeof(putPacketAck));
    putUdpPacket.packetInfoNotpacket = 0;
    putUdpPacket.frameNumber = INT_MAX-20;
    putUdpPacket.packetInfoNotpacket = 0;
    strcpy(putUdpPacket.bufferData, listFileInCurrentDirectory());
    DEBUG(printf ("%s", listFileInCurrentDirectory()));
    fd_set readfds;
    struct timeval tv;
    int retval;
    unsigned int DataYetToSend = 1;
    unsigned int errorCounter =0, errorCounterFrame=0;
    while(DataYetToSend)
    {
        if(!(sendto(sockfd, &putUdpPacket, sizeof(putUdpPacket), 0, (struct sockaddr * ) cliaddr, clilen) >0))
        {
            perror("sendto Packet List command :");
        }
        CorrectAckNotReceived =1;
        DEBUG(printf("sent the packet\n"));
        while(CorrectAckNotReceived)
        {
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            /* Wait up to 20ms from the server */

            tv.tv_sec = 0;
            tv.tv_usec = 50000;
            retval = select(sockfd+1, &readfds, NULL, NULL, &tv);

            if (retval == -1)
                perror("select()");
            else if (retval && (FD_ISSET(sockfd, &readfds)))
            {
                DEBUG(printf("Data is available now.\n"));
                if(!(recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, NULL, NULL)>0))
                {
                    perror("recvfrom Command Info:");
                }
                if(putPacketAck.frameNumber == putUdpPacket.frameNumber) // to handle the duplicate packets sent from client
                {
                    DataYetToSend = 0;
                    CorrectAckNotReceived=0;
                    putPacketAck.previousFrameNumber = putPacketAck.frameNumber;
                    putPacketAck.frameNumber = putUdpPacket.frameNumber;//ntohl(packetInfo.frameNumber);

                }
                FD_CLR(sockfd, &readfds);
            }
            else
            {
                DEBUG(printf("No data within 20ms.\n"));
                if(errorCounterFrame == putUdpPacket.frameNumber)errorCounter ++;
                else errorCounter=0;
                if(errorCounter == ERROR_COUNTER_LIMIT)
                {
                    printf("There is an issue with the network, Please try again after some time\n");
                    return;
                }
                errorCounterFrame = putUdpPacket.frameNumber;
                break;
            }
        }

    }
}



/**********************************************************************
*** putFileToServer : function puts/uploads the respective file to the server(Reliable communication)
*** filepath : the file which would be uploaded to the server 
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
int putFilefromClient(int sockfd, struct sockaddr * cliaddr, socklen_t clilen, int realFileSize, char * fileName,int previousFrameNumber) {

    unsigned int CommandDataYetToSend = 1;
    unsigned int CorrectAckNotReceived=1;
    unsigned int DataYetToSend = 1;
    unsigned int CommandDataYetToReceive=1;

    int n, fd;
    socklen_t len;
    char buf[MAXLIMIT];

    len = clilen;
    FILE * fileOpen = NULL;
    fileOpen = fopen(fileName, "wb+");
    memset(fileName, '\0', MAXLIMIT);
    udpPacket putUdpPacket;
    udpPacketAck putUdpPacketAck;
    memset(&putUdpPacket, '\0', sizeof(putUdpPacket));
    memset(&putUdpPacketAck, '\0', sizeof(putUdpPacketAck));
    putUdpPacketAck.previousFrameNumber =  previousFrameNumber;
    putUdpPacketAck.frameNumber =  previousFrameNumber;
    int receivedBytes = 0, writtenBytes = 0;
    int sizeCheck = 0;
    printf("the filesize is :%d\n",realFileSize );
    while (sizeCheck < (realFileSize))
    {
        CorrectAckNotReceived = 1;
        DEBUG(printf("Data is available now.\n"));
        while(CorrectAckNotReceived)
        {
            receivedBytes = recvfrom(sockfd, &putUdpPacket, sizeof(putUdpPacket), 0, cliaddr, & len);
            DEBUG(printf("received  packet\n"));
            DEBUG(printf("\nprevious ack %d, current ack from client : %d\n, filesize = %d,currentfileSize : %d\n", putUdpPacketAck.previousFrameNumber,putUdpPacket.frameNumber, realFileSize, sizeCheck));
            if((!(putUdpPacketAck.previousFrameNumber == putUdpPacket.frameNumber))&&(!putUdpPacket.packetInfoNotpacket))
            {
                CorrectAckNotReceived=0;
                encryptDecryptData(putUdpPacket.bufferData, putUdpPacket.readBytes);
                writtenBytes = fwrite(putUdpPacket.bufferData, 1, putUdpPacket.readBytes, fileOpen);
                fflush(fileOpen);
                sizeCheck += writtenBytes;
                putUdpPacketAck.frameNumber = putUdpPacket.frameNumber;
                putUdpPacketAck.previousFrameNumber = putUdpPacketAck.frameNumber;
                DEBUG(printf("The file size in incremenets writebytes => %d received bytes = %d sizecheck = %d \n",writtenBytes, putUdpPacket.readBytes,sizeCheck));
            }
            sendto(sockfd, &putUdpPacketAck, sizeof(putUdpPacketAck), 0, cliaddr, len); //printf("sent packet ack\n");
        }
    }
    if (realFileSize == sizeCheck) {
        printf("Success in receving the file\n");
    } else {
        printf("Failed to receive the file\n");
        fclose(fileOpen);
        return -1;
    }
    fclose(fileOpen);
    fflush(stdout);
    return 0;
}


/**********************************************************************
*** The main program =: 
*** PLease give the IP address and the POrt number as argumenst to the program
*** example : ./client 127.0.0.1 50001
**********************************************************************/
int main(int argc, char ** argv) {
    int sockfd;
    char buf[MAXLIMIT];
    struct sockaddr_in servaddr, cliaddr;
    if(argc != 2)
    {
        printf("Please specify the Port\nExample: ./server 50001\n");
        exit(0);
    }
    unsigned int CommandDataYetToReceive=1;
    unsigned int fileExistsAtServer =0;
    char commandInput[MAXLIMIT];
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    bzero( & servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(strToInteger(argv[1]));

    bind(sockfd, (struct sockaddr * ) &servaddr, sizeof(servaddr));
    udpPacketInfo packetInfo;
    udpPacketAck  packetAck;
    int fileSize =0;
    while (1) {
        printf("%s\n@Waiting for the client\n",DELIMITER_1);
        fflush(stdout);
        fileExistsAtServer =0; //for the server to check if it has the file in Server Local Data Base
        memset(commandInput, 0, MAXLIMIT);
        memset(&packetInfo, '\0', sizeof(packetInfo));
        memset(&packetAck, '\0', sizeof(packetAck));
        memset(buf,'\0', MAXLIMIT);
        socklen_t socklen = sizeof(cliaddr);
        packetAck.previousFrameNumber = 1;
        packetAck.packetInfoNotpacket = 1;
        CommandDataYetToReceive=1;
        while(CommandDataYetToReceive)
        {
            int n = recvfrom(sockfd, &packetInfo, sizeof(packetInfo), 0, (struct sockaddr * ) &cliaddr, (socklen_t * ) &socklen);
            DEBUG(printf("received packetInfo\n"));
            DEBUG(printf("\nprevious ack %d, current ack from client : %d\n", packetAck.previousFrameNumber,packetInfo.frameNumber));
            if(!(n>0))
            {
                perror("recvfrom command packetInfo:");
            }
            if(!(packetAck.frameNumber == packetInfo.frameNumber)) // to handle the duplicate packets sent from client
            {
                CommandDataYetToReceive = 0;
                packetAck.previousFrameNumber = packetAck.frameNumber;
                packetAck.frameNumber = packetInfo.frameNumber;//ntohl(packetInfo.frameNumber);
                if (strcmp(packetInfo.command, GET_COMMAND) == 0) {
                    if(fileExist(packetInfo.fileName))
                    {
                        printf("file exists in server \n");
                        FILE * fileOpen = NULL;
                        fileOpen = fopen(packetInfo.fileName, "rb+");
                        fseek(fileOpen, 0, SEEK_END);
                        fileSize = ftell(fileOpen);
                        packetAck.previousFrameNumber =fileSize;// htonl(fileSize);
                        rewind(fileOpen);
                        fileExistsAtServer =1;
                        fclose(fileOpen);
                        printf("file size in server : %d", fileSize);
                    }
                    else
                    {
                        printf("file dose not exists in server \n");
                        packetAck.previousFrameNumber = FILE_NOT_EXISTS;
                        DEBUG(printf("the packet fram is : %d\n", packetAck.previousFrameNumber));
                    }
                }
            }
            if(!(sendto(sockfd, &packetAck, sizeof(packetAck), 0,  (struct sockaddr * ) &cliaddr, socklen)>0))
            {
                perror("sendto");
            }
            DEBUG(printf("sent packetInfo ack\n"));
        }
        if(CommandDataYetToReceive == 0)
        {
            CommandDataYetToReceive=1;
            if (strcmp(packetInfo.command, PUT_COMMAND) == 0) {

                sprintf(buf,"md5sum %s",packetInfo.fileName);
                int returnVal = putFilefromClient(sockfd, (struct sockaddr * ) & cliaddr, sizeof(cliaddr), ntohl(packetInfo.fileSize), packetInfo.fileName, packetAck.frameNumber);
                if(returnVal ==0)
                {
                    printf("The Md5sum of the file is:");fflush(stdout);
                    system(buf);
                }

            } else if (strcmp(packetInfo.command, GET_COMMAND) == 0) {
                if(fileExistsAtServer)
                {
                    fileExistsAtServer=0;
                    sprintf(buf,"md5sum %s",packetInfo.fileName);
                    if(getFileFromServer(sockfd,(struct sockaddr * ) & cliaddr, sizeof(cliaddr), packetInfo.fileName,packetAck.frameNumber,fileSize)==0)
                    {
                        printf("The Md5sum of the file is:");fflush(stdout);
                        system(buf);
                    }
                    fileSize =0;
                }
            } else if (strcmp(packetInfo.command, LIST_COMMAND) == 0) {
                printf("ls command\n");
                listFileServer(sockfd, (struct sockaddr * ) & cliaddr, sizeof(cliaddr), packetAck.frameNumber);
            } else if (strcmp(packetInfo.command, EXIT_COMMAND) == 0) {
                printf("exit command\n");
                exitGracefully(sockfd, (struct sockaddr * ) & cliaddr, sizeof(cliaddr), packetAck.frameNumber);
            } else if (strcmp(packetInfo.command, DELETE_COMMAND) == 0) {
                printf("delete command\n");
                deleteFile(sockfd, (struct sockaddr * ) & cliaddr, sizeof(cliaddr), packetAck.frameNumber, packetInfo.fileName);
            } else {
                printf("other command\n");
                otherCommand(sockfd, (struct sockaddr * ) & cliaddr, sizeof(cliaddr), packetAck.frameNumber, packetInfo.fileName);
            }
        }

    }

    return 0;
}
