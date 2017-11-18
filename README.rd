This is a Reliable UDP communicatin between a client and the server 

TO use this application 
1. please go into each folder "Client" & "Server"
2. OPen terminal in the respective command prompt 
3. execute the make file with "make" command 
4. for client :  
   the syntax of use is ./client [IP adress] [Port NUmber] 
   Eg:
   ./client 127.0.0.1 50001

5. for server :  
   the syntax of use is ./server  [Port NUmber] 
   Eg:
   ./server 50001


The Command you can find in the Client are: 
 COmmand              Usage                       Comments 
get [fILE Name]    get image.jpg               gets the file from the server if it has or appropoiat message is displayed
put [FILE NAME]     put image.jpg              puts the file to the server
delete [FILE NAME]   delete image.jpg          delete the file in the server 
exit                 exit                      exits the server
ls                  ls                        lists the files in the server 
jargon commnds      hbsdc hbdc                 relays back from server 


I have used STOP-N-WAIT protocol to implemenet the Relaiablity
extra: I am waiting for RTT*n is I am receing the ack (of previous frame) from the server. 
I have used Caser Chipher to encrpyt and decrypt the file chunks transfered
