#include <iostream>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cerrno>
#include <unistd.h>

using namespace std;

void signalHandler(int signal){
    printf("Sinal recebido %d\n",signal);

    if (signal == 2){
    	printf("Saindo do processo...");
        exit(signal);
    }
}

int main(int argc, char** argv) {
    signal(SIGINT, signalHandler); //2
    signal(SIGUSR1, signalHandler); //10
    signal(SIGUSR2, signalHandler); //12

    while(1) {
		//pause()
        printf("Esperando sinal...\n");
        usleep(1000000);
    }
    return 0;
}