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
#include <atomic>
#include <sys/mman.h>
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
    char buffer[1025] = {0};
};

atomic<int>* count_accepted;
atomic<int>* count_ready;
atomic<int>* count_server;
int len_proc;
struct process *procs;
thread* geracoes[2];
vector<string> geral_frases;


// variaveis do socket
int opt = true;
int master_socket, addrlen,  *client_socket , max_clients;
struct sockaddr_in address;  
char read_buffer[1025], write_buffer[1025];  //data buffer of 1K
fd_set* serverfds, *clientfds, *exceptfds;

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
    // while(true) {
        int i;
        string f = to_string(id) + " Local - " + gerar_frase(); // gerei um evento
        //cout << f << endl;
        geral_frases.push_back(f);
        // envia eventos pra todos
        // 1. envia para todos os clientes
        for(i=0;i<len_proc-id;i++)
            //cout << "CLIENTES: enviado de " << id << " para " << clientes[i].sock << endl;
            send(clientes[i].sock, f.c_str(), strlen(f.c_str()), 0);
        // 2. Envia para todos os servidores
        for (i=0;i<max_clients;i++) {
            if(client_socket[i]>0) {
                //cout << "SERVIDORES: enviado de " << id << " para " << client_socket[i] << endl;
                send(client_socket[i], f.c_str(), strlen(f.c_str()),0);
            }
        }
    // }
}

int preparar_sockets() {
     // 2. Criamos o socket master do servidor
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
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
    if (listen(master_socket, len_proc) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    //cout << "Processo " << to_string(id) << " com socket configurado e escutando na porta " << to_string(address.sin_port) << endl;
    return 1;
}

int geracao_remota() {

    // recebe a frase;
    //if(id==0) { cout << "ID ZERO" << endl;}
    
    //cout << id << " GERACAO REMOTA " << count_ready->load()  << endl;
    addrlen = sizeof(address);
    serverfds = new fd_set;
    clientfds = new fd_set;
    exceptfds = new fd_set;
    while(true) {        
        int max_sd;
        FD_ZERO(serverfds);
        FD_ZERO(clientfds);
        max_sd=master_socket;
        FD_SET(master_socket, serverfds);
        int sd;
        for(int i=0;i<max_clients;i++) {
            sd = client_socket[i];
            if(sd>0)
                FD_SET(sd, serverfds);
            
            if(sd > max_sd)
                max_sd = sd;

        }
        for(int i=0;i<len_proc-id;i++) {
            sd = clientes[i].sock;
            if(sd >0)
                FD_SET(sd, clientfds);
            if(sd>max_sd)
                max_sd=sd;
        }
        int activity;
        activity = select(max_sd+1, serverfds, clientfds, exceptfds,NULL);
        if(activity<0)
            cout << "erro no select" << endl;
        
        // se algo aconteceu ao master socket, entao é uma nova conexão que deve ser tratada.
        if (FD_ISSET(master_socket, serverfds)) {
            int new_socket;
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            count_accepted->fetch_add(1);
            //printf("ID: %d - New connection , socket fd is %d , ip is : %s , port : %d \n" , id, new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            
            //adiciona no array de sockets
            int i;
            for (i = 0; i < max_clients; i++) 
            {
                //procura a primeira posicao vazia no array
                if( client_socket[i] == 0 )
                {   
                    client_socket[i] = new_socket;
                    //printf("Adicionanado na posicao %d\n" , i);
                    break;
                }
            }
        } else {
            
            // se nao, é uma operacao de IO no socket!
            int tmp=0;
            char *buf;
            int i;
            for (i = 0; i < max_clients+(len_proc-id); i++) 
            {
                
                if(i>=max_clients) {
                    tmp = i;
                    i-=max_clients;
                    sd=clientes[i].sock;
                    buf = clientes[i].buffer;
                }
                else {
                    sd = client_socket[i];
                    buf = read_buffer;
                }
                
                
                if (FD_ISSET( sd , serverfds) || FD_ISSET(sd, clientfds)) 
                {
                    //Verifica se é alguem fechando a conexao
                    int valread;
                    if ((valread = read( sd , read_buffer, 1024)) == 0)
                    {
                        //Pega os dados de quem desconectou e printa
                        getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                        printf("Host desconectado , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                        
                        //Close the socket and mark as 0 in list for reuse
                        close( sd );
                        client_socket[i] = 0;
                    }
                    
                    //Se nao for, ja temos o valor lido no nosso buffer
                    else
                    {
                        //Adiciona o caracter de terminacao de string;
                        read_buffer[valread] = '\0';
                        cout << "ID " << id << " recebeu: " << read_buffer << endl;

                    }
                }

                if(tmp>0) {
                    i=tmp;
                    tmp=0;
                }
            }
        }
        
    }
    //processa e adiciona nafila local
    string f = to_string(id) + " Remota - Frase recebida";
    geral_frases.push_back(f);
    
}



int conectar_clientes(){
    // 7. Agora vamos configurar os clients.
    clientes = new client_properties[len_proc-id];
    int port_iterator=id+1;
    //cout << "Id: " << id << " _ " << len_proc - id << endl; // debug
    for(int i=0;i<len_proc-id;i++) {
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
        //cout <<"Conexão enviada por ID: " << id << " Para ID: " << port_iterator << " com porta " << to_string(clientes[i].serv_addr.sin_port) <<endl;
        if (connect(clientes[i].sock, (struct sockaddr *)&clientes[i].serv_addr, sizeof(clientes[i].serv_addr)) < 0)
        {
            cout << "\n"<< id <<" - Connection Failed \n";
            return -1;
        }
        port_iterator++;
    }
    return 1;
}



string convert (int b[]){
    
    string a = to_string(b[0]);
    for ( int i = 1 ; i < 6 ; i ++ ){
        a.append(", " + to_string(b[i]));
    }
    return a;
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

    //memória compatilhada para armazenar variavies de controle de sockets
    count_accepted = static_cast<atomic<int>*>(mmap(0, sizeof *count_accepted, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    count_ready = static_cast<atomic<int>*>(mmap(0, sizeof *count_ready, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    count_server = static_cast<atomic<int>*>(mmap(0, sizeof *count_server, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    
    spawn(len_proc);

    // 1. Instanciamos e inicializamos o vetor de clients
    srand(time(NULL) + id);
    max_clients = len_proc;
    client_socket = new int [max_clients];
    for(int i =0; i< max_clients;i++)
        client_socket[i]=0;
    

    //o servidor socket de cada processo será preparado e configurado para ouvir novas conexões
    preparar_sockets();
    count_server->fetch_add(1);
    while (static_cast<int>(count_server->load()) != (len_proc + 1)) {
        std::this_thread::yield();
    }
    //cada processo irá solicitar conexão para os processos com maior ID
    conectar_clientes();
    count_ready->fetch_add(1);
    while (static_cast<int>(count_ready->load()) != (len_proc + 1)) {
        std::this_thread::yield();
    }
    
    
    //o processo irá escutar a porta do socket e processar os dados e novas conexões
    geracoes[1] = new thread(geracao_remota);    
    while (static_cast<int>(count_accepted->load()) != ((double)len_proc + 1)*((double)len_proc/2)) {
        std::this_thread::yield();
    }

    //cout << id << " Client socket: " << convert(client_socket) << endl;

    //agora que todos estão conectados e ouvindo as devidas portas, gerar mensagens
    geracoes[0] = new thread(geracao_local);
    
    geracoes[0]->join();
    geracoes[1]->join();
    for(int i = 0; i < geral_frases.size(); i++) {
        // cout << geral_frases[i] << endl;
    }
}