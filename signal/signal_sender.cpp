
#include <iostream>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cerrno>

using namespace std;

bool processExists(int pID){
    return 0 == kill(pID, 0);
}

int main(int argc, char** argv) {
    try
    {
        int pID = atoi(argv[1]);
        int signal = atoi(argv[2]);
        bool pIDExists = processExists(pID);
        if (!pIDExists) {
            printf("Processo %d não existe\n", pID);
            return 0;
        }
        int response = kill( pID, signal);
        if (response == 0){
            printf( "Sinal enviado %d ao processo %d\n", signal, pID);
        }
        else {
            printf("Signal not sent");
        }
    }
    catch (int e) {
        printf ("Erro %d\n", e);
    }
    return 0;
}




