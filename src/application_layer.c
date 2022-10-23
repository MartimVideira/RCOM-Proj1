// Application layer protocol implementation

//#include "application_layer.h"
#include "../include/application_layer.h"
#include <errno.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../include/frames.h"
#include "../include/link_layer.h"

#define MAX_DATAPACKET_SIZE (MAX_PAYLOAD_SIZE - 4)

void printHexN(byte *string, size_t size) {
  size_t i = 0;
  while (i < size) {
    printf("0x%x ", string[i]);
    i++;
  }
}

typedef unsigned char byte;

typedef struct {
  byte sequenceNumber;
  byte dataSize[2];
  byte *data;

} DataPacket;

enum ParameterType { PT_FILE_SIZE = 1, PT_FILE_NAME = 2 };

typedef struct {
  enum ParameterType type;
  byte length;
  byte *value;

} Tlv;

typedef struct {
  Tlv *buf;
  int size;
} TlvArray;

enum PacketControl { PC_DATA = 1, PC_START = 2, PC_END = 3 };

typedef struct {

  enum PacketControl control;
  union Content {
    TlvArray tlvArray;
    DataPacket dataPacket;
  } content;

} Packet;

byte *packetToBuffer(Packet p, int *size) {
  byte *buff;
  if (p.control == PC_DATA) {
    // 1Controlo + 1sequenceNumber + 2DataSize + (1*dataSize)
    int dataSize = (p.content.dataPacket.dataSize[1] << 8) +
                   p.content.dataPacket.dataSize[0];
    printf("DataSize : %d\n", dataSize);
    *size = 4 + dataSize;
    buff = (byte *)malloc((*size) * sizeof(byte));
    buff[0] = p.control;
    buff[1] = p.content.dataPacket.sequenceNumber;
    buff[2] = p.content.dataPacket.dataSize[1];
    buff[3] = p.content.dataPacket.dataSize[0];
    int buf_i = 4;
    for (int data_i = 0; data_i < dataSize; data_i++, buf_i++) {
      buff[buf_i] = p.content.dataPacket.data[data_i];
    }
  } else {
    // Controlo + tamanho de cada tlv
    // tlv : 1tipo + 1tamanho + 1*tamanho
    int tlvSize = 0;
    printf("TLVArray size : %d\n", p.content.tlvArray.size);
    for (byte i = 0; i < p.content.tlvArray.size; i++) {
      Tlv current = p.content.tlvArray.buf[i];
      tlvSize += 1 + 1 + current.length;
    }
    printf("TLVArray size: %d\n", tlvSize);
    *size = 1 + tlvSize;

    buff = (byte *)malloc((*size) * sizeof(byte));
    buff[0] = p.control;
    int buf_i = 1;
    // For each tlv
    for (byte i = 0; i < p.content.tlvArray.size; i++) {
      Tlv current = p.content.tlvArray.buf[i];
      buff[buf_i++] = current.type;
      buff[buf_i++] = current.length;
      // Copy the value into the buffer
      for (byte j = 0; j < current.length; j++) {
        buff[buf_i++] = current.value[j];
      }
    }
  }
  return buff;
}

Packet bufferToPacket(byte* buff,int size){
    // DataPacket
    if (buff[0] == PC_DATA){
    }


    // Contol Packet
    else{
    }

}

