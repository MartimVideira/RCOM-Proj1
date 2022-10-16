// Application layer protocol implementation

//#include "application_layer.h"
#include "../include/application_layer.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    // TODO
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
    // Depois de ver que consigo ler o ficheiro definir o pacote de dados
    // Usar a interface com a link layer 
    // Criar um LinkLayer paraameters
    
}
