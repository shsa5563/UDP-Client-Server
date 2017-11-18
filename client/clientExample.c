/**********************************************************************
*** This is a Reliable Client UDP program with get/put/exit/delete/other 
*** command options. With encrypted data exchange for file transfers
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
#include <limits.h>
#include <sys/time.h>
#include "util.h"

/***Declaration of the Functions **/
int putFileToServer(const char * filepath, struct sockaddr_in * servaddr);
int getFileFromServer(const char * filepath, struct sockaddr_in * servaddr);
int listFileAtServer(struct sockaddr_in * servaddr);
int deleteFileAtServer(const char * filepath, struct sockaddr_in * servaddr);
int exitServer(struct sockaddr_in * servaddr);
int otherCommandToServer(const char * bufferData, struct sockaddr_in *servaddr);

/**********************************************************************
*** getFileFromServer : function gets the file form the server (Reliable UDP)
*** filepath : provides the file name that the user needs from server 
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
int getFileFromServer(const char * filepath, struct sockaddr_in * servaddr)
{

    unsigned int CommandDataYetToSend = 1;
    unsigned int CorrectAckNotReceived=1;
    unsigned int DataYetToSend = 1;
    unsigned int CommandDataYetToReceive=1;
    int sockfd, n, fd;
    char buf[MAXLIMIT];
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    udpPacketInfo putPacketInfo;
    udpPacketAck putPacketAck;
    memset(&putPacketInfo,'\0',sizeof(putPacketInfo));
    memset(&putPacketAck,'\0',sizeof(putPacketAck));
    putPacketAck.previousFrameNumber = INT_MAX-37;
    putPacketAck.frameNumber = INT_MAX-37;

    int sizeCheck = 0, readBytes = 0, sentBytes = 0;

    strcpy(putPacketInfo.command,GET_COMMAND);
    strcpy(putPacketInfo.fileName,filepath);
    putPacketInfo.frameNumber = INT_MAX-33;//htonl(INT_MAX);
    socklen_t socklength = sizeof(* servaddr);
    putPacketInfo.packetInfoNotpacket = 1;

    unsigned int realFileSize = 0;

    fd_set readfds;
    struct timeval tv;
    int retval;
    unsigned int errorCounter=0, errorCounterFrame =0;

/**********************************************************************
*** The next chunk of code would be the handshake of information before the actual data exchange
**********************************************************************/
    while(CommandDataYetToSend)
    {
        sendto(sockfd, &putPacketInfo, sizeof(putPacketInfo), 0, (struct sockaddr * ) servaddr, socklength);
        DEBUG(printf("sent packetInfo with frame num : %d\n",putPacketInfo.frameNumber));
        CorrectAckNotReceived=1;
        while(CorrectAckNotReceived)
        {
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            /* Wait up to 35ms from the server */

            tv.tv_sec = 0;
            tv.tv_usec = 35000;
            retval = select(sockfd+1, &readfds, NULL, NULL, &tv);

            if (retval == -1)
                perror("select()");
            else if (retval && (FD_ISSET(sockfd, &readfds)))
            {
                DEBUG(printf("Data is available now.\n"));
                n = recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, NULL, NULL);
                DEBUG(printf("received  packetInfo ack\n"));
                DEBUG(printf("\nack from server %d, current frame of client : %d, FileSize = %d\n", putPacketAck.frameNumber,putPacketInfo.frameNumber,  putPacketAck.previousFrameNumber));
                realFileSize = putPacketAck.previousFrameNumber;
                if(!(n>0))
                {
                    perror("recvfrom Command Info:");
                }

                if(putPacketAck.frameNumber == putPacketInfo.frameNumber) // to handle the duplicate packets sent from client
                {
                    CommandDataYetToSend = 0;
                    CorrectAckNotReceived=0;
                    putPacketAck.previousFrameNumber = putPacketAck.frameNumber;
                    putPacketAck.frameNumber = putPacketInfo.frameNumber;//ntohl(packetInfo.frameNumber);
                }
                FD_CLR(sockfd, &readfds);
            }
            else
            {
                DEBUG(printf("No data within 35ms in command.\n"));

                if(errorCounterFrame == putPacketInfo.frameNumber)errorCounter ++;
                else errorCounter =0;
                if(errorCounter == ERROR_COUNTER_LIMIT)
                {
                    printf("There is an issue with the network, Please try again after some time\n");
                    return -1;
                }
                errorCounterFrame = putPacketInfo.frameNumber;
                break;
            }
        }

    }
    if(realFileSize==FILE_NOT_EXISTS)
    {
        printf("The file %s does not exist in Server, Could not fetch the file\n", filepath);
        close(sockfd);
        return -1;
    }
    else
    {
        FILE * fileOpen = NULL;
        fileOpen = fopen(filepath, "wb+");

        udpPacket putUdpPacket;
        memset(&putUdpPacket, '\0', sizeof(putUdpPacket));
        int receivedBytes = 0, writtenBytes = 0;
        int sizeCheck = 0;
        printf("the filesize is :%d\n",realFileSize );


/**********************************************************************
*** The next chunk of code would be the actual data exchange for get command
**********************************************************************/

        while (sizeCheck < (realFileSize))
        {
            CorrectAckNotReceived = 1;
            DEBUG(printf("Data is available now.\n"));
            while(CorrectAckNotReceived)
            {
                receivedBytes = recvfrom(sockfd, &putUdpPacket, sizeof(putUdpPacket), 0, NULL, NULL);
                DEBUG(printf("received  packet\n"));
                DEBUG(printf("\nprevious ack %d, current ack from client : %d\n, filesize = %d,currentfileSize : %d\n", putPacketAck.previousFrameNumber,putUdpPacket.frameNumber, realFileSize, sizeCheck));
                if((!(putPacketAck.frameNumber == putUdpPacket.frameNumber))&&(!putUdpPacket.packetInfoNotpacket))
                {
                    CorrectAckNotReceived=0;
                    encryptDecryptData(putUdpPacket.bufferData,putUdpPacket.readBytes); // decrypt data and write 
                    writtenBytes = fwrite(putUdpPacket.bufferData, 1, putUdpPacket.readBytes, fileOpen);
                    fflush(fileOpen);
                    sizeCheck += writtenBytes;
                    putPacketAck.frameNumber = putUdpPacket.frameNumber;
                    putPacketAck.previousFrameNumber = putPacketAck.frameNumber;
                    DEBUG(printf("The file size in incremenets writebytes => %d received bytes = %d sizecheck = %d \n",writtenBytes, putUdpPacket.readBytes,sizeCheck));
                }
                sendto(sockfd, &putPacketAck, sizeof(putPacketAck), 0, (struct sockaddr * ) servaddr, socklength);
                DEBUG(printf("sent packet ack\n"));
            }
        }
        if (realFileSize == sizeCheck) {
            printf("Success uploading the file\n");
        } else {
            printf("Failed to upload the file\n");
        }
        fclose(fileOpen);
        close(sockfd);
    }
    return 0 ;
}

