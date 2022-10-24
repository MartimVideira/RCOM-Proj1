// Application layer protocol implementation

//#include "application_layer.h"
#include "../include/application_layer.h"
#include "../include/frames.h"
#include "../include/link_layer.h"
#include <errno.h>
#include <libgen.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Memory leaks
#define MAX_DATAPACKET_SIZE (MAX_PAYLOAD_SIZE - 4)
#define MAX_TLVARRAY_SIZE 10

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

Packet bufferToPacket(byte *buff, int size) {
  Packet p;
  union Content c;
  // DataPacket
  if (buff[0] == PC_DATA) {
    p.control = PC_DATA;
    // Parse Data Packet
    DataPacket dp;
    dp.sequenceNumber = buff[1];
    dp.dataSize[1] = buff[2];
    dp.dataSize[0] = buff[3];
    int dataSize = (dp.dataSize[1] << 8) + dp.dataSize[0];
    printf("Data Size : %d\n", dataSize);
    dp.data = (byte *)(malloc(dataSize * sizeof(byte)));
    for (int i = 0; i < dataSize; i++) {
      dp.data[i] = buff[4 + i];
    }
    c.dataPacket = dp;
  } else {
    p.control = buff[0];
    // Parse Tlv Vector
    Tlv *tlvs = (Tlv *)(malloc(MAX_TLVARRAY_SIZE * sizeof(Tlv)));
    TlvArray t;
    t.buf = tlvs;
    int counter = 1;
    int realNumTlvs = 0;
    while (counter < size) {
      enum ParameterType type = buff[counter++];
      // printf("Type: %d\n",type);
      int valueSize = buff[counter++];
      // printf("length: %d\n",valueSize);
      byte *value = (byte *)(malloc(valueSize * sizeof(byte)));
      for (int i = 0; i < valueSize; i++) {
        value[i] = buff[counter++];
        // printf("0x%x ",value[i]);
      }
      // printf("\n");
      Tlv current;
      current.value = value;
      current.type = type;
      current.length = valueSize;
      tlvs[realNumTlvs] = current;
      realNumTlvs++;
    }
    t.size = realNumTlvs;
    c.tlvArray = t;
  }
  p.content = c;
  return p;
}

void applicationSend(LinkLayer parameters, const char *filename) {

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

  //printf("About the File\n");
  //printf("Name: %s\n", baseFilename);
  //printf("Size: %ld bytes\n", fileStat.st_size);

  Tlv fileSize = {PT_FILE_SIZE, sizeof(fileStat.st_size),
                  (byte *)(&fileStat.st_size)};
  Tlv fileName = {PT_FILE_NAME, strlen(baseFilename), (byte *)baseFilename};

  Tlv buf[2];
  buf[0] = fileSize;
  buf[1] = fileName;

  TlvArray tlvInfo = {buf, 2};
  union Content controlContent;
  controlContent.tlvArray = tlvInfo;

  Packet packet = {PC_START, controlContent};
  int size = 0;
  byte *packetBuffer = packetToBuffer(packet, &size);
  if(llwrite(packetBuffer, size) == -1)
            printf("Timout occured! Exitting Program\n");
  // enviar e depois libertar

  // printf("Packet: ");
  // printHexN(packetBuffer, size);
  // printf("\n");

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
    //printf("Read packet %d\n", currentPacket);
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
    
    if(llwrite(currentPacketBuffer, size) == -1){
            printf("Timout occured! Exitting Program\n");
        }

    //printf("Primeiro DataPacket: ");
    //printHexN(currentPacketBuffer, size);
    //printf("\n");

    currentPacket++;
    free(currentPacketBuffer);
  }
  packet.control = PC_END;
  byte *pEnd = packetToBuffer(packet, &size);
  if(llwrite(pEnd, size))
            printf("Timout occured! Exitting Program\n");
  //printf("Primeiro DataPacket: ");
  //printHexN(pEnd, size);
  //printf("\n");
  // send packet end
  //  Send packet end
  //  llwrite(packetBuffer, sizeof(packet));

  fclose(fdFile);
}

void applicationReceive(LinkLayer parameters) {

  int size = 0;
  int finished = 0;
  byte *buffer = (byte *)(malloc(MAX_PAYLOAD_SIZE * (sizeof(byte))));

  // Read First Packet and
  size = llread(buffer);
  Packet header = bufferToPacket(buffer, size);
  char *newFileName;

  //printf("DataPacket: ");
  //printHexN(buffer, size);
  //printf("\n");
  //printf("header. %d\n", header.control);
  for (int i = 0; i < header.content.tlvArray.size; i++) {
    enum ParameterType t = header.content.tlvArray.buf[i].type;
    // printf("Here type is %d\n",t);
    if (t == PT_FILE_SIZE) {
      // nothing?
    } else if (t == PT_FILE_NAME) {
      int fileNameSize = header.content.tlvArray.buf[i].length;
      // printf("filename size:%d\n",fileNameSize);
      newFileName = (char *)(malloc(fileNameSize * sizeof(char)));
      memccpy(newFileName, header.content.tlvArray.buf[i].value, 0,
              fileNameSize);
    }
  }
  // printf("FileName %s\n", newFileName);
  char *newFileName2 = malloc(strlen(newFileName) + 20);
  // printf("FileName2 %s\n", newFileName2);
  strcpy(newFileName2,"new_");
  strcat(newFileName2, newFileName);
  // printf("FileName2 %s\n", newFileName2);
  FILE *f = fopen(newFileName2, "w");
  free(newFileName);
  free(newFileName2);
  while (!finished) {
    int size = llread(buffer);
    if (buffer[0] == 0x3) {
      finished = 1;
      break;
    }
    printf("Wrote to file! bytes: %d\n",size);
    
    fwrite((buffer + 4), size - 4,1, f);
    // printf("Primeiro DataPacket: ");
    // printHexN(buffer, size);
    // printf("\n");
  }
  fclose(f);
  free(buffer);
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
  LinkLayerRole llrole;
  if (strcmp(role, "tx") == 0) {
    //printf("Escritor\n");
    llrole = LlTx;
  } else if (strcmp(role,"rx") == 0){
    //printf("Recetor\n");
    llrole = LlRx;
  }

  LinkLayer ll;
  ll.role = llrole;
  ll.baudRate = baudRate;
  ll.nRetransmissions = nTries;
  ll.timeout = timeout;
  strcpy(ll.serialPort, serialPort);
  printf("Serial Port: %s",ll.serialPort);
  if (llopen(ll) == -1) {
    printf("Could not establish connection");
    return;
  }

  if (llrole == LlTx) {
    applicationSend(ll, filename);
  } else {
    applicationReceive(ll);
  }
  llclose(3);
}
