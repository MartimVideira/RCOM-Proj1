// Write to serial port in non-canonical mode
//
// Modified by: Eduardo Nuno Almeida [enalmeida@fe.up.pt]

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "../include/link_layer.h"

#include "../include/frames.h"
// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 256

volatile int STOP = FALSE;

int main()
{
    LinkLayer parameters;
    parameters.nRetransmissions = 2;
    parameters.timeout = 1;
    parameters.baudRate = BAUDRATE;
    parameters.role = LlTx;
    strcpy(parameters.serialPort,"/dev/ttyS11");
    printf("Trying To open a writting connection in /dev/ttyS11\n");

    int established = llopen(parameters);
    char * result = (established)? "" : "not";
    printf("Connection for writting was %s established\n",result);

    return 0;
}