/**********************************************************************
*** listFileAtServer : function provides the file/folder names in current directory of server  
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
int listFileAtServer(struct sockaddr_in * servaddr)
{
    unsigned int CommandDataYetToSend = 1;
    unsigned int CorrectAckNotReceived=1;
    unsigned int DataYetToSend = 1;
    unsigned int CommandDataYetToReceive=1;
    int sockfd, n, fd;
    char buf[MAXLIMIT];
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    udpPacketInfo putPacketInfo;
    udpPacketAck putPacketAck;
    memset(&putPacketInfo,'\0',sizeof(putPacketInfo));
    memset(&putPacketAck,'\0',sizeof(putPacketAck));
    putPacketAck.previousFrameNumber = INT_MAX-10;
    putPacketAck.frameNumber = INT_MAX-10;

    int sizeCheck = 0, readBytes = 0, sentBytes = 0;

    strcpy(putPacketInfo.command,LIST_COMMAND);
    putPacketInfo.frameNumber = INT_MAX-15;//htonl(INT_MAX);
    socklen_t socklength = sizeof(* servaddr);
    putPacketInfo.packetInfoNotpacket = 1;

    fd_set readfds;
    struct timeval tv;
    int retval;
    unsigned int errorCounter = 0, errorCounterFrame=0;


/**********************************************************************
*** The next chunk of code would be the handshake of information before the actual data exchange
**********************************************************************/

    while(CommandDataYetToSend)
    {
        sendto(sockfd, &putPacketInfo, sizeof(putPacketInfo), 0, (struct sockaddr * ) servaddr, socklength);
        DEBUG(printf("sent packetInfo with frame num : %d\n",putPacketInfo.frameNumber));
        CorrectAckNotReceived=1;
        while(CorrectAckNotReceived)
        {
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            /* Wait up to 35ms from the server */

            tv.tv_sec = 0;
            tv.tv_usec = 35000;
            retval = select(sockfd+1, &readfds, NULL, NULL, &tv);

            if (retval == -1)
                perror("select()");
            else if (retval && (FD_ISSET(sockfd, &readfds)))
            {
                DEBUG(printf("Data is available now.\n"));
                n = recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, NULL, NULL);
                DEBUG(printf("received  packetInfo ack\n"));
                DEBUG(printf("\nack from server %d, current frame of client : %d\n", putPacketAck.frameNumber,putPacketInfo.frameNumber));
                if(!(n>0))
                {
                    perror("recvfrom Command Info:");
                }

                if(putPacketAck.frameNumber == putPacketInfo.frameNumber) // to handle the duplicate packets sent from client
                {
                    CommandDataYetToSend = 0;
                    CorrectAckNotReceived=0;
                    putPacketAck.previousFrameNumber = putPacketAck.frameNumber;
                    putPacketAck.frameNumber = putPacketInfo.frameNumber;//ntohl(packetInfo.frameNumber);
                }
                FD_CLR(sockfd, &readfds);
            }
            else
            {
                DEBUG(printf("No data within 35ms in command.\n"));

                if(errorCounterFrame == putPacketInfo.frameNumber)errorCounter ++;
                else errorCounter =0;
                if(errorCounter == ERROR_COUNTER_LIMIT)
                {
                    printf("There is an issue with the network, Please try again after some time\n");
                    return -1;
                }
                errorCounterFrame = putPacketInfo.frameNumber;
                break;
            }
        }

    }

