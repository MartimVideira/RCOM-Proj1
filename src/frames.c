#include "../include/frames.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


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

byte* byteStuff(byte* array,size_t *size){
    size_t stuffedSize = 0;
    size_t array_it = 0;
    while(array_it < *size){
        if(array[array_it] == FLAG || array[array_it] == ESCAPE_CHR)
            stuffedSize++;
        stuffedSize++;
        array_it++;
    }
    
    byte* stuffedArray = (byte*)malloc(stuffedSize*sizeof(byte));

    array_it = 0;
    size_t stuffed_it = 0;
    while(array_it < *size){
        if(array[array_it] == FLAG || array[array_it] == ESCAPE_CHR){
            stuffedArray[stuffed_it] = ESCAPE_CHR;
            stuffed_it++;
            stuffedArray[stuffed_it] = array[array_it] ^ ESCAPE_XOR_CHR;
        }
        else 
            stuffedArray[stuffed_it] = array[array_it];
        array_it++;
        stuffed_it++;
    }

    *size = stuffedSize;
    return stuffedArray;
}

byte* byteDeStuff(byte* array,size_t *size){
    // Can Be negative at times
    int deStuffedSize = 0;
    size_t array_it = 0;
    while(array_it < *size){
        deStuffedSize++;
        if(array[array_it] == ESCAPE_CHR)
            deStuffedSize--;
        array_it++;
    }

    byte* deStuffed = (byte*)malloc(deStuffedSize *sizeof(byte));

    array_it = 0;
    size_t deStuffed_it = 0;
    while(array_it < *size){
        if(array[array_it] == ESCAPE_CHR){
            array_it++;
            deStuffed[deStuffed_it] = array[array_it] ^ ESCAPE_XOR_CHR;
        }
        else
            deStuffed[deStuffed_it] = array[array_it];
        deStuffed_it++;
        array_it++;
    }
    *size = deStuffedSize;
    return deStuffed;

}

byte* byteStuffString(byte* string){
    size_t len = strlen(string)+1;
    byte* result = byteStuff(string,&len);
    result[len] = 0;
    return result;
}
byte* byteDeStuffString(byte* string){
    size_t len = strlen(string)+1;
    byte* result = byteDeStuff(string,&len);
    result[len] = 0;
    return result;
}
