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
    union {
        TlvArray tlvArray;
        DataPacket data;
    } content;

} Packet;


void applicationSend(LinkLayer parameters,const char *filename){

    struct stat fileStat;
    if (stat(filename,&fileStat) != 0){
        printf("File does not exist\n");
        return;
    }
    if (S_ISREG(fileStat.st_mode) == 0){
        printf("The given path corresponds to a directory!\n");
        return;
    }
    char  filePath[300];
    strcpy(filePath,filename);
    const char* baseFilename = basename(filePath);
    printf("About the File\n");
    printf("Name: %s\n",baseFilename);
    printf("Size: %ld bytes\n",fileStat.st_size);

    // Já consigo ver se o ficheiro existe e não é um diretorio tenho acesso ao nome dele
    // Falta secalhar truncar e considerar so o nome do ficheiro e não o path ver como isso será possivel
    // Depois falta abrir em modo binário e comecar a ler o ficheiro!
}
void applicationRecieve(LinkLayer parameters){
    // What if that fileName already exists?
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // Create Linklayer parameters and do a switch on the role
}