/**********************************************************************
*** The next chunk of code would get the ls command results from the server
**********************************************************************/
    udpPacket putUdpPacket;
    memset(&putUdpPacket, '\0', sizeof(putUdpPacket));
    CorrectAckNotReceived = 1;
    DEBUG(printf("Data is available now.\n"));
    while(CorrectAckNotReceived)
    {
        if(!(recvfrom(sockfd, &putUdpPacket, sizeof(putUdpPacket), 0, NULL, NULL)>0))
        {
            perror("recvfrom Packet List Cmd:");
        }
        if((!(putPacketAck.previousFrameNumber == putUdpPacket.frameNumber))&&(!putUdpPacket.packetInfoNotpacket))
        {
            CorrectAckNotReceived=0;
            putPacketAck.frameNumber = putUdpPacket.frameNumber;
            putPacketAck.previousFrameNumber = putPacketAck.frameNumber;
            printf("%sls command at Server fetched :\n%s\n", DELIMITER,putUdpPacket.bufferData);
        }
        sendto(sockfd, &putPacketAck, sizeof(putPacketAck), 0,  (struct sockaddr * ) servaddr, socklength);
        DEBUG(printf("sent packet ack\n"));
    }

    close(sockfd);
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

int otherCommandToServer(const char * bufferData, struct sockaddr_in *servaddr)
{
    char buf[MAXLIMIT];
    int sockfd, n, fd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    udpPacketInfo putPacketInfo;
    udpPacketAck putPacketAck;
    memset(buf,'\0',MAXLIMIT);
    memset(&putPacketInfo, '\0',sizeof(putPacketInfo));
    memset(&putPacketAck, '\0',sizeof(putPacketAck));
    putPacketInfo.frameNumber = INT_MAX-45;
    strcpy(putPacketInfo.command,OTHER_COMMAND);
    strcpy(putPacketInfo.fileName,bufferData);
    socklen_t socklength = sizeof(* servaddr);

    if(!(sendto(sockfd, &putPacketInfo, sizeof(putPacketInfo), 0, (struct sockaddr * ) servaddr, socklength)))
    {
        perror("sendto OtherCommand:");
    }
    if(!(recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, NULL, NULL)))
    {
        perror("recvfrom OtherCommand Ack:");
    }
    if(!(recvfrom(sockfd, buf, MAXLIMIT, 0, NULL, NULL)))
    {
        perror("recvfrom OtherCommand bufferData:");
    }
    close(sockfd);
    printf("%s", buf);
    return 0;
}


