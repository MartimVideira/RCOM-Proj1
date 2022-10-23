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
        applicationLayer("/dev/ttyS10", "tx",123, 3, 3, "highRes.jpg");
    else
        applicationLayer("/dev/ttyS11", "rx",123, 3, 3, "bcd");
    return 0;
}
