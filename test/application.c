#include <stdio.h>
#include "../include/application_layer.h"
int main(int argc,char*argv[]){

    // LinkLayer parameters;
    // parameters.nRetransmissions = 3;
    // parameters.timeout = 2;
    // parameters.baudRate = BAUDRATE;
    // parameters.role = LlRx;
    // strcpy(parameters.serialPort,);
    printf("Argc:%d\n",argc);
    if (argc < 2)
        applicationLayer("/dev/ttyS10", "Lltx",123, 3, 3, "bcd");
    else
        applicationLayer("/dev/ttyS11", "Llrx",123, 3, 3, "bcd");
    return 0;
}
