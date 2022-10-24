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
// G_lobals
////////////////////////////////////////////////
struct termios G_oldtio;
struct termios G_newtio;
int G_fd;
int G_READING = 1;
int G_SENDING = 1;
int G_reTransmitions = 0;
int G_frameNumber = 0;
int G_expectedFrameNumber = 0;
LinkLayer G_parameters;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int openSerialPort(LinkLayer connectionParameters) {
  // Open serial port device for reading and writing and not as controlling tty
  // because we don't want to get killed if linenoise sends CTRL-C.
  char *serialPortName = connectionParameters.serialPort;

  printf("Serial Port: %s",serialPortName);
  G_fd = open(serialPortName, O_RDWR | O_NOCTTY);
  if (G_fd < 0) {
    perror(serialPortName);
    return FALSE;
  }

  memset(&G_newtio, 0, sizeof(G_newtio));

  // Save current port settings
  if (tcgetattr(G_fd, &G_oldtio) == -1) {
    perror("tcgetattr");
    return FALSE;
  }

  // Clear struct for new port settings

  G_newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
  G_newtio.c_iflag = IGNPAR;
  G_newtio.c_oflag = 0;

  // Set input mode (non-canonical, no echo,...)
  G_newtio.c_lflag = 0;
  G_newtio.c_cc[VTIME] = 0; // Inter-character timer unused
  G_newtio.c_cc[VMIN] = 0;  // Blocking read until 0 chars received

  // VTIME e VMIN should be changed in order to protect with a
  // timeout the reception of the following character(s)

  // Now clean the line and activate the settings for the port
  // tcflush() discards data written to the object referred to
  // by G_fd but not transmitted, or data received but not read,
  // depending on the value of queue_selector:
  //   TCIFLUSH - flushes data received but not read.
  tcflush(G_fd, TCIOFLUSH);

  // Set new port settings
  if (tcsetattr(G_fd, TCSANOW, &G_newtio) == -1) {
    perror("tcsetattr");
    return FALSE;
  }
  return TRUE;
}
void alarmHandler() {
  // Stop Reading  to Send SET again
  G_READING = 0;
  // Max retransmittions reached stop sending
  if (G_reTransmitions < 1) {
    G_SENDING = 0;
  }
  G_reTransmitions -= 1;
  printf("Retransmition!\n");
}

