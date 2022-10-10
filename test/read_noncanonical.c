// Read from serial port in non-canonical mode
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

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1
int main()
{

    LinkLayer parameters;
    parameters.nRetransmissions = 3;
    parameters.timeout = 2;
    parameters.baudRate = BAUDRATE;
    parameters.role = LlRx;
    strcpy(parameters.serialPort,"/dev/ttyS10");
    printf("Trying To open a reading connection in /dev/ttyS10\n");
    int established = llopen(parameters);
    char * result = (established)? "" : "not";
    printf("Connection for reading was %s established\n",result);

    unsigned char* p;
    llread(p);

    return 0;
}
