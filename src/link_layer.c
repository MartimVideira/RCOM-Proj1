// Link layer protocol implementation
// #include "link_layer.h"
#include "../include/link_layer.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

#include "../include/frames.h"

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

struct termios oldtio;
struct termios newtio;
int fd;
int READING = 1;
int SENDING = 1;
int reTransmitions= 0;

int openSerialPort(LinkLayer connectionParameters){
   
    // Open serial port device for reading and writing and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    char * serialPortName = connectionParameters.serialPort;

    fd = open(serialPortName, O_RDWR | O_NOCTTY);
    if (fd < 0)
    {
        perror(serialPortName);
        return FALSE;
    }

    memset(&newtio, 0, sizeof(newtio));

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        return FALSE;
    }

    // Clear struct for new port settings

    newtio.c_cflag = connectionParameters.baudRate| CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 0 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        return FALSE;
    }
    return TRUE;

}
void alarmHandler(){
    // Stop Reading  to Send SET again
    READING = 0;
    // Max retransmittions reached stop sending
    if(reTransmitions <= 1){
        SENDING = 0;
    }
    reTransmitions -=1;
}

int llopen(LinkLayer connectionParameters)
{
    if (!openSerialPort(connectionParameters)){
        perror("Error while opening serial port");
        return FALSE;
    }
    printf("Opened serialport\n");
    int connectionEstablished = -1;

    reTransmitions = connectionParameters.nRetransmissions;
    //Tranitter
    if (connectionParameters.role == LlTx){
        (void)signal(SIGALRM, alarmHandler);
        while (SENDING) {
            //printf("retransmitions left:%d\n",reTransmitions);
            //printf("Sending Set command\n");
            sendFrame_su(fd,C_SET);
            alarm(connectionParameters.timeout);
            READING  = 1;
            byte answer[5]; 
            memset(answer,0,5);
            int currentByte = 0;
            // printf("Waitting For answer\n");
            while(READING){
                byte nextByte = 0;
                if(read(fd,&nextByte,1) == -1)
                    continue;
                // printf("Read byte : 0x%x\n",nextByte);
                // printf("answer 0x");
                // for(int i =0; i < 5; i++)
                //     printf("%x",answer[i]);
                // printf("\n");
                if(buildFrame_su(answer, &currentByte,nextByte)){
                    if(answer[2] == C_UA){
                        READING = 0;
                        SENDING = 0;
                        connectionEstablished = 1;
                        //printf("Connection Was Established for writting RECEIVED UA!\n");
                    }
                }
            }
            READING = 1;
        }
        alarm(0);
    }
    // Receiver
    else if (connectionParameters.role == LlRx){
        byte answer[5]; 
        memset(answer,0,5);
        int currentByte = 0;
        int connecting = 1;
        while(connecting){
            byte nextByte= 0;
            if(read(fd,&nextByte,1) == -1)
                continue;
            //printf("Read byte : 0x%x\n",nextByte);
            //printf("answer 0x");
            //for(int i =0; i < 5; i++)
            //    printf("%x",answer[i]);
            //printf("\n");
            if(buildFrame_su(answer, &currentByte,nextByte)){
                if(answer[2] == C_SET){
                    connectionEstablished = 1;
                    connecting = 0;
                    //printf("Connection Was  For Reading Established! RECEIVED SET\n");
                    // Send Back UA 
                    sendFrame_su(fd,C_UA);
                }
            }
        }
    }

    return connectionEstablished;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // Build I-Frame from buff
    int number = 0;
    size_t size = bufSize;
    byte* frame = bufferToFrameI(buf,&size,number);

    // Send I-Frame
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO
    // Receive I-Frame
    // byte DeDestuff 
    // Return
    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
