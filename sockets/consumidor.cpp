/**
*
* Produtor / Servidor
*
**/
#include <stdlib.h>
#include<stdio.h>
#include<iostream>
#include<ctime>
#include<cstdlib>
#include<sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include<unistd.h>
#include<math.h>
#include <arpa/inet.h>

#define PORT 3000
using namespace std;

bool isPrime (int num)
{
    if (num <=1)
        return false;
    else if (num == 2)         
        return true;
    else if (num % 2 == 0)
        return false;
    else
    {
        bool prime = true;
        int divisor = 3;
        double num_d = static_cast<double>(num);
        int upperLimit = static_cast<int>(sqrt(num_d) +1);
        
        while (divisor <= upperLimit)
        {
            if (num % divisor == 0)
                prime = false;
            divisor +=2;
        }
        return prime;
    }
}

int main(int argc, char** argv) {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[1024] = {0};
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        cout << "\n Socket creation error \n";
        return -1;
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
  
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
    {
        cout << "\nInvalid address/ Address not supported \n";
        return -1;
    }
  
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cout << "\nConnection Failed \n";
        return -1;
    }
    
    cout << "Hello message sent\n";
    bool fim = false;
    //devo ler até receber um 0
    while(!fim) {
        // leio a msg
        valread = read( sock , buffer, 1024);
        //verifico se eh zero
        int valor = atoi(buffer);
        if(valor==0) {
            fim=1;
            cout << "Recebi zero, encerrando." << endl;
            break;
        }
        memset(buffer, 0, sizeof(buffer));
        //verifico se e primo
        if(isPrime(valor)){
            const char * msg = (to_string(1)).c_str();
            send(sock , msg , strlen(msg) , 0 );
        }
        else {
            const char * msg = (to_string(0)).c_str();
            send(sock , msg , strlen(msg) , 0 );
        }
        cout << "Recebi " << valor << " Primo: " << isPrime(valor) << endl;
        //respondo
        
    }
}