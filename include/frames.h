#ifndef FRAMES_H
#define FRAMES_H



#define FLAG 0x7e
#define ADDRESS 0x03

typedef unsigned char byte;


typedef  enum {
    C_SET = 3 ,
    C_DISC = 11,
    C_UA = 7,
    C_RR = 5,
    C_REJ = 1,
} ControlField;

/**
I -> Information Frame 
S -> Supervision Frame
U -> un-numbered Frame
* */

int sendFrame_i(int fd);
int sendFrame_s(int fd);

int sendFrame_su(int fd,ControlField control);

int buildFrame_su(byte frame[5], int* currentByte,byte nextByte);

int isControl(byte b);

#endif
