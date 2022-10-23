// Application layer protocol implementation

//#include "application_layer.h"
#include "../include/application_layer.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>

#include "../include/link_layer.h"

typedef unsigned char byte;


typedef struct{
    byte sequenceNumber;
    byte dataSize[2];
    byte* data;

} DataPacket;

enum ParameterType{
    PT_FILE_SIZE = 1,
    PT_FILE_NAME = 2
};

typedef struct {
    enum ParameterType type;
    byte length;
    byte* value;

} Tlv;

typedef struct{ 
    Tlv* buf;
    int size;
} TlvArray;

enum PacketControl{
    PC_DATA = 1,
    PC_START = 2,
    PC_END = 3
};

typedef struct{

    enum PacketControl control;
    union Content {
        TlvArray tlvArray;
        DataPacket data;
    } content;

} Packet;


void applicationSend(LinkLayer parameters,const char *filename){

    struct stat fileStat;
    if (stat(filename,&fileStat) != 0) {
        printf("File does not exist\n");
        return;
    }

    if (S_ISREG(fileStat.st_mode) == 0) {
        printf("The given path corresponds to a directory!\n");
        return;
    }

    char  filePath[300];
    strcpy(filePath, filename);
    const char* baseFilename = basename(filePath);

    printf("About the File\n");
    printf("Name: %s\n",baseFilename);
    printf("Size: %ld bytes\n",fileStat.st_size);

    Tlv fileName = {2, sizeof(baseFilename), baseFilename};
    Tlv fileSize = {1, sizeof(fileStat.st_size), fileStat.st_size};

    Tlv *buf;
    buf[0] = fileName;
    buf[1] = fileSize;

    TlvArray tlvInfo = {buf, sizeof(buf)};
    union Content content;
    content.tlvArray = tlvInfo;

    Packet packet = {2, content};
    char * packetBuffer;

    // Write start control packet to signal start of file

    memcpy(packetBuffer, &packet, sizeof(packet));
    llwrite(packetBuffer, sizeof(packet));

    FILE* fdFile = fopen(filename, "rb");
    byte* data;

    while (1) { // Not sure how yet

        size_t bufferSize = fread(data, 1, 4, fdFile);

        if (bufferSize == -1)
            break;

        byte dataSize[2];
        dataSize[0] = (bufferSize >> 8) & 0xFF;
        dataSize[1] = bufferSize & 0xFF;

        DataPacket dataPacket = {0, dataSize, data};
        packet.content.data = dataPacket;
        packet.control = 1;

        memcpy(packetBuffer, &packet, sizeof(packet));
        llwrite(packetBuffer, sizeof(packet));
    
    }

    // Write end control packet to signal end of file

    packet.content.tlvArray = tlvInfo;
    packet.control = 3;

    memcpy(packetBuffer, &packet, sizeof(packet));
    llwrite(packetBuffer, sizeof(packet));
    
    fclose(fdFile);
    
}

void applicationReceive(LinkLayer parameters){

    FILE* fdFile;
    Packet packet;
    char * packetBuffer;

    while (1)
    {
        llread(packetBuffer);
        memcpy(&packet, packetBuffer, sizeof(packet));

        if (packet.control == 2)
        {
            fdFile = fopen(packet.content.tlvArray.buf[0].value, "wb"); // read from filename
        }
        else if (packet.control == 1)
        {
            frwite(packet.content.data.data, 1, packet.content.data.dataSize, fdFile);   
        }
        else
        {
            llclose(1);
            break;
        }
    }

    fclose(fdFile);

}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayerRole llrole;
    if (strcmp(role, "transmitter") == 0)
        llrole = LlTx;
    else
        llrole = LlRx;

    LinkLayer ll = {serialPort, llrole, baudRate, nTries, timeout};
    if (llopen(ll) == 0)
        return;

    if (llrole == LlTx)
        applicationSend(ll, filename);
    else
        applicationReceive(ll);

}