/**********************************************************************
*** deleteFileAtServer : function deletes the particular file at the server
*** if the file is not found, it will return back sayin "Error Deleteing the file"
*** filepath : The file which the user wants to delete 
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
int deleteFileAtServer(const char * filepath, struct sockaddr_in * servaddr)
{
    char buf[MAXLIMIT];
    int sockfd, n, fd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    udpPacketInfo putPacketInfo;
    udpPacketAck putPacketAck;
    memset(buf,'\0',MAXLIMIT);
    memset(&putPacketInfo, '\0',sizeof(putPacketInfo));
    memset(&putPacketAck, '\0',sizeof(putPacketAck));
    putPacketInfo.frameNumber = INT_MAX-25;
    strcpy(putPacketInfo.command,DELETE_COMMAND);
    strcpy(putPacketInfo.fileName,filepath);
    socklen_t socklength = sizeof(* servaddr);

    if(!(sendto(sockfd, &putPacketInfo, sizeof(putPacketInfo), 0, (struct sockaddr * ) servaddr, socklength)))
    {
        perror("sendto DeleteFile:");
    }
    if(!(recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, NULL, NULL)))
    {
        perror("recvfrom exitServer Ack:");
    }
    if(!(recvfrom(sockfd, buf, MAXLIMIT, 0, NULL, NULL)))
    {
        perror("recvfrom exitServer bufferData:");
    }
    close(sockfd);
    printf("%s", buf);
    return 0;
}



/**********************************************************************
*** exitServer : Gracefully ends the server with a close() and fcloseall() followed by a exit(0)
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
int exitServer(struct sockaddr_in * servaddr)
{
    char buf[MAXLIMIT];
    int sockfd, n, fd;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    udpPacketInfo putPacketInfo;
    udpPacketAck putPacketAck;
    memset(buf,'\0',MAXLIMIT);
    memset(&putPacketInfo, '\0',sizeof(putPacketInfo));
    memset(&putPacketAck, '\0',sizeof(putPacketAck));
    putPacketInfo.frameNumber = INT_MAX-30;
    strcpy(putPacketInfo.command,EXIT_COMMAND);
    socklen_t socklength = sizeof(* servaddr);
    if(!(sendto(sockfd, &putPacketInfo, sizeof(putPacketInfo), 0, (struct sockaddr * ) servaddr, socklength)))
    {
        perror("sendto exitServer:");
    }
    if(!(recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, NULL, NULL)))
    {
        perror("recvfrom exitServer Ack:");
    }
    if(!(recvfrom(sockfd, buf, MAXLIMIT, 0, NULL, NULL)))
    {
        perror("recvfrom exitServer bufferData:");
    }
    close(sockfd);
    printf("%s",buf);
    return 0;
}


/**********************************************************************
*** putFileToServer : function puts/uploads the respective file to the server(Reliable communication)
*** filepath : the file which would be uploaded to the server 
*** servaddr  : provides the UDP socket info of the server to which info is to be sent
**********************************************************************/
int putFileToServer(const char * filepath, struct sockaddr_in * servaddr) {
    unsigned int CommandDataYetToSend = 1;
    unsigned int CorrectAckNotReceived=1;
    unsigned int DataYetToSend = 1;
    unsigned int CommandDataYetToReceive=1;
    int sockfd, n, fd;
    char buf[MAXLIMIT];
    const char * target, * path;
    unsigned int frameCounter = 0;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    path = filepath;
    target = filepath;
    udpPacketInfo putPacketInfo;
    udpPacketAck putPacketAck;
    memset(&putPacketInfo,'\0',sizeof(putPacketInfo));
    memset(&putPacketAck,'\0',sizeof(putPacketAck));
    putPacketAck.previousFrameNumber = INT_MAX-10;
    putPacketAck.frameNumber = INT_MAX-10;

    FILE * fileOpen = NULL;
    fileOpen = fopen(path, "rb+");

    fseek(fileOpen, 0, SEEK_END);
    int fileSize = ftell(fileOpen);
    putPacketInfo.fileSize = htonl(fileSize);
    rewind(fileOpen);
    int sizeCheck = 0, readBytes = 0, sentBytes = 0;

    printf("the filesize is :%d\n",fileSize );
    strcpy(putPacketInfo.command,PUT_COMMAND);
    strcpy(putPacketInfo.fileName,filepath);
    putPacketInfo.frameNumber = INT_MAX;//htonl(INT_MAX);
    socklen_t socklength = sizeof(* servaddr);
    putPacketInfo.packetInfoNotpacket = 1;

    DEBUG(printf("the clinet port: %d, the client addr %ld", servaddr->sin_port, servaddr->sin_addr.s_addr ));
    fd_set readfds;
    struct timeval tv;
    int retval;
    unsigned int errorCounter =0, errorCounterFrame=0;

/**********************************************************************
*** The next chunk of code is the handshake of information between server and client
**********************************************************************/
    while(CommandDataYetToSend)
    {
        sendto(sockfd, &putPacketInfo, sizeof(putPacketInfo), 0, (struct sockaddr * ) servaddr, socklength);
        DEBUG(printf("sent packetInfo with frame num : %d\n",putPacketInfo.frameNumber));
        CorrectAckNotReceived=1;
        while(CorrectAckNotReceived)
        {
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            /* Wait up to 35ms from the server */

            tv.tv_sec = 0;
            tv.tv_usec = 35000;
            retval = select(sockfd+1, &readfds, NULL, NULL, &tv);

            if (retval == -1)
                perror("select()");
            else if (retval && (FD_ISSET(sockfd, &readfds)))
            {
                DEBUG(printf("Data is available now command.\n"));
                memset(buf, '\0', MAXLIMIT);

                n = recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, NULL, NULL);
                DEBUG(printf("received  packetInfo ack\n"));
                DEBUG(printf("\nack from server %d, current frame of client : %d\n", putPacketAck.frameNumber,putPacketInfo.frameNumber));
                if(!(n>0))
                {
                    perror("recvfrom Command Info:");
                }

                if(putPacketAck.frameNumber == putPacketInfo.frameNumber) // to handle the duplicate packets sent from client
                {
                    CommandDataYetToSend = 0;
                    CorrectAckNotReceived=0;
                    putPacketAck.previousFrameNumber = putPacketAck.frameNumber;
                    putPacketAck.frameNumber = putPacketInfo.frameNumber;//ntohl(packetInfo.frameNumber);
                }
                FD_CLR(sockfd, &readfds);
            }
            else
            {
                DEBUG(printf("No data within 20ms in command.\n"));

                if(errorCounterFrame == putPacketInfo.frameNumber)errorCounter ++;
                else errorCounter =0;
                if(errorCounter == ERROR_COUNTER_LIMIT)
                {
                    printf("There is an issue with the network, Please try again after some time\n");
                    return -1;
                }
                errorCounterFrame = putPacketInfo.frameNumber;
                break;
            }
        }

    }
    CommandDataYetToSend = 1;
    CorrectAckNotReceived = 1;
    udpPacket putUdpPacket;
    frameCounter=0;
    memset(&putUdpPacket, '\0', sizeof(putUdpPacket));
    putUdpPacket.packetInfoNotpacket = 0;