int llopen(LinkLayer connectionParameters) {
  if (!openSerialPort(connectionParameters)) {
    perror("Error while opening serial port");
    return FALSE;
  }
  G_parameters = connectionParameters;
  printf("Opened serialport\n");
  int connectionEstablished = -1;

  // Tranitter
  if (connectionParameters.role == LlTx) {
    G_reTransmitions = connectionParameters.nRetransmissions;
    (void)signal(SIGALRM, alarmHandler);
    while (G_SENDING) {
      // printf("retransmitions left:%d\n",G_reTransmitions);
      // printf("Sending Set command\n");
      sendFrame_su(G_fd, C_SET);
      alarm(connectionParameters.timeout);
      G_READING = 1;
      byte answer[5];
      memset(answer, 0, 5);
      int currentByte = 0;
      // printf("Waitting For answer\n");
      while (G_READING) {
        byte nextByte = 0;
        if (read(G_fd, &nextByte, 1) == -1)
          continue;
        // printf("Read byte : 0x%x\n",nextByte);
        // printf("answer 0x");
        // for(int i =0; i < 5; i++)
        //     printf("%x",answer[i]);
        // printf("\n");
        if (buildFrame_s(answer, &currentByte, nextByte)) {
          if (answer[2] == C_UA) {
            G_READING = 0;
            G_SENDING = 0;
            connectionEstablished = 1;
            // printf("Connection Was Established for writting RECEIVED UA!\n");
          }
        }
      }
      G_READING = 1;
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
      if (read(G_fd, &nextByte, 1) == -1)
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
          sendFrame_su(G_fd, C_UA);
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
  //printf("Sending frameNumbe :%d\n", G_frameNumber);
  byte *frame = bufferToFrameI(buf, &size, G_frameNumber);
    //printf("Sent frame with frameNumber: %d\n",frameNumber(frame));
  // printf("Writing the following I%d frame : ",G_frameNumber);
  // printHexN(frame, size); printf("\n");
  G_reTransmitions = G_parameters.nRetransmissions;
  int numberBytesWritten = -1;
  (void)signal(SIGALRM, alarmHandler);
  G_SENDING = 1;
  G_READING = 1;
  while (G_SENDING) {
    // printf("retransmitions left:%d\n", G_reTransmitions);
    // printf("Sending frameI%d\n",G_frameNumber);
    sendFrame_i(G_fd, frame, size);
    alarm(G_parameters.timeout);
    G_READING = 1;
    byte answer[5];
    memset(answer, 0, 5);
    int currentByte = 0;
    // printf("Waitting For answer\n");
    while (G_READING) {
      byte nextByte = 0;
      if (read(G_fd, &nextByte, 1) == -1)
        continue;
      if (buildFrame_s(answer, &currentByte, nextByte)) {

        // printf("Received answer: \n");
        // printHexN(answer, 5);
        // printf("\n");

        if (!checkBccFrame_s(answer)) {
          // Someething BCC dosnt match
          // G_reTransmitions = G_parameters.nRetransmissions;
          printf("Answer is wrong! Error on the BCC\n");
        }
        // Everything is okay
        else if ((answer[2] == C_RR0 && G_frameNumber == 1) ||
                 (answer[2] == C_RR1 && G_frameNumber == 0)) {
          G_READING = 0;
          G_SENDING = 0;
          numberBytesWritten = size;
          G_frameNumber = (G_frameNumber) ? 0 : 1;
          printf("Everything is alright got RR\n");
          // Got rej need to send again!
        } else if ((answer[2] == C_REJ0 && G_frameNumber == 0) ||
                   (answer[2] == C_REJ1 && G_frameNumber == 1)) {
          // Se receber rejn do frame n retransmittir o frame n e dar refresc as
          // tentativas
          // G_reTransmitions = G_parameters.nRetransmissions;
          printf("Received rej need to send again\n");
        }
      }
    }
  }
  alarm(0);
  return numberBytesWritten;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) {
  int reading = 1;
  printf("llread\n");
  while (reading) {
    size_t size = 0;
    // Alterar buildFrame_i para considerar que o frame pode ter um MAX_SIZE
    // A funcao deve retornar assim que receber um frame !
    // G_expectedFrameNumber vai ter que passar a ser global!
    // conforme MAX_PAYLOAD_SIZE o frame terá 2*(MAX_PAYLOAD_SIZE + bcc1 +bcc2)
    // + F + C + A + F_final
    byte *frameI = buildFrame_i(G_fd, &size);
    // printf("Received frame: ");
    // printHexN(frameI, size);
    // printf("\n");
    int receivedFrameNumber = frameNumber(frameI);
    // Send Packet again! Error in bcc
    if (!checkBccFrame_i(frameI, size)) {
      ControlField control = C_REJ0;
      if (receivedFrameNumber)
        control = C_REJ1;
      sendFrame_su(G_fd, control);
      printf("Sent Frame Reject{%d}!\n", receivedFrameNumber);
    } else {
      // If received the correct Frame send change G_expectedFrameNumber to Next
      // Else  sender did not receive our RR G_expectedFrameNumber remains the
      // same meaning repeated frame was received
      if (receivedFrameNumber == G_expectedFrameNumber) {
        G_expectedFrameNumber = (G_expectedFrameNumber) ? 0 : 1;
      }
      ControlField control = C_RR0;
      if (G_expectedFrameNumber)
        control = C_RR1;
      sendFrame_su(G_fd, control);
      printf("Received I{%d} Send Next Frame RR%d!\n", receivedFrameNumber,
             G_expectedFrameNumber);
      // Get the packet from the frame
      // while cycle
      for (int i=4,c=0;i < (size-2);i++,c++){
            packet[c] = frameI[i];
      }
      return (size - 6);
    }
  }
  return 0;
}

int llcloseT() {
  int connectionClosed = 0;
  G_reTransmitions = G_parameters.nRetransmissions;
  (void)signal(SIGALRM, alarmHandler);
  while (!connectionClosed) {
    // printf("retransmitions left:%d\n",G_reTransmitions);
    // printf("Sending Set command\n");
    sendFrame_su(G_fd, C_DISC);
    alarm(G_parameters.timeout);
    G_READING = 1;
    byte answer[5];
    memset(answer, 0, 5);
    int currentByte = 0;
    // printf("Waitting For answer\n");
    while (G_READING) {
      byte nextByte = 0;
      if (read(G_fd, &nextByte, 1) == -1)
        continue;
      // printf("Read byte : 0x%x\n",nextByte);
      // printf("answer 0x");
      // for(int i =0; i < 5; i++)
      //     printf("%x",answer[i]);
      // printf("\n");
      if (buildFrame_s(answer, &currentByte, nextByte)) {
        if (answer[2] == C_DISC) {
          sendFrame_su(G_fd, C_UA);
          connectionClosed = 1;
          G_READING = 0;
          G_SENDING = 0;
          //printf("Received disc from reader sending ua\n");
        }
      }
    }
    G_READING = 1;
  }
  alarm(0);
  if (connectionClosed)
    printf("Connecton was correctly closed by the writter\n");
  else
    printf("Connecton was not correctly closed by the writter\n");
  return connectionClosed;
}

int llcloseR() {
  byte answer[5];
  int currentByte = 0;
  int connectionClosed = 0;
  int waitingUA = 0;
  while (!connectionClosed) {
    byte nextByte = 0;
    if (read(G_fd, &nextByte, 1) == -1)
      continue;
    if (buildFrame_s(answer, &currentByte, nextByte)) {
      //printf("Got frame\n");
      if (waitingUA && (answer[2] == C_UA)) {
        connectionClosed = 1;
        //printf("Received UA\n");
      }
      if (answer[2] == C_DISC) {
        waitingUA = 1;
        memset(answer, 0, 5);
        currentByte = 0;
        //printf("Received disc from writter sending disc and waiting for ua \n");
        sendFrame_su(G_fd, C_DISC);
      }
    }
  }
  if (connectionClosed)
    printf("Connecton was correctly closed by the reader\n");
  else
    printf("Connecton was not correctly closed by the reader\n");
  return connectionClosed;
}
////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) {
  printf("llclose\n");
  if (G_parameters.role == LlTx) {
    llcloseT();
  } else
    llcloseR();
  return 1;
}
