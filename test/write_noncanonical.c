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

void testLLOpenWrite(){
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
}

void printHex(byte* string){
    while(*string != 0){
        printf("0x%x ",*string);
        string++;
    }
}

void testByteStuffing(){
    byte testFlag[] = {FLAG,0};
    byte testEscape[] = {ESCAPE_CHR,0};
    byte testFEF[] = {FLAG,ESCAPE_CHR,0};
    byte testFEFEFF[] = {FLAG,ESCAPE_CHR,FLAG,ESCAPE_CHR,FLAG,FLAG,ESCAPE_CHR,0};

    byte** tests =(byte**)malloc(5 * sizeof(byte*));
    tests[0] = testFlag;
    tests[1] = testEscape;
    tests[2] = testFEF;
    tests[3] =testFEFEFF;
    tests[4] = 0;

    byte** iterTests = tests;
    while (*iterTests != 0){
        printf("\n\n\n");
        printf("Before Byte Stuffing: ");
        printHex(*iterTests);
        printf("\n");
        unsigned char* stuffed = byteStuff(*iterTests);
        printf("After Byte Stuffing: ");
        printHex(stuffed);
        free(stuffed);
        iterTests++;
    }
    free(tests);
    printf("\nFinished testing byteStuff\n");
}

void testByteDeStuffing(){
    byte testFlag[] = {FLAG,0};
    byte testEscape[] = {ESCAPE_CHR,0};
    byte testFEF[] = {FLAG,ESCAPE_CHR,0};
    byte testFEFEFF[] = {FLAG,ESCAPE_CHR,FLAG,ESCAPE_CHR,FLAG,FLAG,ESCAPE_CHR,0};

    byte** tests =(byte**)malloc(5 * sizeof(byte*));
    tests[0] = testFlag;
    tests[1] = testEscape;
    tests[2] = testFEF;
    tests[3] =testFEFEFF;
    tests[4] = 0;

    byte** iterTests = tests;
    while (*iterTests != 0){
        printf("\n\n\n");
        printf("Before Byte Stuffing: ");
        printHex(*iterTests);
        printf("\n");
        unsigned char* stuffed = byteStuff(*iterTests);
        printf("After Byte  Stuffing: ");
        printHex(stuffed);
        printf("\n");
        unsigned char* deStuffed = byteDeStuff(stuffed);
        printf("After     deStuffing: ");
        printHex(deStuffed);

        free(stuffed);
        free(deStuffed);

        iterTests++;
    }
    free(tests);
    printf("\nFinished testing byteDeStuff\n");
}

int main()
{
    testByteDeStuffing();
    return 0;
}
