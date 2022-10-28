#ifndef FRAMES_H
#define FRAMES_H
#include <stdlib.h>
#include <time.h>


#define MAX_PAYLOAD_SIZE 10000

#define MAX_FRAME_SIZE (MAX_PAYLOAD_SIZE*2 + 2 + 1 + 2*2)

#define FLAG 0x7e
#define ESCAPE_CHR 0x7d
#define ESCAPE_XOR_CHR 0x20
#define ADDRESS 0x03

typedef unsigned char byte;


typedef  enum {
    C_SET = 3 ,
    C_DISC = 11,
    C_UA = 7,
    C_RR0 = 5,
    C_RR1 = (5+128),
    C_REJ0 = 1,
    C_REJ1 = (1+128)
} ControlField;

/**
I -> Information Frame 
S -> Supervision Frame
U -> un-numbered Frame
* */

// Function to get frame number if has number.
int sendFrame_s(int fd);

int sendFrame_su(int fd,ControlField control);
int checkBccFrame_s(byte frame[5]);
int buildFrame_s(byte frame[5], int* currentByte,byte nextByte);
int sendFrame_i(int fd,const byte* frameI,size_t size);
int isControl(byte b);

byte *buildFrame_i(int fd, size_t *size);
int checkBccFrame_i(const byte* frameI,size_t size);


byte* byteStuff(const byte* string,size_t *size);
byte* byteDeStuff(const byte* string,size_t *size);

byte* byteStuffString(const byte* string);
byte* byteDeStuffString(const byte* string);

byte* bufferToFrameI(const byte* buf,size_t *size,int number);
void printHexN(byte* string,size_t size);

int frameNumber(byte* frame);
#endif
