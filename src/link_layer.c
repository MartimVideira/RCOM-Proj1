// Link layer protocol implementation
// #include "link_layer.h"
#include "../include/link_layer.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

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
int reTransmitions = 0;
int frameNumber = 0;
LinkLayer parameters;

int openSerialPort(LinkLayer connectionParameters) {
  // Open serial port device for reading and writing and not as controlling tty
  // because we don't want to get killed if linenoise sends CTRL-C.
  char *serialPortName = connectionParameters.serialPort;

  fd = open(serialPortName, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror(serialPortName);
    return FALSE;
  }

  memset(&newtio, 0, sizeof(newtio));

  // Save current port settings
  if (tcgetattr(fd, &oldtio) == -1) {
    perror("tcgetattr");
    return FALSE;
  }

  // Clear struct for new port settings

  newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
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
  if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
    perror("tcsetattr");
    return FALSE;
  }
  return TRUE;
}
void alarmHandler() {
  // Stop Reading  to Send SET again
  READING = 0;
  // Max retransmittions reached stop sending
  if (reTransmitions < 1) {
    SENDING = 0;
  }
  reTransmitions -= 1;
}

int llopen(LinkLayer connectionParameters) {
  if (!openSerialPort(connectionParameters)) {
    perror("Error while opening serial port");
    return FALSE;
  }
  parameters = connectionParameters;
  printf("Opened serialport\n");
  int connectionEstablished = -1;

  // Tranitter
  if (connectionParameters.role == LlTx) {
    reTransmitions = connectionParameters.nRetransmissions;
    (void)signal(SIGALRM, alarmHandler);
    while (SENDING) {
      // printf("retransmitions left:%d\n",reTransmitions);
      // printf("Sending Set command\n");
      sendFrame_su(fd, C_SET);
      alarm(connectionParameters.timeout);
      READING = 1;
      byte answer[5];
      memset(answer, 0, 5);
      int currentByte = 0;
      // printf("Waitting For answer\n");
      while (READING) {
        byte nextByte = 0;
        if (read(fd, &nextByte, 1) == -1)
          continue;
        // printf("Read byte : 0x%x\n",nextByte);
        // printf("answer 0x");
        // for(int i =0; i < 5; i++)
        //     printf("%x",answer[i]);
        // printf("\n");
        if (buildFrame_s(answer, &currentByte, nextByte)) {
          if (answer[2] == C_UA) {
            READING = 0;
            SENDING = 0;
            connectionEstablished = 1;
            // printf("Connection Was Established for writting RECEIVED UA!\n");
          }
        }
      }
      READING = 1;
    }
    alarm(0);
  }
  // Receiver
  else if (connectionParameters.role == LlRx) {
    byte answer[5];
    memset(answer, 0, 5);
    int currentByte = 0;
    int connecting = 1;
    while (connecting) {
      byte nextByte = 0;
      if (read(fd, &nextByte, 1) == -1)
        continue;
      // printf("Read byte : 0x%x\n",nextByte);
      // printf("answer 0x");
      // for(int i =0; i < 5; i++)
      //     printf("%x",answer[i]);
      // printf("\n");
      if (buildFrame_s(answer, &currentByte, nextByte)) {
        if (answer[2] == C_SET) {
          connectionEstablished = 1;
          connecting = 0;
          // printf("Connection Was  For Reading Established! RECEIVED SET\n");
          //  Send Back UA
          sendFrame_su(fd, C_UA);
        }
      }
    }
  }

  return connectionEstablished;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize) {
  // Build I-Frame from buff
  size_t size = bufSize;
  byte *frame = bufferToFrameI(buf, &size, frameNumber);
  printf("Writing the following I%d frame : ",frameNumber);
  printHexN(frame, size);
  printf("\n");
  reTransmitions = parameters.nRetransmissions;
  int frameReceived = -1;
  (void)signal(SIGALRM, alarmHandler);
  SENDING = 1;
  READING = 1;
  while (SENDING) {
    printf("retransmitions left:%d\n", reTransmitions);
    printf("Sending frameI%d\n",frameNumber);
    sendFrame_i(fd, frame, size);
    alarm(parameters.timeout);
    READING = 1;
    byte answer[5];
    memset(answer, 0, 5);
    int currentByte = 0;
    printf("Waitting For answer\n");
    while (READING) {
      byte nextByte = 0;
      if (read(fd, &nextByte, 1) == -1)
        continue;
      if (buildFrame_s(answer, &currentByte, nextByte)) {

        printf("Received answer: \n");
        printHexN(answer, 5);
        printf("\n");

        if (!checkBccFrame_s(answer)) {
          // Someething BCC dosnt match
          // reTransmitions = parameters.nRetransmissions;
          printf("Answer is wrong!\n");
        } else if ((answer[2] == C_RR0 && frameNumber == 1) ||
                   (answer[2] == C_RR1 && frameNumber == 0)) {
          READING = 0;
          SENDING = 0;
          frameReceived = 1;
          frameNumber = (frameNumber) ? 0 : 1;
          printf("Received RR can return");
        } else if ((answer[2] == C_REJ0 && frameNumber == 0) ||
                   (answer[2] == C_REJ1 && frameNumber == 1)) {
          // Se receber rejn do frame n retransmittir o frame n e dar refresc as
          // tentativas
          // reTransmitions = parameters.nRetransmissions;
          printf("Received rej need to send again");
        }
      }
    }
  }
  alarm(0);
  return frameReceived;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) {
  int reading = 1;
  int expectedPacketNumber = 0;
  printf("Reading and Waiting\n");
  while (reading) {
    size_t size = 0;
    // Alterar buildFrame_i para considerar que o frame pode ter um MAX_SIZE 
    // conforme MAX_PAYLOAD_SIZE o frame terá 2*(MAX_PAYLOAD_SIZE + bcc1 +bcc2) + F + C + A + F_final
    byte *frameI = buildFrame_i(fd, &size);
    printf("Received frame: ");
    printHexN(frameI, size);
    printf("\n");
    int receivedFrameNumber = frameI[2] & 0x20;
    // Check if disconnect was sent!

    // Send Packet again! Error in bcc
    if (!checkBccFrame_i(frameI, size)) {
      ControlField control = C_REJ0;
      if (receivedFrameNumber)
        control = C_REJ1;
      sendFrame_su(fd, control);
      printf("Sent Frame Reject%d!\n",receivedFrameNumber);
    } else {
      // If received the correct Frame send change expectedPacketNumber to Next
      // Else  sender did not receive our RR expectedPacketNumber remains the
      // same meaning repeated frame was received
      if (receivedFrameNumber == expectedPacketNumber) {
        expectedPacketNumber = (expectedPacketNumber) ? 0 : 1;
      }
      ControlField control = C_RR0;
      if (expectedPacketNumber)
        control = C_RR1;
      sendFrame_su(fd, control);
      printf("Send Next  Frame RR%d!\n",expectedPacketNumber);
    }
  }
  return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) {
  // TODO

  return 1;
}
