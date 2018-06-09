#include <iostream>
#include <fstream>
#include <string.h>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <ctime>
#include <unistd.h>
#include <thread> 
#include <vector>
// comunicacao via socket
#include<sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BASE_PORT 3000
using namespace std;


struct process {
    int id;
    string ip;
    int port;
};

struct client_properties {
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[1025] = {0};
};


int len_proc;
struct process *procs;
thread geracoes[2];
vector<string> geral_frases;



// variaveis do socket
int opt = true;
int master_socket , addrlen , new_socket , *client_socket , max_clients, activity, i , valread , sd;
int max_sd;
struct sockaddr_in address;  
char read_buffer[1025], write_buffer[1025];  //data buffer of 1K
fd_set readfds;

struct client_properties *clientes;
// fim das variaveis de socket
pid_t *processos;
int id;
bool spawn(int n) { // cria n processos recursivamente.
    if(n) {
        if(fork()==0) {
            id--;
            if(n) {
                spawn(n-1);
            }
            else
                return true;
        }
    }
}

bool carregar_tabela(string path) {
    ifstream file (path);
    string valor;
    while(file.good()){
        getline(file, valor, ',');
    }
}

string gerar_frase(){
    string frases[] = {
        "Quando vires um homem bom, tenta imitá-lo; quando vires um homem mau, examina-te a ti mesmo.",
        "Tente mover o mundo - o primeiro passo será mover a si mesmo.",
        "A religião do futuro será cósmica e transcenderá um Deus pessoal, evitando os dogmas e a teologia.",
        "Não ser descoberto numa mentira é o mesmo que dizer a verdade.",
        "Mesmo desacreditado e ignorado por todos, não posso desistir, pois para mim, vencer é nunca desistir.",
        "Para ver muita coisa é preciso despregar os olhos de si mesmo",
        "Duas coisas são infinitas: o universo e a estupidez humana. Mas, em relação ao universo, ainda não tenho certeza absoluta.",
        "Há uma inocência na admiração: é a daquele a quem ainda não passou pela cabeça que também ele poderia um dia ser admirado.",
        "A alegria que se tem em pensar e aprender faz-nos pensar e aprender ainda mais."
    };
    
    auto len = end(frases) - begin(frases);
    return frases[rand() % len];
}

int geracao_local() {
    string f = to_string(id) + " Local - " + gerar_frase(); // gerei um evento
    geral_frases.push_back(f);
    // envia eventos pra todos
}

int geracao_remota() {
    // recebe a frase;
    string f = to_string(id) + " Remota - Frase recebida";
    geral_frases.push_back(f);
}


int programa() {
    srand(time(NULL) + id);
    /* HORA DE CONFIGURAR AS CONEXOES */

    // 1. Instanciamos e inicializamos o vetor de clients
    max_clients = len_proc;
    client_socket = new int [max_clients];
    for(int i =0; i< max_clients;i++)
        client_socket[i]=0;

    // 2. Criamos o socket master do servidor
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 3. Configuramos o socket para aceitar multiplas conexoes
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // 4. Configuracoes do endereco (e.g. Porta, familia, e enderecos)    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( BASE_PORT+id ); // detalhe: ele comeca a escutar na porta base+id

    // 5. Fazemos o bind do endereco no socket
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // 6. Colocamos o socket para aceitar conexoes. O numero maximo de pedidos pendentes é 3.
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    addrlen = sizeof(address);

    // 7. Agora vamos configurar os clients.
    clientes = new client_properties[len_proc-id];
    int port_iterator=id;
    for(int i=0;i<len_proc-id;i++) {
        port_iterator--;
        if ((clientes[i].sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            cout << "\n Socket creation error \n";
            return -1;
        }
    
        memset(&clientes[i].serv_addr, '0', sizeof(clientes[i].serv_addr));
    
        clientes[i].serv_addr.sin_family = AF_INET;
        clientes[i].serv_addr.sin_port = htons(BASE_PORT + port_iterator);
        
        // Convert IPv4 and IPv6 addresses from text to binary form
        if(inet_pton(AF_INET, "127.0.0.1", &clientes[i].serv_addr.sin_addr)<=0) 
        {
            cout << "\nInvalid address/ Address not supported \n";
            return -1;
        }
    
        if (connect(clientes[i].sock, (struct sockaddr *)&clientes[i].serv_addr, sizeof(clientes[i].serv_addr)) < 0)
        {
            cout << "\nConnection Failed \n";
            return -1;
        }
        cout << "conectado!" << endl << endl;
    }

    geracoes[0] = thread(geracao_local);
    geracoes[1] = thread(geracao_remota);
    
    //cout << "ID: " << id << " " << gerar_frase()<<endl;
    return 1;
}

int main(int argc, char *argv[]) {
    //gerar processos
    if(argc<2) {
        cout << "Informe o número de processos que serão criados!" << endl;
        return 0;
    }
    
    len_proc = atoi(argv[1]);
    id=len_proc;
    processos = new pid_t[len_proc];
    procs = new struct process[len_proc];
    //carregar_tabela("tabela.csv");    

    spawn(len_proc);
    programa();
    geracoes[0].join();
    geracoes[1].join();
    for(int i = 0; i < geral_frases.size(); i++) {
        cout << geral_frases[i] << endl;
    }
}