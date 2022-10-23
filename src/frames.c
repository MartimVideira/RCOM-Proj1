#include "../include/frames.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int sendFrame_su(int fd, ControlField control) {
  byte su[5];

  byte bcc = FLAG ^ ADDRESS ^ control;

  su[0] = FLAG;
  su[1] = ADDRESS;
  su[2] = control;
  su[3] = bcc;
  su[4] = FLAG;
  // printf("Sending frame Supervision : ");printHexN(su,5);printf("\n");
  int res = write(fd, su, sizeof su);

  return res;
}

int frameNumber(byte* frame){
    return (frame[2] & 0x40)? 1 : 0;
}
int sendFrame_i(int fd, const byte *frameI, size_t size) {
  int res = write(fd, frameI, size);
  return res;
}

int isControl(byte b) {
  switch (b) {
  case C_SET:
  case C_DISC:
  case C_UA:
  case C_REJ0:
  case C_REJ1:
  case C_RR0:
  case C_RR1:
    return 1;
  default:
    break;
  }
  return 0;
}
int checkBccFrame_s(byte frame[5]) {
  return frame[3] == (frame[0] ^ frame[1] ^ frame[2]);
}
int buildFrame_s(byte frame[5], int *currentByte, byte nextByte) {
  switch (*currentByte) {
  // First Falg
  case 0:
    if (nextByte == FLAG) {
      frame[0] = FLAG;
      *currentByte += 1;
    } else {
      *currentByte = 0;
    }
    break;
  // A field
  case 1:
    if (nextByte == ADDRESS) {
      frame[1] = ADDRESS;
      *currentByte += 1;
    } else {
      memset(frame, 0, 5);
      *currentByte = 0;
      return buildFrame_s(frame, currentByte, nextByte);
    }
    break;
  // Control Field
  case 2:
    if (isControl(nextByte)) {
      frame[2] = nextByte;
      *currentByte += 1;
    } else {
      memset(frame, 0, 5);
      *currentByte = 0;
      return buildFrame_s(frame, currentByte, nextByte);
    }
    break;
  // Bcc A XOR Control
  case 3:
    if ((frame[1] ^ frame[2] ^ frame[0]) == nextByte) {
      frame[3] = nextByte;
      *currentByte += 1;
    } else {
      memset(frame, 0, 5);
      *currentByte = 0;
      return buildFrame_s(frame, currentByte, nextByte);
    }
    break;
  // Final Flag
  case 4:
    if (nextByte == FLAG) {
      frame[4] = FLAG;
      *currentByte += 1;
      //printf("Got Frame control was : %d\n",frame[2]);
      return 1;
    } else {
      memset(frame, 0, 5);
      *currentByte = 0;
      return buildFrame_s(frame, currentByte, nextByte);
    }
    break;
  }
  return 0;
}
byte *buildFrame_i(int fd, size_t *size) {
  size_t currentByte = 0;
  int reading = 1;
  byte frameI[MAX_FRAME_SIZE];
  // printf("Start Reading in buildFrameI\n");
  while (reading) {
    byte nextByte = 0;
    if (read(fd, &nextByte, sizeof(byte)) <= 0)
      continue;

    // printHexN(&nextByte,1);printf("\n");
    // printf("Frame Before logic :
    // ");printHexN(frameI,currentByte);printf("\n");

    if ((currentByte == 0 && nextByte == FLAG) ||
        (currentByte == 1 && nextByte == ADDRESS) ||
        (currentByte == 2 && (nextByte == 0x00 || nextByte == 0x40))) {
      frameI[currentByte] = nextByte;
      currentByte++;
    }
    // BCC or data Field
    else if (currentByte > 2) {
      frameI[currentByte] = nextByte;
      currentByte++;
      if (nextByte == FLAG) {
        reading = 0;
      }
    } else {
      memset(frameI, 0, sizeof(frameI));
      printf("Got an error building frame I\n");
      currentByte = 0;
      if (nextByte == FLAG) {
        frameI[currentByte] = FLAG;
        currentByte++;
      }
    }
  }
  // printf("Frame Final Frame before deStuffing :
  // ");printHexN(frameI,currentByte);printf("\n");
  byte *res = byteDeStuff(frameI, &currentByte);
  // printf("Frame Final Frame after destuffing:
  // ");printHexN(frameI,currentByte);printf("\n");
  *size = currentByte;
  return res;
}
int checkBccFrame_i(const byte *frameI, size_t size) {
  if (size < 7)
    return 0;

  int bcc1Correct = (frameI[0] ^ frameI[1] ^ frameI[2]) == frameI[3];
  byte bcc2 = frameI[4];
  for (int i = 5; i < (size - 2); i++) {
    bcc2 ^= frameI[i];
  }
  int bcc2Correct = bcc2 == frameI[size - 2];

  return bcc1Correct && bcc2Correct;
}

