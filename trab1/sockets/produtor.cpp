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

#define PORT 3000
using namespace std;
int gerarNumero(int ant) {
	return rand() % 100 + ant;
}

int main(int argc, char** argv) {
	// Verifica se existem argumentos no programa
	if(argc<2) {
		cout << "Favor informar o numero de numeros a ser gerado" << endl;
		return 0;
	}
	int cont=0, limite = stoi(argv[1]), numero=0;
	if(limite<=0) {
		cout << "Favor inserir um limite positivo e não nulo.";
	}





	// Preparamos o inicio do programa
	// Criamos o socket e aguardamos a conexao
	srand(time(NULL));
	int server, new_socket, valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024];
	server=socket(AF_INET, SOCK_STREAM,0);
	if(server==0){
		perror("Falha na criação do socket!");
		exit(EXIT_FAILURE);
	}

	// Vinculamos o produtor a porta
	if(setsockopt(server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("Erro ao vincular porta 3000");
        exit(EXIT_FAILURE);
	}
	
	address.sin_family=AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if(bind(server, (struct sockaddr *)&address, sizeof(address))<0) {
		perror("bind failed");
        exit(EXIT_FAILURE);
	}
	if (listen(server, 3) < 0) {
        perror("erro no listen");
        exit(EXIT_FAILURE);
    }
	
	if ((new_socket = accept(server, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0){
        perror("accept");
        exit(EXIT_FAILURE);
    }
	
	// Existindo argumentos, comecamos a gerar numeros convertidos para string.

	do {
		numero = gerarNumero(numero);
		cont++;
		const char * msg = (to_string(numero)).c_str();
		// envia pro cliente.
		send(new_socket, msg, strlen(msg),0);
		valread = read( new_socket , buffer, 1024);
		int resposta = atoi(buffer);
		if(resposta==1) {
			cout << "Enviei: " << numero << " e recebi que ele É primo" << endl;
		}
		else {
			cout << "Enviei: " << numero << " e recebi que ele NÃO é primo" << endl;
		}
	} while(cont < limite);
	cout << "Encerrei! Enviando zero para o consumidor." << endl;
	send(new_socket, "0", strlen("0"), 0);
	return 0;
}
