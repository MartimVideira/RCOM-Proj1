#include "../include/frames.h"
#include <string.h>
#include <unistd.h>


int sendFrame_su(int fd, ControlField control){

    byte su[5];

    byte bcc = ADDRESS ^ control;

    su[0] = FLAG;
    su[1] = ADDRESS;
    su[2] = control;
    su[3] = bcc;
    su[4] = FLAG;

    int res = write(fd,su,sizeof su); 
    
    return res;
}

int isControl(byte b){
    switch (b) {
        case C_SET:
        case C_DISC:
        case C_UA:
        case C_REJ: 
        case C_RR:
            return 1;
        default:
            break;
        }
    return 0;
}

int buildFrame_su(byte frame[5], int* currentByte,byte nextByte){
    
    switch (*currentByte) {
        // First Falg
        case 0:
            if(nextByte == FLAG){
                frame[0] = FLAG;
                *currentByte +=1;
            }
            else{
            *currentByte = 0;
            }
            break;
        // A field
        case 1:
            if(nextByte == ADDRESS){
                frame[1] = ADDRESS;
                *currentByte +=1;
            }
            else {
                memset(frame,0,5);
                *currentByte = 0;
                return buildFrame_su(frame,currentByte,nextByte);
            }
            break;
        // Control Field
        case 2:
            if(isControl(nextByte)){
                frame[2] = nextByte;
                *currentByte +=1;
            }
            else {
                memset(frame,0,5);
                *currentByte = 0;
                return buildFrame_su(frame,currentByte,nextByte);
            }
            break;
        // Bcc A XOR Control
        case 3:
            if ((frame[1] ^ frame[2]) == nextByte){
                frame[3] = nextByte;
                *currentByte +=1;
            }
            else {
                memset(frame,0,5);
                *currentByte = 0;
                return buildFrame_su(frame,currentByte,nextByte);
            }
            break;
        // Final Flag
        case 4:
            if (nextByte == FLAG){
                frame[4] = FLAG;
                *currentByte +=1;
                return 1;
            }
            else {
                memset(frame,0,5);
                *currentByte = 0;
                return buildFrame_su(frame,currentByte,nextByte);
            }
            break;
    }
    return 0;
}