byte *byteStuff(const byte *array, size_t *size) {
  size_t stuffedSize = 0;
  size_t array_it = 0;
  while (array_it < *size) {
    if (array[array_it] == FLAG || array[array_it] == ESCAPE_CHR)
      stuffedSize++;
    stuffedSize++;
    array_it++;
  }

  byte *stuffedArray = (byte *)malloc(stuffedSize * sizeof(byte));

  array_it = 0;
  size_t stuffed_it = 0;
  while (array_it < *size) {
    if (array[array_it] == FLAG || array[array_it] == ESCAPE_CHR) {
      stuffedArray[stuffed_it] = ESCAPE_CHR;
      stuffed_it++;
      stuffedArray[stuffed_it] = array[array_it] ^ ESCAPE_XOR_CHR;
    } else
      stuffedArray[stuffed_it] = array[array_it];
    array_it++;
    stuffed_it++;
  }

  *size = stuffedSize;
  return stuffedArray;
}

byte *byteDeStuff(const byte *array, size_t *size) {
  // Can Be negative at times
  int deStuffedSize = 0;
  size_t array_it = 0;
  while (array_it < *size) {
    deStuffedSize++;
    if (array[array_it] == ESCAPE_CHR)
      deStuffedSize--;
    array_it++;
  }

  byte *deStuffed = (byte *)malloc(deStuffedSize * sizeof(byte));

  array_it = 0;
  size_t deStuffed_it = 0;
  while (array_it < *size) {
    if (array[array_it] == ESCAPE_CHR) {
      array_it++;
      deStuffed[deStuffed_it] = array[array_it] ^ ESCAPE_XOR_CHR;
    } else
      deStuffed[deStuffed_it] = array[array_it];
    deStuffed_it++;
    array_it++;
  }
  *size = deStuffedSize;
  return deStuffed;
}

byte *byteStuffString(const byte *string) {
  size_t len = strlen(string) + 1;
  byte *result = byteStuff(string, &len);
  result[len] = 0;
  return result;
}
byte *byteDeStuffString(const byte *string) {
  size_t len = strlen(string) + 1;
  byte *result = byteDeStuff(string, &len);
  result[len] = 0;
  return result;
}

byte *bufferToFrameI(const byte *buf, size_t *size, int number) {
  // To stuff frame is the data and the 2 bccs
  size_t toStuffSize = *size + 2;
  byte *toStufframe = (byte *)malloc(toStuffSize * sizeof(byte));

  byte control = (number) ? 0x40:0;

  byte bcc1 = FLAG ^ ADDRESS ^ control;

  toStufframe[0] = bcc1;

  size_t i = 0;
  toStufframe[1] = buf[i];
  i++;
  byte bcc2 = toStufframe[1];
  for (; i < *size; i++) {
    bcc2 ^= buf[i];
    toStufframe[1 + i] = buf[i];
  }
  toStufframe[1 + i] = bcc2;
  *size = toStuffSize;
  // print to stuff frame before
  // printf("Before Stuffing : ");printHexN(toStufframe,*size);printf("\n");
  //  After this call size will be the correct size of the stuffed byte stream
  byte *stuffedFrame = byteStuff(toStufframe, size);

  // printf("After Stuffing : ");printHexN(stuffedFrame,*size);printf("\n");
  //  add flags and address and control

  // After Stuffing we insert the frames f a c and last f
  size_t finalSize = *size + 4;
  byte *finalFrame = (byte *)malloc(finalSize * sizeof(byte));

  size_t f_it = 0;
  finalFrame[f_it++] = FLAG;
  finalFrame[f_it++] = ADDRESS;
  finalFrame[f_it++] = control;
  for (size_t c = 0; c < *size; c++)
    finalFrame[f_it++] = stuffedFrame[c];

  // Close the frame
  finalFrame[f_it] = FLAG;
  // Free Intermediate Alocated  Results
  free(toStufframe);
  free(stuffedFrame);

  // printf("Final Frame: ");printHexN(finalFrame,finalSize);printf("\n");

  *size = finalSize;
  return finalFrame;
}

void printHexN(byte *string, size_t size) {
  size_t i = 0;
  while (i < size) {
    printf("0x%x ", string[i]);
    i++;
  }
}