void applicationSend(LinkLayer parameters, const char *filename) {

  printf("Here\n");
  struct stat fileStat;
  if (stat(filename, &fileStat) != 0) {
    printf("File does not exist\n");
    return;
  }

  if (S_ISREG(fileStat.st_mode) == 0) {
    printf("The given path corresponds to a directory!\n");
    return;
  }

  char filePath[300];
  strcpy(filePath, filename);
  const char *baseFilename = basename(filePath);

  printf("About the File\n");
  printf("Name: %s\n", baseFilename);
  printf("Size: %ld bytes\n", fileStat.st_size);

  Tlv fileSize = {1, sizeof(fileStat.st_size), (byte *)(&fileStat.st_size)};
  Tlv fileName = {2, strlen(baseFilename), (byte *)baseFilename};

  Tlv buf[2];
  buf[0] = fileSize;
  buf[1] = fileName;

  TlvArray tlvInfo = {buf, 2};
  union Content controlContent;
  controlContent.tlvArray = tlvInfo;

  Packet packet = {PC_START, controlContent};
  int size = 0;
  byte *packetBuffer = packetToBuffer(packet, &size);
  printf("Packet: ");
  printHexN(packetBuffer, size);
  printf("\n");


  // Write start control packet to signal start of file

  // memcpy(packetBuffer, &packet, sizeof(packet));
  //  llwrite(packetBuffer, sizeof(packet));

  printf("File : %s\n", filename);
  FILE *fdFile = fopen(filename, "rb");
  if (NULL == fdFile) {
    printf("Error while opening the file %s\n", strerror(errno));
    return;
  }

  // Carefull
  int totalPackets =
      ceil((double)fileStat.st_size / (double)MAX_DATAPACKET_SIZE);

  printf("Total Packets :%d\n", totalPackets);
  byte data[MAX_DATAPACKET_SIZE];
  int currentPacket = 0;
  while (currentPacket < totalPackets) { // Not sure how yet

    memset(data, 0, sizeof(data));
    size_t bufferSize = fread(data, sizeof(byte), MAX_DATAPACKET_SIZE, fdFile);

    if (bufferSize == -1)
      break;
    printf("Read packet %d\n", currentPacket);
    DataPacket p;
    p.sequenceNumber = currentPacket % 255;
    p.dataSize[0] = bufferSize & (0xFF);
    p.dataSize[1] = bufferSize >> 8;
    p.data = data;
    union Content content;
    content.dataPacket = p;
    Packet dp;
    dp.control = PC_DATA;
    dp.content = content;
    int size = 0;

    byte *currentPacketBuffer = packetToBuffer(dp, &size);

     printf("Primeiro DataPacket: ");
     printHexN(currentPacketBuffer, size);
     printf("\n");
    // Mandar o pacotellwrite
    // printf("sequenceNumber:0x%x\n", p.sequenceNumber);
    // printf("dataSize[0]:0x%x\n", p.dataSize[0]);
    // printf("datSize[1]:0x%x\n", p.dataSize[1]);
    // printf("DataSize : %d\n", (p.dataSize[0] + (p.dataSize[1] << 8)));
    // printf("Read: ");
    // printHexN(data, bufferSize);
    // printf("\n");
    currentPacket++;
    free(currentPacketBuffer);
  }

  // Write end control packet to signal end of file

  // packet.content.tlvArray = tlvInfo;
  // packet.control = 3;

  // memcpy(packetBuffer, &packet, sizeof(packet));
  // llwrite(packetBuffer, sizeof(packet));

  // fclose(fdFile);
}

void applicationReceive(LinkLayer parameters) {

  FILE *fdFile;
  Packet packet;
  char *packetBuffer;

  while (1) {
    llread(packetBuffer);
    memcpy(&packet, packetBuffer, sizeof(packet));

    if (packet.control == 2) {
      fdFile = fopen(packet.content.tlvArray.buf[0].value,
                     "wb"); // read from filename
    } else if (packet.control == 1) {
      fwrite(packet.content.dataPacket.data, 1,
             packet.content.dataPacket.dataSize, fdFile);
    } else {
      llclose(1);
      break;
    }
  }

  fclose(fdFile);
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
  LinkLayerRole llrole;
  if (strcmp(role, "Lltx") == 0)
    llrole = LlTx;
  else
    llrole = LlRx;

  LinkLayer ll;
  ll.role = llrole;
  ll.baudRate = baudRate;
  ll.nRetransmissions = nTries;
  ll.timeout = timeout;
  strcpy(ll.serialPort, serialPort);

  if (llrole == LlTx)
    applicationSend(ll, filename);
  else
    applicationReceive(ll);
}