/**********************************************************************
*** The next chunk of code would get the file - the actual data exchange
**********************************************************************/
    while (sizeCheck < fileSize) {
        DataYetToSend = 1;
        readBytes = fread(putUdpPacket.bufferData, 1, MAXLIMIT, fileOpen);
        putUdpPacket.frameNumber = frameCounter;
        encryptDecryptData(putUdpPacket.bufferData , readBytes); // encrypted data and sending 
        putUdpPacket.readBytes = readBytes;
        while(DataYetToSend)
        {
            sentBytes = sendto(sockfd, &putUdpPacket, sizeof(putUdpPacket), 0, (struct sockaddr * ) servaddr, sizeof( * servaddr));
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
                    n = recvfrom(sockfd, &putPacketAck, sizeof(putPacketAck), 0, NULL, NULL);//printf("received packet ack\n");
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

                    if(errorCounterFrame == putUdpPacket.frameNumber) {
                        errorCounter++;
                    } else {
                        errorCounter=0;
                    }
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
        printf("Success sending the file\n");
        fflush(stdout);
    } else {
        printf("Failed to send the file\n");
        fflush(stdout);
    }
    close(sockfd);
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
    DEBUG(printf("hey\n"));
    int sockfd;
    char buf[MAXLIMIT];
    struct sockaddr_in servaddr;
    char commandInput[MAXLIMIT];
    char filePathInput[MAXLIMIT];
    if(argc !=3)
    {
        printf("Please specify the IP address and Port\nExample: ./client 127.0.0.1 50001\n");
        exit(0);
    }
    bzero( & servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(strToInteger(argv[2]));
    if (inet_pton(AF_INET, argv[1], & servaddr.sin_addr) != 1) {
        printf("Invalid IP Address\n");
        exit(-1);
    }

    printf("Usage of Command:\n   get [path/fileName]\n   put [path/fileName]\n   ls\n   delete [path/fileName]\n   exit\n%sExample: get MyFileName.jpg\n", DELIMITER_1);
    unsigned int serverAlive =1;
    while (1) {

        if(serverAlive) {
            printf("%s@Waiting for User Input\n\n", DELIMITER_1);
        }
        memset(buf, 0, MAXLIMIT);
        memset(commandInput, 0, MAXLIMIT);
        memset(filePathInput, 0, MAXLIMIT);

        if (fgets(buf, MAXLIMIT - 1, stdin) == NULL) {
            printf("Invalid Entry\n");
            exit(-1);
        }
        if (sscanf(buf, "%s %s", commandInput, filePathInput) < 1) // this itself takes care of trailing and leading zeros
        {
            printf("Invalid Entry\n");
            continue;
        }
        else
        {
            DEBUG(printf("%s %s", commandInput,filePathInput));
            if(serverAlive) {
                //since switch statement doesnt work, strcmp is better- but I can aslo use my own hash to
                //the inputs and #define them. Once that is done it is easy for int
                if (strcmp(commandInput, PUT_COMMAND) == 0) {
                    if (fileExist(filePathInput)) {
                        if(putFileToServer(filePathInput, & servaddr)==0) {
                            printf("The Md5sum of the file %s is:",filePathInput);
                            sprintf(buf,"md5sum %s",filePathInput);
                            system(buf);
                        }
                    }
                    printf("put command selected\n");

                } else if (strcmp(commandInput, GET_COMMAND) == 0) {
                    printf("get command selected\n");
                    if(getFileFromServer(filePathInput, & servaddr)==0)
                    {
                        printf("The Md5sum of the file %s is:",filePathInput);
                        sprintf(buf,"md5sum %s",filePathInput);
                        system(buf);
                    }
                } else if (strcmp(commandInput, LIST_COMMAND) == 0) {
                    printf("ls command selected\n");
                    listFileAtServer(&servaddr);
                } else if (strcmp(commandInput, EXIT_COMMAND) == 0) {
                    printf("exit command selected\n");
                    exitServer(&servaddr);
                    serverAlive = 0;
                } else if (strcmp(commandInput, DELETE_COMMAND) == 0) {
                    printf("delete command selected\n");
                    deleteFileAtServer(filePathInput,  &servaddr);
                } else {
                    printf("other command selected\n");
                    otherCommandToServer(buf, &servaddr);
                }
            } else {
                printf("Please exit the Client with Cntrl+C, ther server is not Alive.\nYou need to reinitiate both server and the Client\n");
            }
        }
    }

    return 0;
}
